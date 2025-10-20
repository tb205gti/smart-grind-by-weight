#pragma once
#include <lvgl.h>
#include "../../config/constants.h"
#include "../../bluetooth/manager.h"
#include "../../controllers/grind_controller.h"
#include "../../system/diagnostics_controller.h"
#include "../ui_helpers.h"

class GrindingScreen;  // Forward declaration

class MenuScreen {
private:
    lv_obj_t* screen;
    lv_obj_t* menu;
    lv_obj_t* info_page;
    lv_obj_t* bluetooth_page;
    lv_obj_t* display_page;
    lv_obj_t* grind_mode_page;
    lv_obj_t* data_page;
    lv_obj_t* stats_page;
    lv_obj_t* diagnostics_page;
    lv_obj_t* scale_page;

    // Info tab elements
    lv_obj_t* info_label;
    lv_obj_t* uptime_label;
    lv_obj_t* memory_label;
    lv_obj_t* instant_label;
    lv_obj_t* samples_label;
    lv_obj_t* raw_label;
    lv_obj_t* sessions_label;
    lv_obj_t* events_label;
    lv_obj_t* measurements_label;
    lv_obj_t* refresh_stats_button;

    // Lifetime statistics labels
    lv_obj_t* stat_total_grinds_label;
    lv_obj_t* stat_shots_label;
    lv_obj_t* stat_motor_runtime_label;
    lv_obj_t* stat_device_uptime_label;
    lv_obj_t* stat_total_weight_label;
    lv_obj_t* stat_mode_grinds_label;
    lv_obj_t* stat_avg_accuracy_label;
    lv_obj_t* stat_total_pulses_label;
    
    // Menu toggle elements
    lv_obj_t* ble_toggle;
    lv_obj_t* ble_startup_toggle;
    lv_obj_t* ble_status_label;
    lv_obj_t* ble_timer_label;
    lv_obj_t* logging_toggle;
    lv_obj_t* brightness_normal_slider;
    lv_obj_t* brightness_screensaver_slider;
    lv_obj_t* brightness_normal_label;
    lv_obj_t* brightness_screensaver_label;
    lv_obj_t* purge_button;
    lv_obj_t* reset_button;
    
    // Grind mode tab elements
    lv_obj_t* grind_mode_radio_group;
    lv_obj_t* grind_mode_swipe_toggle;
    lv_obj_t* auto_start_toggle;
    lv_obj_t* auto_return_toggle;
    
    // Tools entries / scale page elements
    lv_obj_t* scale_item;
    lv_obj_t* cal_button;
    lv_obj_t* motor_test_button;
    lv_obj_t* autotune_button;
    lv_obj_t* scale_weight_label;
    lv_obj_t* scale_tare_button;

    // Diagnostics tab elements
    lv_obj_t* diag_status_label;
    lv_obj_t* diag_calibration_factor_label;
    lv_obj_t* diag_std_dev_g_label;
    lv_obj_t* diag_std_dev_adc_label;
    lv_obj_t* diag_noise_level_label;
    lv_obj_t* diag_motor_latency_label;
    lv_obj_t* diag_info_label;
    lv_obj_t* diag_reset_button;

    // Common elements
    bool visible;
    bool scale_active;
    
    BluetoothManager* bluetooth_manager;
    GrindController* grind_controller;
    GrindingScreen* grinding_screen;
    class HardwareManager* hardware_manager; // Forward declaration to access preferences
    DiagnosticsController* diagnostics_controller;

public:
    void create(BluetoothManager* bluetooth, GrindController* grind_ctrl, GrindingScreen* grind_screen, class HardwareManager* hw_mgr, DiagnosticsController* diag_ctrl);
    void show();
    void hide();
    void update_info(const WeightSensor* weight_sensor, unsigned long uptime_ms, size_t free_heap);
    void update_diagnostics(WeightSensor* weight_sensor);
    void update_ble_status();
    void refresh_statistics(bool show_overlay = true);
    void update_brightness_labels(int normal_percent = -1, int screensaver_percent = -1); // Use negative value to leave unchanged
    void update_brightness_sliders();
    void update_bluetooth_startup_toggle();
    void update_logging_toggle();
    void update_grind_mode_toggles();
    void reset_scale_display();
    void update_scale_weight(float weight);

    bool is_visible() const { return visible; }
    lv_obj_t* get_screen() const { return screen; }
    lv_obj_t* get_tabview() const { return menu; }
    lv_obj_t* get_diagnostics_page() const { return diagnostics_page; }
    lv_obj_t* get_cal_button() const { return cal_button; }
    lv_obj_t* get_purge_button() const { return purge_button; }
    lv_obj_t* get_reset_button() const { return reset_button; }
    lv_obj_t* get_motor_test_button() const { return motor_test_button; }
    lv_obj_t* get_autotune_button() const { return autotune_button; }
    bool is_scale_page_active() const { return scale_active; }
    lv_obj_t* get_ble_toggle() const { return ble_toggle; }
    lv_obj_t* get_ble_startup_toggle() const { return ble_startup_toggle; }
    lv_obj_t* get_logging_toggle() const { return logging_toggle; }
    lv_obj_t* get_refresh_stats_button() const { return refresh_stats_button; }
    lv_obj_t* get_diag_reset_button() const { return diag_reset_button; }
    lv_obj_t* get_brightness_normal_slider() const { return brightness_normal_slider; }
    lv_obj_t* get_brightness_screensaver_slider() const { return brightness_screensaver_slider; }
    lv_obj_t* get_grind_mode_radio_group() const { return grind_mode_radio_group; }
    lv_obj_t* get_grind_mode_swipe_toggle() const { return grind_mode_swipe_toggle; }
    lv_obj_t* get_auto_start_toggle() const { return auto_start_toggle; }
    lv_obj_t* get_auto_return_toggle() const { return auto_return_toggle; }

private:
    void create_menu_ui();
    void create_info_page(lv_obj_t* parent);
    void create_bluetooth_page(lv_obj_t* parent);
    void create_display_page(lv_obj_t* parent);
    void create_grind_mode_page(lv_obj_t* parent);
    void create_scale_page(lv_obj_t* parent);
    void create_data_page(lv_obj_t* parent);
    void create_stats_page(lv_obj_t* parent);
    void create_diagnostics_page(lv_obj_t* parent);
    lv_obj_t* create_separator(lv_obj_t* parent, const char* text = nullptr);
    lv_obj_t* create_menu_item(lv_obj_t* parent, const char* text);
    lv_obj_t *create_toggle_row(lv_obj_t *parent, const char *text,lv_obj_t **out_toggle);
    lv_obj_t *create_slider_row(lv_obj_t *parent, const char *text,
                                lv_obj_t **label, lv_obj_t **slider,
                                lv_color_t slider_color = lv_color_hex(THEME_COLOR_ACCENT),
                                uint32_t min = 0, uint32_t max = 100);
    lv_obj_t *create_static_data_label(lv_obj_t *parent, const char *name,
                                       const char *value);
    lv_obj_t *create_data_label(lv_obj_t *parent, const char *name,
                                lv_obj_t **variable, bool stacked = false);
    lv_obj_t *create_description_label(lv_obj_t *parent, const char *text);
};
