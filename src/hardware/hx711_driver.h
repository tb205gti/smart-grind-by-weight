#pragma once

#include "../config/hardware_config.h"
#include <Arduino.h>
#include <math.h>

/**
 * HX711 ADC Driver Implementation
 * 
 * Hardware-specific driver for HX711 24-bit ADC used with load cells.
 * Provides direct HX711 communication without a hardware abstraction layer.
 * 
 * HX711 hardware communication algorithms based on:
 * HX711_ADC library by Olav Kallhovd (olkal)
 * https://github.com/olkal/HX711_ADC
 * 
 * Original HX711_ADC library:
 * Copyright (c) 2018 Olav Kallhovd
 * Licensed under MIT License
 */
class HX711Driver {
private:
    // HX711 Hardware pins and configuration
    uint8_t sck_pin;
    uint8_t dout_pin;
    uint8_t gain;
    
    // HX711 hardware state
    int32_t last_raw_data;
    bool data_ready_flag;
    unsigned long conversion_start_time;
    unsigned long conversion_time;
    
    // HX711-specific timing constants
    static const uint8_t SCK_DELAY = 1;           // Microsecond delay after SCK toggle
    static const uint16_t SIGNAL_TIMEOUT = 100;  // Signal timeout in ms
    
    // HX711 hardware methods
    void conversion_24bit();
    void power_up_sequence();
    void power_down_sequence();
    
public:
    HX711Driver(uint8_t sck_pin = HW_LOADCELL_SCK_PIN, uint8_t dout_pin = HW_LOADCELL_DOUT_PIN);
    virtual ~HX711Driver() = default;
    
    // Initialization and configuration
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
    
    // HX711-specific capabilities
    bool supports_temperature_sensor() const { return false; }
    float get_temperature() const { return NAN; }
    uint32_t get_max_sample_rate() const { return HW_LOADCELL_SAMPLE_RATE_SPS; }
    
    uint8_t get_current_gain() const;
    const char* get_driver_name() const { return "HX711"; }
};
