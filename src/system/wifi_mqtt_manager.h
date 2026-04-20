#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "../config/wifi_mqtt.h"
#include "../logging/grind_logging.h"

//------------------------------------------------------------------------------
// WiFi + MQTT configuration — persisted in NVS namespace "wifi"
//------------------------------------------------------------------------------
struct WiFiMqttConfig {
    char     ssid[64];
    char     wifi_pass[64];
    char     broker[64];
    uint16_t port;
    char     mqtt_user[32];
    char     mqtt_pass[64];
    char     topic[64];
    bool     tls;

    WiFiMqttConfig() {
        memset(this, 0, sizeof(WiFiMqttConfig));
        port = WIFI_MQTT_DEFAULT_PORT;
        strncpy(topic, WIFI_MQTT_DEFAULT_TOPIC, sizeof(topic) - 1);
    }
};

//------------------------------------------------------------------------------
// Publish request — snapshot of one completed grind session
//------------------------------------------------------------------------------
struct MqttPublishRequest {
    GrindSession session;
    GrindEvent   events[MAX_EVENTS_PER_GRIND];
    uint16_t     event_count;
};

/**
 * WiFiMqttManager
 *
 * Manages WiFi connection, MQTT publishing, and configuration persistence.
 * publish() is a blocking call intended to run from the dedicated WiFiMqttTask.
 * WiFi is brought up only for the duration of the publish, then torn down.
 */
class WiFiMqttManager {
public:
    void init(Preferences* prefs);

    // Enable / disable (persisted to NVS)
    bool is_enabled() const;
    void set_enabled(bool enabled);

    // Config persistence
    WiFiMqttConfig load_config() const;
    void           save_config(const WiFiMqttConfig& cfg);

    // Blocking publish — call from WiFiMqttTask only
    bool publish(const MqttPublishRequest& req);

private:
    Preferences* _prefs = nullptr;

    bool connect_wifi(const WiFiMqttConfig& cfg);
    void disconnect_wifi();

    String build_session_json(const GrindSession& session) const;
    String build_events_json(const GrindEvent* events, uint16_t count) const;
};

extern WiFiMqttManager wifi_mqtt_manager;
