#pragma once
#include <functional>
#include <lvgl.h>

class UIManager;
enum class UIState;

// Provides confirmation dialogs with custom callbacks and button text

class ConfirmUIController {
public:
    explicit ConfirmUIController(UIManager* manager);

    void register_events();
    void update();

    void show(const char* title,
              const char* message,
              const char* confirm_text,
              lv_color_t confirm_color,
              std::function<void()> on_confirm,
              const char* cancel_text = "CANCEL",
              std::function<void()> on_cancel = nullptr);

    void handle_confirm();
    void handle_cancel();

private:
    void reset_callbacks();

    UIManager* ui_manager_;
    std::function<void()> on_confirm_;
    std::function<void()> on_cancel_;
    UIState previous_state_;
};
