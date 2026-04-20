#include "wifi_mqtt_manager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../config/logging.h"

WiFiMqttManager wifi_mqtt_manager;

void WiFiMqttManager::init(Preferences* prefs) {
    _prefs = prefs;
}

bool WiFiMqttManager::is_enabled() const {
    Preferences p;
    p.begin(WIFI_MQTT_NVS_NAMESPACE, true);
    bool enabled = p.getBool(WIFI_MQTT_NVS_ENABLED, false);
    p.end();
    return enabled;
}

void WiFiMqttManager::set_enabled(bool enabled) {
    Preferences p;
    p.begin(WIFI_MQTT_NVS_NAMESPACE, false);
    p.putBool(WIFI_MQTT_NVS_ENABLED, enabled);
    p.end();
}

WiFiMqttConfig WiFiMqttManager::load_config() const {
    WiFiMqttConfig cfg;
    Preferences p;
    p.begin(WIFI_MQTT_NVS_NAMESPACE, true);

    String ssid     = p.getString(WIFI_MQTT_NVS_SSID,      "");
    String pass     = p.getString(WIFI_MQTT_NVS_WIFI_PASS,  "");
    String broker   = p.getString(WIFI_MQTT_NVS_BROKER,     "");
    String muser    = p.getString(WIFI_MQTT_NVS_MQTT_USER,  "");
    String mpass    = p.getString(WIFI_MQTT_NVS_MQTT_PASS,  "");
    String topic    = p.getString(WIFI_MQTT_NVS_TOPIC,      WIFI_MQTT_DEFAULT_TOPIC);
    cfg.port        = (uint16_t)p.getUInt(WIFI_MQTT_NVS_PORT, WIFI_MQTT_DEFAULT_PORT);
    cfg.tls         = p.getBool(WIFI_MQTT_NVS_TLS, false);

    p.end();

    strncpy(cfg.ssid,      ssid.c_str(),   sizeof(cfg.ssid)      - 1);
    strncpy(cfg.wifi_pass, pass.c_str(),   sizeof(cfg.wifi_pass) - 1);
    strncpy(cfg.broker,    broker.c_str(), sizeof(cfg.broker)    - 1);
    strncpy(cfg.mqtt_user, muser.c_str(),  sizeof(cfg.mqtt_user) - 1);
    strncpy(cfg.mqtt_pass, mpass.c_str(),  sizeof(cfg.mqtt_pass) - 1);
    strncpy(cfg.topic,     topic.c_str(),  sizeof(cfg.topic)     - 1);

    return cfg;
}

void WiFiMqttManager::save_config(const WiFiMqttConfig& cfg) {
    Preferences p;
    p.begin(WIFI_MQTT_NVS_NAMESPACE, false);
    p.putString(WIFI_MQTT_NVS_SSID,      cfg.ssid);
    p.putString(WIFI_MQTT_NVS_WIFI_PASS,  cfg.wifi_pass);
    p.putString(WIFI_MQTT_NVS_BROKER,     cfg.broker);
    p.putUInt  (WIFI_MQTT_NVS_PORT,       cfg.port);
    p.putString(WIFI_MQTT_NVS_MQTT_USER,  cfg.mqtt_user);
    p.putString(WIFI_MQTT_NVS_MQTT_PASS,  cfg.mqtt_pass);
    p.putString(WIFI_MQTT_NVS_TOPIC,      cfg.topic);
    p.putBool  (WIFI_MQTT_NVS_TLS,        cfg.tls);
    p.end();
}

bool WiFiMqttManager::connect_wifi(const WiFiMqttConfig& cfg) {
    if (cfg.ssid[0] == '\0') {
        LOG_BLE("WiFiMQTT: No SSID configured\n");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.ssid, cfg.wifi_pass);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= WIFI_MQTT_WIFI_CONNECT_TIMEOUT_MS) {
            LOG_BLE("WiFiMQTT: WiFi connect timeout (SSID: %s)\n", cfg.ssid);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    LOG_BLE("WiFiMQTT: WiFi connected, IP %s\n", WiFi.localIP().toString().c_str());
    return true;
}

void WiFiMqttManager::disconnect_wifi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

