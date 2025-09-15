#include "hx711_mock_driver.h"
#include "../config/logging.h"
#include "../config/constants.h"
#include "../config/hardware_config.h"
#include "../config/theme_config.h"
#include "../ui/ui_manager.h"
#include "grinder.h"
#include <lvgl.h>

// Default mock parameters
#define MOCK_DEFAULT_CAL_FACTOR -7050.0f  // Negative factor to match real hardware behavior
#define MOCK_STABLE_NOISE_G 0.005f
#define MOCK_GRINDING_NOISE_G 0.04f
#define MOCK_PULSE_NOISE_G 0.10f          // Higher noise during pulses
#define MOCK_DEFAULT_FLOW_RATE_GPS 1.5f   // Default simulated flow rate
#define MOCK_GRIND_LATENCY_MS 500         // Latency before grinding starts adding weight
#define MOCK_COAST_DURATION_MS 400        // Coast after grinding stops

HX711MockDriver::HX711MockDriver(uint8_t sck_pin, uint8_t dout_pin) {
    // Initialize mock hardware state
    last_raw_data = 0;
    mock_data_ready_at_ms = 0;
    
    // Initialize mock grinding simulation
    current_weight_g = 0.0f;
    grinder_active = false;
    flow_rate_gps = MOCK_DEFAULT_FLOW_RATE_GPS; // Use 1.5g/s from local define
    last_grind_update_time_ms = 0;
    
    // Initialize coast and latency tracking
    was_grinding_last_update = false;
    grind_start_time_ms = 0;
    grind_stop_time_ms = 0;
    
    // Initialize pulse simulation
    pulse_active = false;
    pulse_start_time_ms = 0;
    pulse_duration_ms = 0;
    pulse_total_weight_g = 0.0f;
    pulse_baseline_weight_g = 0.0f;
    
    // Initialize sample rate simulation
    last_sample_time_ms = millis();
    
    // Initialize mock calibration to match real HX711 behavior
    // Real HX711: weight = (raw - tare) / cal_factor
    // For mock: We want raw values that will give 0g weight after tare
    // So when weight=0: raw = tare_offset + (0 * cal_factor) = tare_offset.
    mock_cal_factor = MOCK_DEFAULT_CAL_FACTOR;
    mock_tare_offset = 0x800000;  // Middle of 24-bit range - this will be the "0g" point
    
    // Initialize grinder reference
    grinder_ref = nullptr;
}

bool HX711MockDriver::begin() {
    return begin(128);  // Default gain
}

bool HX711MockDriver::begin(uint8_t gain_value) {
    set_gain(gain_value);
    
    // Initialize mock state
    mock_data_ready_at_ms = millis(); // First sample is ready immediately
    
    BLE_LOG("Mock HX711 driver initialized - simulating grinding\n");
    BLE_LOG("  Simulated flow rate: %.2fg/s\n", flow_rate_gps);
    
    return true;
}

void HX711MockDriver::set_gain(uint8_t gain_value) {
    // Mock driver ignores gain setting but accepts it for API compatibility
}

void HX711MockDriver::power_up() {
    // Mock power up - instant
    mock_data_ready_at_ms = millis();
}

void HX711MockDriver::power_down() {
    // Mock power down
    mock_data_ready_at_ms = 0; // No data will be ready
}

bool HX711MockDriver::is_ready() {
    // Simulate the DOUT pin going LOW when data is ready.
    // Data is ready if the current time is past the next scheduled sample time.
    // A value of 0 means it's powered down.
    return mock_data_ready_at_ms != 0 && millis() >= mock_data_ready_at_ms;
}

bool HX711MockDriver::data_waiting_async() {
    // This is the same as is_ready() for both real and mock drivers.
    return is_ready();
}

bool HX711MockDriver::update_async() {
    if (!is_ready()) {
        return false;
    }
    
    // A read has occurred. Generate the data for this read.
    last_raw_data = generate_mock_raw_reading();
    
    // Schedule the next data ready time. This simulates the HX711 starting a new
    // conversion after the data has been read.
    // The real hardware starts conversion after the 25th SCK pulse, but simulating it
    // after the read is complete is a valid and simpler model.
    mock_data_ready_at_ms = millis() + HW_LOADCELL_SAMPLE_INTERVAL_MS;
    
    return true;
}

int32_t HX711MockDriver::get_raw_data() const {
    return last_raw_data;
}

bool HX711MockDriver::validate_hardware() {
    // Mock hardware always validates successfully
    BLE_LOG("Mock HX711 validation: SUCCESS (simulated)\n");
    return true;
}

uint8_t HX711MockDriver::get_current_gain() const {
    return 128;  // Mock returns default gain
}

bool HX711MockDriver::supports_temperature_sensor() const {
    return false;
}

float HX711MockDriver::get_temperature() const {
    return NAN;
}

uint32_t HX711MockDriver::get_max_sample_rate() const {
    return HW_LOADCELL_SAMPLE_RATE_SPS;
}

