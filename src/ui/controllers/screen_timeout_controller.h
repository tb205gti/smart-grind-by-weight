#pragma once

#include <cstdint>

class UIManager;

// Implements automatic screen dimming based on touch/weight activity

class ScreenTimeoutController {
public:
    explicit ScreenTimeoutController(UIManager* manager);

    void register_events();
    void update();

private:
    UIManager* ui_manager_;
    bool screen_dimmed_;
};
