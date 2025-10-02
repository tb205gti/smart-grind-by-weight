#pragma once
#include <lvgl.h>

class UIManager;

// Manages target value editing with save/cancel and plus/minus controls using jog acceleration

class EditUIController {
public:
    explicit EditUIController(UIManager* manager);

    void register_events();
    void update();
    void handle_save();
    void handle_cancel();
    void handle_plus(lv_event_code_t code);
    void handle_minus(lv_event_code_t code);
    void update_display();

private:
    UIManager* ui_manager_;
};
