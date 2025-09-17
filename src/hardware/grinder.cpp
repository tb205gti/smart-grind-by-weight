#include "grinder.h"
#include "../controllers/grind_events.h"
#include "../config/logging.h"
#include "../config/hardware_config.h"

void Grinder::init(int pin) {
    motor_pin = pin;
    grinding = false;
    pulse_active = false;
    rmt_initialized = false;
    
    // Initialize background indicator
    background_active = false;
    ui_event_callback = nullptr;
    
    // Initialize RMT for all motor control (Arduino HAL)
    if (rmtInit(motor_pin, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 1000000)) { // 1MHz = 1µs tick
        rmt_initialized = true;
        initialized = true;
    }
}

void Grinder::start() {
    if (!initialized || !rmt_initialized) return;
    
    // Safety: Never activate motor when mock HX711 driver is active
    #if HW_LOADCELL_USE_MOCK_DRIVER
    BLE_LOG("[SAFETY] Motor activation blocked - mock HX711 driver is active\n");
    return;
    #endif
    
    // Reset any active pulse state when using continuous mode
    pulse_active = false;
    
    // Start continuous transmit (HAL)
    rmt_data_t continuous_data[1];
    continuous_data[0].duration0 = 32767; // ~32ms at 1µs tick
    continuous_data[0].level0 = 1;
    continuous_data[0].duration1 = 0;
    continuous_data[0].level1 = 0;
    
    // Loop continuously
    rmtWriteLooping(motor_pin, continuous_data, 1);
    grinding = true;
    emit_background_change(true);
}

void Grinder::stop() {
    if (!initialized || !rmt_initialized) return;
    
    // Stop RMT transmission (HAL)
    // Deinitialize to stop loop, then reinitialize for next operation
    rmtDeinit(motor_pin);
    if (rmtInit(motor_pin, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 1000000)) { // 1MHz = 1µs tick
        rmt_initialized = true;
    } else {
        rmt_initialized = false;
    }
    
    grinding = false;
    pulse_active = false;
    emit_background_change(false);
}

void Grinder::start_pulse_rmt(uint32_t duration_ms) {
    if (!initialized || !rmt_initialized) return;
    
    // Safety: Never activate motor when mock HX711 driver is active
    #if HW_LOADCELL_USE_MOCK_DRIVER
    BLE_LOG("[SAFETY] Motor pulse activation blocked - mock HX711 driver is active\n");
    return;
    #endif
    
    // HAL path: perform blocking writes in chunks
    uint32_t remaining_us = duration_ms * 1000U;
    const uint32_t max_us = 32767; // with 1µs tick
    pulse_active = true;
    grinding = true;
    emit_background_change(true);
    
    while (remaining_us > 0) {
        uint32_t this_us = remaining_us > max_us ? max_us : remaining_us;
        rmt_data_t sym;
        sym.level0 = 1;
        sym.duration0 = this_us;  // 1 tick = 1µs
        sym.level1 = 0;
        sym.duration1 = 1; // ensure low at end
        rmtWrite(motor_pin, &sym, 1, 60000); // 60s timeout
        remaining_us -= this_us;
    }
    pulse_active = false;
    grinding = false;
    emit_background_change(false);
}

bool Grinder::is_pulse_complete() {
    if (!pulse_active) return true;
    
    // HAL path already completed in start_pulse_rmt
    return true;
    
    return false;
}

void Grinder::set_ui_event_callback(const std::function<void(const GrindEventData&)>& callback) {
    ui_event_callback = callback;
}

void Grinder::emit_background_change(bool active) {
    if (background_active == active) {
        return; // No change
    }
    
    background_active = active;
    
    if (ui_event_callback) {
        // Properly initialize all required fields to prevent null pointer crashes
        GrindEventData event_data = {};
        event_data.event = UIGrindEvent::BACKGROUND_CHANGE;
        event_data.phase = GrindPhase::IDLE;  // Safe default
        event_data.current_weight = 0.0f;
        event_data.progress_percent = 0;
        event_data.phase_display_text = "BACKGROUND";  // Safe string for logging
        event_data.show_taring_text = false;
        event_data.background_active = active;
        
        ui_event_callback(event_data);
        
        BLE_LOG("[Grinder] Background change: %s\n", active ? "ACTIVE" : "INACTIVE");
    }
}
