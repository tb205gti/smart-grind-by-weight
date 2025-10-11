#include "settings_screen.h"
#include <Arduino.h>
#include "../../config/constants.h"
#include "../../logging/grind_logging.h"
#include "../../system/statistics_manager.h"
#include "../../hardware/hardware_manager.h"
#include "grinding_screen.h"
#include "../event_bridge_lvgl.h"
#include "../../config/logging.h"
#include "../components/blocking_overlay.h"

static void back_event_handler(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target_obj(e);
    lv_obj_t * menu = (lv_obj_t *)lv_event_get_user_data(e);

    if(lv_menu_back_button_is_root(menu, obj)) {
        // Call the EventBridge handle_event directly with SETTINGS_BACK event
        EventBridgeLVGL::handle_event(EventBridgeLVGL::EventType::SETTINGS_BACK, e);
    }
}

void SettingsScreen::create(BluetoothManager* bluetooth, GrindController* grind_ctrl, GrindingScreen* grind_screen, class HardwareManager* hw_mgr, DiagnosticsController* diag_ctrl) {
    bluetooth_manager = bluetooth;
    grind_controller = grind_ctrl;
    grinding_screen = grind_screen;
    hardware_manager = hw_mgr;
    diagnostics_controller = diag_ctrl;
    
    screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));
    lv_obj_align(screen, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    // Create menu instead of tabview
    menu = lv_menu_create(screen);
    lv_obj_set_size(menu, LV_PCT(100), LV_PCT(100));
    lv_obj_align(menu, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(menu, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(menu, 0, 0);
    lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
    lv_obj_add_event_cb(menu, back_event_handler, LV_EVENT_CLICKED, menu);

    // Get header and set up flex layout
    lv_obj_t* header = lv_menu_get_main_header(menu);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_bottom(header, 10, 0);

    // Get the header label
    lv_obj_t* header_label = lv_obj_get_child(header, -1);
    if (header_label) {
        lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(header_label, &lv_font_montserrat_36, 0);
        lv_obj_set_style_text_color(header_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    }
    // Get and style the chevron first
    lv_obj_t* back_chevron = lv_menu_get_main_header_back_button(menu);
    lv_obj_set_style_text_font(back_chevron, &lv_font_montserrat_32, 0);
    lv_obj_set_ext_click_area(back_chevron, 200);
    lv_obj_set_style_text_color(back_chevron, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);

    // Force layout update to get proper chevron dimensions
    lv_obj_update_layout(header);

    // Create spacer using chevron's actual size
    // Spacer is used to center name of the page name in the header
    lv_obj_t* spacer = lv_obj_create(header);
    lv_obj_set_size(spacer, lv_obj_get_width(back_chevron), lv_obj_get_height(back_chevron));
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

    // Create main page last
    lv_obj_t* main_page = lv_menu_page_create(menu, "Settings");

    // Create sub-pages with titles
    info_page = lv_menu_page_create(menu, "Info");
    create_info_page(info_page);

    bluetooth_page = lv_menu_page_create(menu, "Bluetooth");
    create_bluetooth_page(bluetooth_page);

    display_page = lv_menu_page_create(menu, "Display");
    create_display_page(display_page);
    
    grind_mode_page = lv_menu_page_create(menu, "Grind Settings");
    create_grind_mode_page(grind_mode_page);
    
    tools_page = lv_menu_page_create(menu, "Tools");
    create_tools_page(tools_page);
    
    data_page = lv_menu_page_create(menu, "Logs & Data");
    create_data_page(data_page);

    stats_page = lv_menu_page_create(menu, "Lifetime Stats");
    create_stats_page(stats_page);

    diagnostics_page = lv_menu_page_create(menu, "Diagnostics");
    create_diagnostics_page(diagnostics_page);

    // Create menu items grouped with separators
    create_separator(main_page, "Settings");
    lv_obj_t* bluetooth_item = create_menu_item(main_page, "Bluetooth");
    lv_menu_set_load_page_event(menu, bluetooth_item, bluetooth_page);

    lv_obj_t* display_item = create_menu_item(main_page, "Display");
    lv_menu_set_load_page_event(menu, display_item, display_page);

    lv_obj_t* grind_mode_item = create_menu_item(main_page, "Grind Settings");
    lv_menu_set_load_page_event(menu, grind_mode_item, grind_mode_page);

    create_separator(main_page, "Maintenance");
    lv_obj_t* tools_item = create_menu_item(main_page, "Tools");
    lv_menu_set_load_page_event(menu, tools_item, tools_page);

    create_separator(main_page, "Info");
    lv_obj_t* diagnostics_item = create_menu_item(main_page, "Diagnostics");
    lv_menu_set_load_page_event(menu, diagnostics_item, diagnostics_page);

    lv_obj_t* info_item = create_menu_item(main_page, "System Info");
    lv_menu_set_load_page_event(menu, info_item, info_page);

    lv_obj_t* data_item = create_menu_item(main_page, "Logs & Data");
    lv_menu_set_load_page_event(menu, data_item, data_page);

    lv_obj_t* stats_item = create_menu_item(main_page, "Lifetime Stats");
    lv_menu_set_load_page_event(menu, stats_item, stats_page);

    // Set main page as active (menu will be the landing page)
    lv_menu_set_page(menu, main_page);

    // Refresh lifetime/log statistics whenever the Data or Stats page is displayed
    auto changing_page_callback = [](lv_event_t * e) {
        SettingsScreen * self = static_cast<SettingsScreen*>(lv_event_get_user_data(e));
        lv_obj_t * menu = static_cast<lv_obj_t *>(lv_event_get_target(e));
        lv_obj_t * cur = lv_menu_get_cur_main_page(menu);
        if (cur == self->data_page || cur == self->stats_page) {
            self->refresh_statistics();
        }
    };

    lv_obj_add_event_cb(menu, changing_page_callback, LV_EVENT_VALUE_CHANGED, this);

    visible = false;
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
}

void SettingsScreen::create_info_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 0, 0);
    
    // Enable vertical scrolling for the info page content
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    char build_info[64];
    snprintf(build_info, sizeof(build_info), "#%d", BUILD_NUMBER);

    create_description_label(parent, "Device metrics and current status snapshot.");

    create_static_data_label(parent, "Firmware:", "v" BUILD_FIRMWARE_VERSION);
    create_static_data_label(parent, "Build:", build_info);    
    
    create_separator(parent);
    
    create_data_label(parent, "Instant:", &instant_label);
    create_data_label(parent, "Samples:", &samples_label);
    create_data_label(parent, "Raw:", &raw_label);

    create_separator(parent);
   
    create_data_label(parent, "Uptime:", &uptime_label);
    create_data_label(parent, "RAM:", &memory_label);
}


void SettingsScreen::create_bluetooth_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Enable vertical scrolling on the settings page
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_toggle_row(parent, "Enabled", &ble_toggle);
    create_toggle_row(parent, "Startup", &ble_startup_toggle);

    // BLE Status label
    ble_status_label = lv_label_create(parent);
    lv_label_set_text(ble_status_label, "Bluetooth: Disabled");
    lv_obj_set_style_text_font(ble_status_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(ble_status_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_clear_flag(ble_status_label, LV_OBJ_FLAG_SCROLLABLE);

    // BLE Timer label
    ble_timer_label = lv_label_create(parent);
    lv_label_set_text(ble_timer_label, "");
    lv_obj_set_style_text_font(ble_timer_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(ble_timer_label, lv_color_hex(THEME_COLOR_WARNING), 0);
    lv_obj_clear_flag(ble_timer_label, LV_OBJ_FLAG_SCROLLABLE);
}

void SettingsScreen::create_display_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // Enable vertical scrolling on the settings page
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_slider_row(parent, "Brightness", &brightness_normal_label, &brightness_normal_slider);
    create_slider_row(parent, "Screensaver", &brightness_screensaver_label, &brightness_screensaver_slider, lv_color_hex(THEME_COLOR_WARNING));
}


// Callback for grind mode radio button selection
static void grind_mode_callback(int selected_index, void* user_data) {
    // Trigger the event system instead of handling directly
    EventBridgeLVGL::handle_event(EventBridgeLVGL::EventType::GRIND_MODE_RADIO_BUTTON, nullptr);
}

void SettingsScreen::create_grind_mode_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Enable vertical scrolling on the grind mode page
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    // Mode Selection separator/label
    create_separator(parent, "Mode Selection");

    // Radio button group for grind mode selection at top
    const char* grind_modes[] = {"Weight", "Time"};
    grind_mode_radio_group = create_radio_button_group(
        parent,
        grind_modes,
        2,
        LV_FLEX_FLOW_ROW,
        0,  // Weight initially selected
        135, 100,  // Width, Height
        grind_mode_callback,
        this
    );

    // Descriptive label for swipe functionality
    lv_obj_t* swipe_desc_label = lv_label_create(parent);
    lv_label_set_text(swipe_desc_label, "Enable swiping vertically to switch between Weight/Time modes");
    lv_obj_set_style_text_font(swipe_desc_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(swipe_desc_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_margin_bottom(swipe_desc_label, 10, 0);
    lv_label_set_long_mode(swipe_desc_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(swipe_desc_label, 260);

    // Swipe toggle using existing pattern
    create_toggle_row(parent, "Swipe", &grind_mode_swipe_toggle);

    // Automatic actions section
    create_separator(parent, "Automation");
    create_description_label(parent, "Start the selected profile as soon as the cup lands on the scale.");
    create_toggle_row(parent, "Start", &auto_start_toggle);
    create_description_label(parent, "Exit the completion screen once that cup weight drops away.");
    create_toggle_row(parent, "Return", &auto_return_toggle);
}

void SettingsScreen::create_tools_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 20, 0);
    
    // Disable scrolling on the page container itself
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF);

    tare_button = create_button(parent, "Tare Scale");
    cal_button = create_button(parent, "Calibrate");
    autotune_button = create_button(parent, "Tune Pulses");
    motor_test_button = create_button(parent, "Motor Test");
}

void SettingsScreen::create_data_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Enable vertical scrolling on the reset page
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_description_label(parent, "Saved grind logs stored on the grinder for export/analysis.");

    create_toggle_row(parent, "Logging", &logging_toggle);

    // Log data section
    create_separator(parent, "Log Data");
    create_data_label(parent, "Sessions:", &sessions_label);
    create_data_label(parent, "Events:", &events_label);
    create_data_label(parent, "Metrics:", &measurements_label);

    // Reset separator
    create_separator(parent, "Reset");

    purge_button = create_button(parent, "Purge Logs", lv_color_hex(THEME_COLOR_WARNING));
    lv_obj_set_style_margin_bottom(purge_button, 10, 0);
    reset_button = create_button(parent, "Factory Reset", lv_color_hex(THEME_COLOR_ERROR));
}

