#pragma once

#include "../config/user_config.h" // For USER_GRINDER_REFERENCE_FLOW_RATE_GPS
#include <Arduino.h>
#include <math.h>

class Grinder; // Forward declaration
class UIManager; // Forward declaration

/**
 * Mock HX711 Driver for Testing Without Hardware
 * 
 * Implements the same interface as HX711Driver but generates synthetic
 * weight sweep data for testing the UI and grind control systems
 * without requiring actual HX711 hardware.
 * 
 * Features:
 * - Simulates a stable weight with a low noise floor.
 * - When the grinder is active, simulates weight addition at a configurable flow rate.
 * - Increased noise during grinding to mimic motor vibrations.
 * - Same API as real HX711Driver for drop-in compatibility
 */
class HX711MockDriver {
private:
    // Mock hardware state
    int32_t last_raw_data;
    uint32_t mock_data_ready_at_ms; // Timestamp when the next sample is "ready"
    
    // Mock grinding simulation parameters
    float current_weight_g;
    bool grinder_active;
    float flow_rate_gps;
    uint32_t last_grind_update_time_ms;
    
    // Coast and latency simulation
    bool was_grinding_last_update;
    uint32_t grind_start_time_ms;
    uint32_t grind_stop_time_ms;
    
    // Pulse simulation state
    bool pulse_active;
    uint32_t pulse_start_time_ms;
    uint32_t pulse_duration_ms;
    float pulse_total_weight_g;
    float pulse_baseline_weight_g;  // Weight at start of pulse for incremental addition
    
    // Sample rate simulation (HX711 is 10 SPS, not continuous)
    uint32_t last_sample_time_ms;
    
    // Mock calibration (mirrors real hardware defaults)
    float mock_cal_factor;
    int32_t mock_tare_offset; // This is the raw ADC value for 0g
    
    // Grinder reference for polling state
    Grinder* grinder_ref;
    
    // Mock hardware methods
    int32_t generate_mock_raw_reading();
    
public:
    HX711MockDriver(uint8_t sck_pin = 0, uint8_t dout_pin = 0);
    virtual ~HX711MockDriver() = default;
    
    // Initialization and configuration (same interface as HX711Driver)
    bool begin();
    bool begin(uint8_t gain_value);
    void set_gain(uint8_t gain_value);
    
    void power_up();
    void power_down();
    
    bool is_ready();
    bool data_waiting_async();
    bool update_async();
    int32_t get_raw_data() const;
    
    bool validate_hardware();
    
    // HX711-specific capabilities (same as real driver)
    bool supports_temperature_sensor() const;
    float get_temperature() const;
    uint32_t get_max_sample_rate() const;
    
    uint8_t get_current_gain() const;
    const char* get_driver_name() const { return "HX711 (Mock)"; }
    
    // Mock-specific configuration methods
    void set_grinder_reference(Grinder* grinder);
    void set_grinder_active(bool active);
    void simulate_pulse(uint32_t duration_ms);
    void set_mock_calibration(float cal_factor, int32_t tare_offset = 0);
    void reset();
};