bool WiFiMqttManager::publish(const MqttPublishRequest& req) {
    WiFiMqttConfig cfg = load_config();

    if (cfg.broker[0] == '\0') {
        LOG_BLE("WiFiMQTT: No broker configured — skipping publish\n");
        return false;
    }

    if (!connect_wifi(cfg)) {
        return false;
    }

    WiFiClient     plain_client;
    WiFiClientSecure secure_client;

    if (cfg.tls) {
        secure_client.setInsecure(); // TLS without cert verification (home MQTT)
    }

    PubSubClient mqtt(cfg.tls ? (Client&)secure_client : (Client&)plain_client);
    mqtt.setServer(cfg.broker, cfg.port);

    bool connected = false;
    uint32_t mqtt_start = millis();
    while (!connected && millis() - mqtt_start < WIFI_MQTT_MQTT_CONNECT_TIMEOUT_MS) {
        if (cfg.mqtt_user[0]) {
            connected = mqtt.connect(WIFI_MQTT_CLIENT_ID, cfg.mqtt_user, cfg.mqtt_pass);
        } else {
            connected = mqtt.connect(WIFI_MQTT_CLIENT_ID);
        }
        if (!connected) {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    if (!connected) {
        LOG_BLE("WiFiMQTT: MQTT connect failed (state: %d)\n", mqtt.state());
        disconnect_wifi();
        return false;
    }

    char session_topic[128];
    char events_topic[128];
    snprintf(session_topic, sizeof(session_topic), "%s/grind_session", cfg.topic);
    snprintf(events_topic,  sizeof(events_topic),  "%s/grind_events",  cfg.topic);

    // Publish session summary
    {
        String json = build_session_json(req.session);
        mqtt.beginPublish(session_topic, json.length(), true);
        mqtt.print(json);
        mqtt.endPublish();
    }

    // Publish events array (potentially large — streamed via beginPublish)
    {
        String json = build_events_json(req.events, req.event_count);
        mqtt.beginPublish(events_topic, json.length(), true);
        mqtt.print(json);
        mqtt.endPublish();
    }

    mqtt.disconnect();
    disconnect_wifi();

    LOG_BLE("WiFiMQTT: Published session %lu (%u events) to %s\n",
            req.session.session_id, req.event_count, cfg.broker);
    return true;
}

String WiFiMqttManager::build_session_json(const GrindSession& s) const {
    JsonDocument doc;
    doc["session_id"]              = s.session_id;
    doc["timestamp"]               = s.session_timestamp;
    doc["grind_mode"]              = s.grind_mode;
    doc["profile_id"]              = s.profile_id;
    doc["target_weight"]           = s.target_weight;
    doc["final_weight"]            = s.final_weight;
    doc["error_grams"]             = s.error_grams;
    doc["tolerance"]               = s.tolerance;
    doc["start_weight"]            = s.start_weight;
    doc["target_time_ms"]          = s.target_time_ms;
    doc["total_time_ms"]           = s.total_time_ms;
    doc["motor_on_time_ms"]        = s.total_motor_on_time_ms;
    doc["time_error_ms"]           = s.time_error_ms;
    doc["pulse_count"]             = s.pulse_count;
    doc["max_pulse_attempts"]      = s.max_pulse_attempts;
    doc["termination_reason"]      = s.termination_reason;
    doc["result_status"]           = s.result_status;
    doc["initial_motor_stop_offset"] = s.initial_motor_stop_offset;
    doc["latency_to_coast_ratio"]  = s.latency_to_coast_ratio;
    doc["flow_rate_threshold"]     = s.flow_rate_threshold;

    String json;
    serializeJson(doc, json);
    return json;
}

String WiFiMqttManager::build_events_json(const GrindEvent* events, uint16_t count) const {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (uint16_t i = 0; i < count; i++) {
        const GrindEvent& ev = events[i];
        JsonObject obj = arr.add<JsonObject>();
        obj["seq_id"]             = ev.event_sequence_id;
        obj["phase_id"]           = ev.phase_id;
        obj["timestamp_ms"]       = ev.timestamp_ms;
        obj["duration_ms"]        = ev.duration_ms;
        obj["start_weight"]       = ev.start_weight;
        obj["end_weight"]         = ev.end_weight;
        obj["motor_stop_target"]  = ev.motor_stop_target_weight;
        obj["pulse_attempt"]      = ev.pulse_attempt_number;
        obj["pulse_duration_ms"]  = ev.pulse_duration_ms;
        obj["pulse_flow_rate"]    = ev.pulse_flow_rate;
        obj["grind_latency_ms"]   = ev.grind_latency_ms;
        obj["settling_ms"]        = ev.settling_duration_ms;
        obj["loop_count"]         = ev.loop_count;
        obj["flags"]              = ev.event_flags;
    }

    String json;
    serializeJson(doc, json);
    return json;
}
