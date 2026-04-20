#pragma once

//==============================================================================
// WIFI & MQTT CONFIGURATION
//==============================================================================
// Constants for WiFi connectivity and MQTT telemetry publishing.
// WiFi is enabled only while publishing a grind session, then disabled.

//------------------------------------------------------------------------------
// NVS KEYS (namespace: "wifi")
//------------------------------------------------------------------------------
#define WIFI_MQTT_NVS_NAMESPACE     "wifi"
#define WIFI_MQTT_NVS_ENABLED       "enabled"
#define WIFI_MQTT_NVS_SSID          "ssid"
#define WIFI_MQTT_NVS_WIFI_PASS     "wifi_pass"
#define WIFI_MQTT_NVS_BROKER        "broker"
#define WIFI_MQTT_NVS_PORT          "port"
#define WIFI_MQTT_NVS_MQTT_USER     "mqtt_user"
#define WIFI_MQTT_NVS_MQTT_PASS     "mqtt_pass"
#define WIFI_MQTT_NVS_TOPIC         "topic"
#define WIFI_MQTT_NVS_TLS           "tls"

//------------------------------------------------------------------------------
// BLE WIFI CONFIG SERVICE
//------------------------------------------------------------------------------
#define BLE_WIFI_CONFIG_SERVICE_UUID    "aabbccdd-eeff-0011-2233-445566778899"
#define BLE_WIFI_CONFIG_WRITE_CHAR_UUID "bbccddee-ff00-1122-3344-556677889900"
#define BLE_WIFI_CONFIG_STATUS_CHAR_UUID "ccddeeef-0011-2233-4455-66778899aabb"

//------------------------------------------------------------------------------
// CONNECTION TIMEOUTS
//------------------------------------------------------------------------------
#define WIFI_MQTT_WIFI_CONNECT_TIMEOUT_MS  10000   // 10 seconds to get WiFi IP
#define WIFI_MQTT_MQTT_CONNECT_TIMEOUT_MS   5000   // 5 seconds to connect MQTT broker
#define WIFI_MQTT_DEFAULT_PORT              1883
#define WIFI_MQTT_DEFAULT_TOPIC             "grinder"
#define WIFI_MQTT_CLIENT_ID                 "GrindByWeight"

//------------------------------------------------------------------------------
// FREERTOS TASK SETTINGS
//------------------------------------------------------------------------------
#define WIFI_MQTT_TASK_STACK_SIZE   8192   // 8KB stack for WiFi + JSON serialization
#define WIFI_MQTT_TASK_PRIORITY     1      // Same as FileIO (lowest priority)
#define WIFI_MQTT_QUEUE_DEPTH       3      // Max pending publish requests
