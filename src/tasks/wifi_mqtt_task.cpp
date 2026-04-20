#include "wifi_mqtt_task.h"
#include "../system/wifi_mqtt_manager.h"
#include "../config/logging.h"
#include <Arduino.h>

WiFiMqttTask wifi_mqtt_task;
WiFiMqttTask* WiFiMqttTask::instance = nullptr;

WiFiMqttTask::WiFiMqttTask()
    : _mqtt_queue(nullptr)
    , _running(false) {
    instance = this;
}

void WiFiMqttTask::init(QueueHandle_t mqtt_queue) {
    _mqtt_queue = mqtt_queue;
}

void WiFiMqttTask::task_wrapper(void* parameter) {
    if (instance) {
        instance->task_impl();
    }
    vTaskDelete(nullptr);
}

void WiFiMqttTask::task_impl() {
    _running = true;
    LOG_BLE("WiFiMqttTask started on Core %d\n", xPortGetCoreID());

    MqttPublishRequest req;
    while (_running) {
        // Block indefinitely until a publish request arrives
        if (xQueueReceive(_mqtt_queue, &req, portMAX_DELAY) == pdPASS) {
            if (!wifi_mqtt_manager.is_enabled()) {
                continue;
            }
            LOG_BLE("WiFiMqttTask: Publishing session %lu...\n", req.session.session_id);
            bool ok = wifi_mqtt_manager.publish(req);
            if (!ok) {
                LOG_BLE("WiFiMqttTask: Publish failed for session %lu\n", req.session.session_id);
            }
        }
    }

    LOG_BLE("WiFiMqttTask: stopped\n");
}
