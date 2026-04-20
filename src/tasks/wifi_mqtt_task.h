#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "../system/wifi_mqtt_manager.h"

/**
 * WiFiMqttTask
 *
 * Event-driven FreeRTOS task that blocks on an MQTT publish queue.
 * When a MqttPublishRequest arrives (placed by FileIOTask after a grind session ends),
 * it calls wifi_mqtt_manager.publish() which handles WiFi connect → MQTT publish → disconnect.
 *
 * Runs on Core 1 at priority 1 (lowest, same as FileIO) so it never interferes with
 * real-time tasks. WiFi+MQTT operations are fully blocking and can take several seconds.
 */
class WiFiMqttTask {
public:
    WiFiMqttTask();

    void init(QueueHandle_t mqtt_queue);

    // Static task wrapper registered with TaskManager
    static void task_wrapper(void* parameter);

    // Task implementation (public for TaskManager pattern)
    void task_impl();

private:
    QueueHandle_t   _mqtt_queue;
    volatile bool   _running;
    static WiFiMqttTask* instance;
};

extern WiFiMqttTask wifi_mqtt_task;
