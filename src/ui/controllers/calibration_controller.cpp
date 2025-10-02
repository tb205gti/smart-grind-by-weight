#include "calibration_controller.h"

#include <Arduino.h>
#include "../../config/constants.h"
#include "../components/ui_operations.h"
#include "../ui_manager.h"

CalibrationUIController::CalibrationUIController(UIManager* manager)
    : ui_manager_(manager) {}

void CalibrationUIController::register_events() {
    if (!ui_manager_) {
        return;
    }

    auto ok_btn = ui_manager_->calibration_screen.get_ok_button();
    if (ok_btn) {
        lv_obj_add_event_cb(ok_btn, [](lv_event_t* e) {
            if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
            static_cast<CalibrationUIController*>(lv_event_get_user_data(e))->handle_ok();
        }, LV_EVENT_CLICKED, this);
    }

    auto cancel_btn = ui_manager_->calibration_screen.get_cancel_button();
    if (cancel_btn) {
        lv_obj_add_event_cb(cancel_btn, [](lv_event_t* e) {
            if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
            static_cast<CalibrationUIController*>(lv_event_get_user_data(e))->handle_cancel();
        }, LV_EVENT_CLICKED, this);
    }

    auto plus_btn = ui_manager_->calibration_screen.get_plus_btn();
    if (plus_btn) {
        lv_obj_add_event_cb(plus_btn, [](lv_event_t* e) {
            static_cast<CalibrationUIController*>(lv_event_get_user_data(e))->handle_plus(lv_event_get_code(e));
        }, LV_EVENT_ALL, this);
    }

    auto minus_btn = ui_manager_->calibration_screen.get_minus_btn();
    if (minus_btn) {
        lv_obj_add_event_cb(minus_btn, [](lv_event_t* e) {
            static_cast<CalibrationUIController*>(lv_event_get_user_data(e))->handle_minus(lv_event_get_code(e));
        }, LV_EVENT_ALL, this);
    }
}

void CalibrationUIController::update() {
    if (!ui_manager_) {
        return;
    }

    CalibrationStep current_step = ui_manager_->calibration_screen.get_step();
    if (current_step == CAL_STEP_COMPLETE) {
        float weight = ui_manager_->get_hardware_manager()->get_weight_sensor()->get_display_weight();
        ui_manager_->calibration_screen.update_current_weight(weight);
    } else {
        int32_t raw_reading = ui_manager_->get_hardware_manager()->get_weight_sensor()->get_raw_adc_instant();
        ui_manager_->calibration_screen.update_current_weight(static_cast<float>(raw_reading));
    }
}

void CalibrationUIController::handle_ok() {
    if (!ui_manager_) return;

    CalibrationStep step = ui_manager_->calibration_screen.get_step();
    switch (step) {
        case CAL_STEP_EMPTY:
            UIOperations::execute_tare(ui_manager_->get_hardware_manager(), [this]() {
                ui_manager_->calibration_screen.set_step(CAL_STEP_WEIGHT);
            });
            break;
        case CAL_STEP_WEIGHT: {
            float cal_weight = ui_manager_->calibration_screen.get_calibration_weight();
            UIOperations::execute_calibration(ui_manager_->get_hardware_manager(), cal_weight, [this]() {
                ui_manager_->calibration_screen.set_step(CAL_STEP_COMPLETE);
            });
            break;
        }
        case CAL_STEP_COMPLETE:
            ui_manager_->set_current_tab(3);
            ui_manager_->switch_to_state(UIState::SETTINGS);
            break;
    }
}

void CalibrationUIController::handle_cancel() {
    if (!ui_manager_) return;
    ui_manager_->set_current_tab(3);
    ui_manager_->switch_to_state(UIState::SETTINGS);
}

void CalibrationUIController::handle_plus(lv_event_code_t code) {
    if (!ui_manager_) return;

    if (code == LV_EVENT_CLICKED) {
        float cal_weight = ui_manager_->calibration_screen.get_calibration_weight();
        cal_weight = ui_manager_->get_profile_controller()->clamp_weight(cal_weight + USER_FINE_WEIGHT_ADJUSTMENT_G);
        ui_manager_->calibration_screen.update_calibration_weight(cal_weight);
    } else if (code == LV_EVENT_LONG_PRESSED) {
        if (ui_manager_->jog_adjust_controller_) {
            ui_manager_->jog_adjust_controller_->start(1);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (ui_manager_->jog_adjust_controller_) {
            ui_manager_->jog_adjust_controller_->stop();
        }
    }
}

void CalibrationUIController::handle_minus(lv_event_code_t code) {
    if (!ui_manager_) return;

    if (code == LV_EVENT_CLICKED) {
        float cal_weight = ui_manager_->calibration_screen.get_calibration_weight();
        cal_weight = ui_manager_->get_profile_controller()->clamp_weight(cal_weight - USER_FINE_WEIGHT_ADJUSTMENT_G);
        ui_manager_->calibration_screen.update_calibration_weight(cal_weight);
    } else if (code == LV_EVENT_LONG_PRESSED) {
        if (ui_manager_->jog_adjust_controller_) {
            ui_manager_->jog_adjust_controller_->start(-1);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (ui_manager_->jog_adjust_controller_) {
            ui_manager_->jog_adjust_controller_->stop();
        }
    }
}
