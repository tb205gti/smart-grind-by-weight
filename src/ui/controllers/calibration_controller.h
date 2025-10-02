#pragma once
#include <lvgl.h>

class UIManager;

// Handles the calibration workflow (tare, weight setting, completion)

class CalibrationUIController {
public:
    explicit CalibrationUIController(UIManager* manager);

    void register_events();
    void update();

    void handle_ok();
    void handle_cancel();
    void handle_plus(lv_event_code_t code);
    void handle_minus(lv_event_code_t code);

private:
    UIManager* ui_manager_;
};
