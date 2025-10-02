#pragma once
#include <lvgl.h>

class UIManager;

// Shows Bluetooth connection status icon with color coding

class StatusIndicatorController {
public:
    explicit StatusIndicatorController(UIManager* manager);

    void build();
    void register_events();
    void update();

private:
    void update_ble_status_icon();

    UIManager* ui_manager_;
    lv_obj_t* ble_status_icon_ = nullptr;
};