int32_t HX711MockDriver::generate_mock_raw_reading() {
    uint32_t current_time = millis();
    float final_weight_g;
    float noise_g;
    
    // Poll grinder state if reference is available, otherwise fall back to internal flag
    bool current_grinding = (grinder_ref && grinder_ref->is_grinding()) || grinder_active;
    
    // Track grinding state changes for latency and coast simulation
    if (current_grinding && !was_grinding_last_update) {
        // Grinding just started
        grind_start_time_ms = current_time;
    } else if (!current_grinding && was_grinding_last_update) {
        // Grinding just stopped
        grind_stop_time_ms = current_time;
    }
    was_grinding_last_update = current_grinding;
    
    // Check pulse simulation first (overrides normal grinding behavior)
    if (pulse_active) {
        uint32_t pulse_elapsed_ms = current_time - pulse_start_time_ms;
        uint32_t total_pulse_time_ms = MOCK_GRIND_LATENCY_MS + pulse_duration_ms + MOCK_COAST_DURATION_MS;
        
        if (pulse_elapsed_ms >= total_pulse_time_ms) {
            // Pulse complete
            pulse_active = false;
            noise_g = ((float)random(-1, 2) * MOCK_STABLE_NOISE_G);
            BLE_LOG("[MockDriver] Pulse completed\n");
        } else if (pulse_elapsed_ms >= MOCK_GRIND_LATENCY_MS) {
            // Past latency, linearly distribute weight over pulse + coast duration
            uint32_t weight_add_duration_ms = pulse_duration_ms + MOCK_COAST_DURATION_MS;
            uint32_t weight_add_elapsed_ms = pulse_elapsed_ms - MOCK_GRIND_LATENCY_MS;
            
            // Calculate progress (0.0 to 1.0) through the weight addition period
            float progress = (float)weight_add_elapsed_ms / (float)weight_add_duration_ms;
            if (progress > 1.0f) progress = 1.0f;
            
            // Set current weight to baseline + (progress * total weight to add)
            current_weight_g = pulse_baseline_weight_g + (pulse_total_weight_g * progress);
            
            // Use pulse noise during active weight addition
            noise_g = ((float)random(-1, 2) * MOCK_PULSE_NOISE_G);
        } else {
            // Still in latency period
            noise_g = ((float)random(-1, 2) * MOCK_STABLE_NOISE_G);
        }
    } else {
        // Normal grinding/coast logic
        bool should_add_weight = false;
        
        if (current_grinding) {
            // Currently grinding - check if latency period has passed
            if (current_time - grind_start_time_ms >= MOCK_GRIND_LATENCY_MS) {
                should_add_weight = true;
            }
        } else {
            // Not currently grinding - check if still in coast period
            if (grind_stop_time_ms > 0 && (current_time - grind_stop_time_ms) < MOCK_COAST_DURATION_MS) {
                should_add_weight = true;
            }
        }
        
        if (should_add_weight) {
            // If grinder is active, add weight based on flow rate and add larger noise
            if (last_grind_update_time_ms == 0) {
                last_grind_update_time_ms = current_time;
            }
            uint32_t delta_ms = current_time - last_grind_update_time_ms;
            current_weight_g += (delta_ms / 1000.0f) * flow_rate_gps;
            last_grind_update_time_ms = current_time;

            // Add grinding noise using the defined constant
            noise_g = ((float)random(-1, 2) * MOCK_GRINDING_NOISE_G);
        } else {
            // Grinder is not active, just add stable noise
            last_grind_update_time_ms = 0; // Reset grind timer

            // Add stable noise using the defined constant
            noise_g = ((float)random(-1, 2) * MOCK_STABLE_NOISE_G);
        }
    }

    final_weight_g = current_weight_g + noise_g;
    
    // Convert weight to raw ADC value using exact HX711 inverse math
    // Real HX711: weight = (raw - tare) / cal_factor
    // Inverse: raw = (weight * cal_factor) + tare
    int32_t raw_value = mock_tare_offset + (int32_t)(final_weight_g * mock_cal_factor);
    
    // Clamp to valid 24-bit signed range that becomes unsigned after XOR
    if (raw_value < 0) raw_value = 0;
    if (raw_value > 0xFFFFFF) raw_value = 0xFFFFFF;
    
    return raw_value;
}


// Mock-specific configuration methods
void HX711MockDriver::set_grinder_reference(Grinder* grinder) {
    grinder_ref = grinder;
}


void HX711MockDriver::set_grinder_active(bool active) {
    grinder_active = active;
    // Reset timer when state changes to ensure correct delta calculation
    last_grind_update_time_ms = active ? millis() : 0;
}

void HX711MockDriver::simulate_pulse(uint32_t duration_ms) {
    // Calculate total weight to add based on pulse duration and flow rate
    pulse_total_weight_g = (duration_ms / 1000.0f) * flow_rate_gps;
    pulse_baseline_weight_g = current_weight_g; // Remember starting weight
    
    // Set up pulse tracking
    pulse_active = true;
    pulse_start_time_ms = millis();
    pulse_duration_ms = duration_ms;
    
    BLE_LOG("[MockDriver] Pulse started: %lums -> %.3fg total (from %.3fg, distributed over %lums latency + %lums pulse + %lums coast)\n", 
            duration_ms, pulse_total_weight_g, pulse_baseline_weight_g, (uint32_t)MOCK_GRIND_LATENCY_MS, duration_ms, (uint32_t)MOCK_COAST_DURATION_MS);
} 

void HX711MockDriver::set_mock_calibration(float cal_factor, int32_t tare_offset) {
    mock_cal_factor = cal_factor;  // Keep original sign to match real HX711
    mock_tare_offset = tare_offset;
    BLE_LOG("Mock HX711: Calibration set to factor=%.1f, tare=%ld\n", cal_factor, (long)tare_offset);
}

void HX711MockDriver::reset() {
    // Reset mock state to 0g and not grinding
    current_weight_g = 0.0f;
    grinder_active = false;
    last_grind_update_time_ms = 0;
    BLE_LOG("Mock HX711: Reset to 0.0g\n");
}