void SettingsScreen::create_stats_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_description_label(parent, "Lifetime totals for the grinder.");

    create_separator(parent, "Lifetime Statistics");
    create_data_label(parent, "Total Grinds:", &stat_total_grinds_label, true);
    create_data_label(parent, "Shots (S/D/C):", &stat_shots_label, true);
    create_data_label(parent, "Motor Runtime:", &stat_motor_runtime_label, true);
    create_data_label(parent, "Device Uptime:", &stat_device_uptime_label, true);
    create_data_label(parent, "Total Weight:", &stat_total_weight_label, true);
    create_data_label(parent, "Mode (W/T):", &stat_mode_grinds_label, true);
    create_data_label(parent, "Avg Accuracy:", &stat_avg_accuracy_label, true);
    create_data_label(parent, "Total Pulses:", &stat_total_pulses_label, true);

    refresh_stats_button = create_button(parent, "Refresh Stats");
    lv_obj_set_style_margin_top(refresh_stats_button, 10, 0);
}

void SettingsScreen::create_diagnostics_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 0, 0);

    // Enable vertical scrolling
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    // Load Cell Status separator
    create_separator(parent, "Load Cell Status");

    // Status indicator
    create_data_label(parent, "Status:", &diag_status_label);

    // Calibration factor - stacked for long decimal values
    create_data_label(parent, "Cal. factor:", &diag_calibration_factor_label);

    // Info label (only shown when not calibrated)
    diag_info_label = lv_label_create(parent);
    lv_label_set_text(diag_info_label, "");
    lv_obj_set_style_text_font(diag_info_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(diag_info_label, lv_color_hex(THEME_COLOR_WARNING), 0);
    lv_obj_set_style_margin_top(diag_info_label, 10, 0);
    lv_obj_set_style_margin_bottom(diag_info_label, 10, 0);
    lv_label_set_long_mode(diag_info_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(diag_info_label, 260);
    lv_obj_add_flag(diag_info_label, LV_OBJ_FLAG_HIDDEN); // Hidden by default

    diag_reset_button = create_button(parent, "Reset Diagnostics", lv_color_hex(THEME_COLOR_WARNING));
    lv_obj_set_style_margin_bottom(diag_reset_button, 10, 0);

    // Noise Floor separator
    create_separator(parent, "Noise Floor");

    // Std dev readings (moved from System Info) - stacked for precision values
    create_data_label(parent, "Std Dev (g):", &diag_std_dev_g_label);
    create_data_label(parent, "Std Dev (ADC):", &diag_std_dev_adc_label);
    create_data_label(parent, "Noise level:", &diag_noise_level_label);

    // Static info label about calibration dependency
    lv_obj_t* cal_info = lv_label_create(parent);
    lv_label_set_text(cal_info, "Noise level readings depend on proper calibration.");
    lv_obj_set_style_text_font(cal_info, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(cal_info, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_margin_top(cal_info, 10, 0);
    lv_label_set_long_mode(cal_info, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(cal_info, 260);

    // Motor Response separator
    create_separator(parent, "Motor Response");

    // Motor latency
    create_data_label(parent, "Motor Latency:", &diag_motor_latency_label, true);
}

void SettingsScreen::show() {
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = true;
    update_ble_status();
    update_brightness_sliders();
    update_bluetooth_startup_toggle();
    update_logging_toggle();
    update_grind_mode_toggles();
}

void SettingsScreen::hide() {
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = false;
}

void SettingsScreen::update_info(const WeightSensor* weight_sensor, unsigned long uptime_ms, size_t free_heap) {
    if (!visible) return;

    set_label_text_float(instant_label, weight_sensor->get_instant_weight(), "g");
    set_label_text_int(samples_label, weight_sensor->get_sample_count());
    set_label_text_int(raw_label, weight_sensor->get_raw_adc_instant());

    // Update uptime - use compact format to avoid horizontal scrolling
    unsigned long seconds = uptime_ms / 1000;
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;

    char uptime_text[48];
    snprintf(uptime_text, sizeof(uptime_text), "%02lu:%02lu:%02lu", hours, minutes, seconds);
    lv_label_set_text(uptime_label, uptime_text);

    set_label_text_int(memory_label, free_heap / 1024, "kB");
}

void SettingsScreen::update_diagnostics(WeightSensor* weight_sensor) {
    if (!visible || !diagnostics_controller) return;

    // Update standard deviations only every 1 second to reduce noise
    static unsigned long last_std_dev_update = 0;
    unsigned long now = millis();
    if (now - last_std_dev_update >= 1000) {  // Update every 1 second
        last_std_dev_update = now;

        // Get standard deviations using same window as grind control precision settling
        float std_dev_g = weight_sensor->get_standard_deviation_g(GRIND_SCALE_PRECISION_SETTLING_TIME_MS);  // 500ms window
        int32_t std_dev_adc = weight_sensor->get_standard_deviation_adc(GRIND_SCALE_PRECISION_SETTLING_TIME_MS);  // 500ms window

        char std_dev_g_text[32];
        snprintf(std_dev_g_text, sizeof(std_dev_g_text), "%.4f", std_dev_g);
        lv_label_set_text(diag_std_dev_g_label, std_dev_g_text);

        set_label_text_int(diag_std_dev_adc_label, std_dev_adc);

        // Check noise level using WeightSensor diagnostic method
        bool noise_acceptable = weight_sensor->noise_level_diagnostic();

        // Update noise level indicator
        if (noise_acceptable) {
            lv_label_set_text(diag_noise_level_label, "OK");
            lv_obj_set_style_text_color(diag_noise_level_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
        } else {
            lv_label_set_text(diag_noise_level_label, "Too High");
            lv_obj_set_style_text_color(diag_noise_level_label, lv_color_hex(THEME_COLOR_ERROR), 0);
        }
    }

    // Update calibration factor
    float cal_factor = weight_sensor->get_calibration_factor();
    char cal_factor_text[32];
    snprintf(cal_factor_text, sizeof(cal_factor_text), "%.2f", cal_factor);
    lv_label_set_text(diag_calibration_factor_label, cal_factor_text);

    // Update motor latency
    if (grind_controller) {
        float motor_latency = grind_controller->get_motor_response_latency();
        char latency_text[32];
        snprintf(latency_text, sizeof(latency_text), "%.0f ms", motor_latency);
        lv_label_set_text(diag_motor_latency_label, latency_text);
    } else {
        lv_label_set_text(diag_motor_latency_label, "-- ms");
    }

    // Get highest priority diagnostic
    DiagnosticCode diagnostic = diagnostics_controller->get_highest_priority_warning();

    // Update status label
    if (diagnostic == DiagnosticCode::NONE) {
        lv_label_set_text(diag_status_label, "OK");
        lv_obj_set_style_text_color(diag_status_label, lv_color_hex(THEME_COLOR_SUCCESS), 0);
        lv_obj_add_flag(diag_info_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_label_set_text(diag_status_label, LV_SYMBOL_WARNING " Warning");
        lv_obj_set_style_text_color(diag_status_label, lv_color_hex(THEME_COLOR_WARNING), 0);

        // Show appropriate warning message
        if (diagnostic == DiagnosticCode::LOAD_CELL_NOT_CALIBRATED) {
            lv_label_set_text(diag_info_label, "Loadcell not calibrated");
            lv_obj_clear_flag(diag_info_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            // For future noise/mechanical warnings, show in info label
            const char* message = diagnostics_controller->get_diagnostic_message(diagnostic);
            lv_label_set_text(diag_info_label, message);
            lv_obj_clear_flag(diag_info_label, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void SettingsScreen::update_ble_status() {
    if (!visible || !bluetooth_manager) return;
    
    // Update toggle state
    if (bluetooth_manager->is_enabled()) {
        lv_obj_add_state(ble_toggle, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(ble_toggle, LV_STATE_CHECKED);
    }
    
    // Update status text
    if (bluetooth_manager->is_enabled()) {
        if (bluetooth_manager->is_connected()) {
            lv_label_set_text(ble_status_label, "Connected");
        } else {
            lv_label_set_text(ble_status_label, "Advertising");
        }
        lv_obj_clear_flag(ble_status_label, LV_OBJ_FLAG_HIDDEN);
        
        // Show remaining time
        unsigned long remaining_ms = bluetooth_manager->get_bluetooth_timeout_remaining_ms();
        unsigned long remaining_min = remaining_ms / (60 * 1000);
        char timer_text[64];
        snprintf(timer_text, sizeof(timer_text), "Auto-disable in: %lu min", remaining_min);
        lv_label_set_text(ble_timer_label, timer_text);
        lv_obj_clear_flag(ble_timer_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        // When nothing to display hide the status labels
        lv_obj_add_flag(ble_status_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ble_timer_label, LV_OBJ_FLAG_HIDDEN);
    }
}

void SettingsScreen::refresh_statistics(bool show_overlay) {
    if (!visible) return;

    // Define the statistics loading operation
    auto load_statistics_operation = [this]() {
        // Log data
        set_label_text_int(sessions_label, grind_logger.get_total_flash_sessions());
        set_label_text_int(events_label, grind_logger.count_total_events_in_flash());
        set_label_text_int(measurements_label, grind_logger.count_total_measurements_in_flash());

        // Lifetime statistics
        set_label_text_int(stat_total_grinds_label, statistics_manager.get_total_grinds());

        // Shot type breakdown (Single/Double/Custom)
        char shot_text[32];
        snprintf(shot_text, sizeof(shot_text), "%lu / %lu / %lu",
                 statistics_manager.get_single_shots(),
                 statistics_manager.get_double_shots(),
                 statistics_manager.get_custom_shots());
        lv_label_set_text(stat_shots_label, shot_text);

        // Motor runtime (convert seconds to hours:minutes)
        uint64_t runtime_ms = statistics_manager.get_motor_runtime_ms();
        uint32_t runtime_sec = static_cast<uint32_t>(runtime_ms / 1000ULL);
        uint32_t hours = runtime_sec / 3600;
        uint32_t minutes = (runtime_sec % 3600) / 60;
        uint32_t seconds = runtime_sec % 60;
        char runtime_text[32];
        if (runtime_sec >= 3600) {
            snprintf(runtime_text, sizeof(runtime_text), "%luh %lum", hours, minutes);
        } else if (runtime_sec >= 60) {
            snprintf(runtime_text, sizeof(runtime_text), "%lum %lus", minutes, seconds);
        } else if (runtime_ms >= 1000) {
            float seconds_float = static_cast<float>(runtime_ms) / 1000.0f;
            snprintf(runtime_text, sizeof(runtime_text), "%.1fs", seconds_float);
        } else {
            snprintf(runtime_text, sizeof(runtime_text), "%llums", static_cast<unsigned long long>(runtime_ms));
        }
        lv_label_set_text(stat_motor_runtime_label, runtime_text);

        // Device uptime
        char uptime_text[32];
        uint32_t uptime_hours = statistics_manager.get_device_uptime_hrs();
        uint32_t uptime_minutes = statistics_manager.get_device_uptime_min_remainder();
        snprintf(uptime_text, sizeof(uptime_text), "%luh %lum", uptime_hours, uptime_minutes);
        lv_label_set_text(stat_device_uptime_label, uptime_text);

        // Total weight
        char weight_text[32];
        snprintf(weight_text, sizeof(weight_text), "%.2f kg", statistics_manager.get_total_weight_kg());
        lv_label_set_text(stat_total_weight_label, weight_text);

        // Mode grinds (Weight/Time)
        char mode_text[32];
        snprintf(mode_text, sizeof(mode_text), "%lu / %lu",
                 statistics_manager.get_weight_mode_grinds(),
                 statistics_manager.get_time_mode_grinds());
        lv_label_set_text(stat_mode_grinds_label, mode_text);

        // Average accuracy
        char accuracy_text[32];
        snprintf(accuracy_text, sizeof(accuracy_text), "%.2fg", statistics_manager.get_avg_accuracy_g());
        lv_label_set_text(stat_avg_accuracy_label, accuracy_text);

        // Total pulses (also show average)
        char pulses_text[32];
        snprintf(pulses_text, sizeof(pulses_text), "%lu (avg: %.1f)",
                 statistics_manager.get_total_pulses(),
                 statistics_manager.get_avg_pulses());
        lv_label_set_text(stat_total_pulses_label, pulses_text);
    };

    // Used for when we reload the statistics after a data purge
    if (!show_overlay){
        load_statistics_operation();
        return;
    }

    // Show blocking overlay while loading statistics
    auto& overlay = BlockingOperationOverlay::getInstance();
    overlay.show_and_execute(BlockingOperation::LOADING_STATISTICS, load_statistics_operation);
}

void SettingsScreen::update_brightness_sliders() {
    if (!hardware_manager || !brightness_normal_slider || !brightness_screensaver_slider) return;
    
    // Read from the dedicated "brightness" namespace using a local Preferences
    // instance so we don't interfere with the global shared handle.
    Preferences prefs;
    prefs.begin("brightness", true); // read-only
    
    // Load brightness values from preferences (default to compile-time values)
    float normal_brightness = prefs.getFloat("normal", USER_SCREEN_BRIGHTNESS_NORMAL);
    float screensaver_brightness = prefs.getFloat("screensaver", USER_SCREEN_BRIGHTNESS_DIMMED);
    prefs.end();
    
    // Convert from 0.0-1.0 to 15-100 range
    int normal_percent = (int)(normal_brightness * 100);
    int screensaver_percent = (int)(screensaver_brightness * 100);
    
    // Ensure minimum 15%
    if (normal_percent < HW_DISPLAY_MINIMAL_BRIGHTNESS_PERCENT) normal_percent = HW_DISPLAY_MINIMAL_BRIGHTNESS_PERCENT;
    if (screensaver_percent < HW_DISPLAY_MINIMAL_BRIGHTNESS_PERCENT) screensaver_percent = HW_DISPLAY_MINIMAL_BRIGHTNESS_PERCENT;
    
    // Update sliders
    lv_slider_set_value(brightness_normal_slider, normal_percent, LV_ANIM_OFF);
    lv_slider_set_value(brightness_screensaver_slider, screensaver_percent, LV_ANIM_OFF);
    
    update_brightness_labels(normal_percent, screensaver_percent);
}

void SettingsScreen::update_brightness_labels(int normal_percent, int screensaver_percent) {
    if (brightness_normal_label && normal_percent >= 0) {
        char normal_text[32];
        snprintf(normal_text, sizeof(normal_text), "Brightness: %d%%", normal_percent);
        lv_label_set_text(brightness_normal_label, normal_text);
    }

    if (brightness_screensaver_label && screensaver_percent >= 0) {
        char screensaver_text[32];
        snprintf(screensaver_text, sizeof(screensaver_text), "Dimmed: %d%%", screensaver_percent);
        lv_label_set_text(brightness_screensaver_label, screensaver_text);
    }
}

lv_obj_t* SettingsScreen::create_separator(lv_obj_t* parent, const char* text) {
    // Create separator container
    lv_obj_t* separator_container = lv_obj_create(parent);
    lv_obj_set_size(separator_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(separator_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(separator_container, 0, 0);
    lv_obj_set_layout(separator_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(separator_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(separator_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(separator_container, LV_OBJ_FLAG_SCROLLABLE);

    // Create left line
    lv_obj_t* left_line = lv_obj_create(separator_container);
    lv_obj_set_size(left_line, LV_SIZE_CONTENT, 2);
    lv_obj_set_flex_grow(left_line, 1);
    lv_obj_set_style_bg_color(left_line, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_border_width(left_line, 0, 0);

    if (!text) {
        // If no text, make the line take full width
        return separator_container;
    }

    // Create text label
    lv_obj_t* separator_label = lv_label_create(separator_container);
    lv_label_set_text(separator_label, text);
    lv_obj_set_style_text_font(separator_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(separator_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_pad_left(separator_label, 10, 0);
    lv_obj_set_style_pad_right(separator_label, 10, 0);

    // Create right line
    lv_obj_t* right_line = lv_obj_create(separator_container);
    lv_obj_set_size(right_line, LV_SIZE_CONTENT, 2);
    lv_obj_set_flex_grow(right_line, 1);
    lv_obj_set_style_bg_color(right_line, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_border_width(right_line, 0, 0);

    return separator_container;
}

void SettingsScreen::update_bluetooth_startup_toggle() {
    if (!ble_startup_toggle) return;

    // Read from the "bluetooth" namespace using a local Preferences instance
    Preferences prefs;
    prefs.begin("bluetooth", true); // read-only

    // Load startup value from preferences (default to false)
    bool startup_enabled = prefs.getBool("startup", true);
    prefs.end();

    // Update toggle state
    if (startup_enabled) {
        lv_obj_add_state(ble_startup_toggle, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(ble_startup_toggle, LV_STATE_CHECKED);
    }
}

void SettingsScreen::update_logging_toggle() {
    if (!logging_toggle) return;

    // Read from the "logging" namespace using a local Preferences instance
    Preferences prefs;
    prefs.begin("logging", true); // read-only

    // Load logging enabled value from preferences (default to false)
    bool logging_enabled = prefs.getBool("enabled", false);
    prefs.end();

    // Update toggle state
    if (logging_enabled) {
        lv_obj_add_state(logging_toggle, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(logging_toggle, LV_STATE_CHECKED);
    }
}

lv_obj_t* SettingsScreen::create_menu_item(lv_obj_t* parent, const char* text) {
    lv_obj_t* cont = lv_menu_cont_create(parent);
    style_as_button(cont);
    lv_obj_set_style_margin_bottom(cont, 10, 0);

    // Set layout
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* label = lv_label_create(cont);
    lv_label_set_text(label, text);

    lv_obj_t* chevron = lv_label_create(cont);
    lv_label_set_text(chevron, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(chevron, lv_color_hex(THEME_COLOR_SECONDARY), 0);

    return cont;
}

lv_obj_t* SettingsScreen::create_toggle_row(lv_obj_t* parent, const char* text, lv_obj_t** out_toggle) {
    lv_obj_t* row_container = lv_obj_create(parent);
    style_as_button(row_container);
    lv_obj_set_style_margin_bottom(row_container, 10, 0);

    // Set layout
    lv_obj_set_layout(row_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* label = lv_label_create(row_container);
    lv_label_set_text(label, text);

    *out_toggle = lv_switch_create(row_container);
    lv_obj_set_size(*out_toggle, 80, 40);
    lv_obj_set_ext_click_area(*out_toggle, 20);
    
    return row_container;
}


lv_obj_t* SettingsScreen::create_slider_row(lv_obj_t* parent, const char* text, lv_obj_t** label, lv_obj_t** slider, lv_color_t slider_color, uint32_t min, uint32_t max) {
    lv_obj_t* row_container = lv_obj_create(parent);
    style_as_button(row_container, 260, LV_SIZE_CONTENT);
    lv_obj_set_style_margin_bottom(row_container, 10, 0);
    // Set layout

    lv_obj_set_layout(row_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row_container, 14, 0);
    lv_obj_set_style_pad_all(row_container, 20, 0);

    *label = lv_label_create(row_container);
    lv_label_set_text(*label, text);

    *slider = lv_slider_create(row_container);
    lv_obj_set_size(*slider, 220, 40);
    lv_obj_set_ext_click_area(*slider, 20);
    lv_slider_set_range(*slider, min, max);
    lv_obj_set_style_bg_color(*slider, lv_color_hex(THEME_COLOR_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_bg_color(*slider, slider_color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(*slider, LV_OPA_TRANSP, LV_PART_KNOB);
    return row_container;
}

lv_obj_t* SettingsScreen::create_static_data_label(lv_obj_t* parent, const char* name, const char* value) {
    lv_obj_t* label;
    lv_obj_t* data_label = create_data_label(parent, name, &label);
    lv_label_set_text(label, value);
    return data_label;
}

lv_obj_t* SettingsScreen::create_data_label(lv_obj_t* parent, const char* name, lv_obj_t** variable, bool stacked) {
    return ::create_data_label(parent, name, variable, stacked);
}

lv_obj_t* SettingsScreen::create_description_label(lv_obj_t* parent, const char* text) {
    // Create container with padding (similar to create_data_label)
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_style_pad_left(container, 10, 0);
    lv_obj_set_style_pad_right(container, 14, 0);
    lv_obj_set_style_margin_top(container, 12, 0);
    lv_obj_set_style_margin_bottom(container, 12, 0);
    lv_obj_set_size(container, 280, LV_SIZE_CONTENT);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Create label inside container
    lv_obj_t* label = lv_label_create(container);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(100));

    return label;
}

void SettingsScreen::update_grind_mode_toggles() {
    // Read swipe enabled from "swipe" namespace
    Preferences swipe_prefs;
    swipe_prefs.begin("swipe", true); // read-only
    bool swipe_enabled = swipe_prefs.getBool("enabled", false);
    swipe_prefs.end();

    // Read current grind mode from main grinder preferences using hardware manager
    int mode_index = 0; // Default to Weight (index 0)
    if (hardware_manager) {
        Preferences* main_prefs = hardware_manager->get_preferences();
        int stored_mode = main_prefs->getInt("grind_mode", static_cast<int>(GrindMode::WEIGHT));
        mode_index = (stored_mode == static_cast<int>(GrindMode::TIME)) ? 1 : 0;
    }

    if (grind_mode_radio_group) {
        radio_button_group_set_selection(grind_mode_radio_group, mode_index);
    }

    if (grind_mode_swipe_toggle) {
        if (swipe_enabled) {
            lv_obj_add_state(grind_mode_swipe_toggle, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(grind_mode_swipe_toggle, LV_STATE_CHECKED);
        }
    }

    // Auto actions toggles (defaults disabled)
    Preferences auto_prefs;
    auto_prefs.begin("autogrind", true);
    bool auto_start_enabled = auto_prefs.getBool("auto_start", false);
    bool auto_return_enabled = auto_prefs.getBool("auto_return", false);
    auto_prefs.end();

    if (auto_start_toggle) {
        if (auto_start_enabled) {
            lv_obj_add_state(auto_start_toggle, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(auto_start_toggle, LV_STATE_CHECKED);
        }
    }

    if (auto_return_toggle) {
        if (auto_return_enabled) {
            lv_obj_add_state(auto_return_toggle, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(auto_return_toggle, LV_STATE_CHECKED);
        }
    }
}
