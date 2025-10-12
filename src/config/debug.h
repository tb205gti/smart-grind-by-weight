#pragma once

//==============================================================================
// DEBUG AND DEVELOPMENT CONFIGURATION
//==============================================================================
// This file contains all debug-related configuration constants including
// mock hardware settings, debug output controls, and performance monitoring.

//------------------------------------------------------------------------------
// MOST IMPORTANT DEBUG PARAMETERS
//------------------------------------------------------------------------------
// Mock hardware (most critical for testing vs production)
#ifndef DEBUG_ENABLE_LOADCELL_MOCK
    #define DEBUG_ENABLE_LOADCELL_MOCK 0                                              // Default: use physical HX711, override with build flag
#endif

// UI visual feedback
#ifndef DEBUG_ENABLE_GRINDER_BACKGROUND_INDICATOR
    #define DEBUG_ENABLE_GRINDER_BACKGROUND_INDICATOR 0                             // Default: disabled, override with build flag
#endif

// Core debug logging settings
#define DEBUG_SERIAL_OUTPUT 1                                             // Enable serial debug output
#define DEBUG_GRIND_CONTROLLER 0                                          // Enable detailed grind debugging
#define DEBUG_LOAD_CELL 0                                                 // Enable detailed load cell debugging  
#define DEBUG_UI_SYSTEM 0                                                 // Enable detailed UI debugging
#define DEBUG_CALIBRATION 0                                               // Enable detailed calibration debugging
#define DEBUG_WEIGHT_SETTLING 0                                           // Enable weight settling debugging

// Touch I2C polling creates expected NACKs when the controller has no data.
// Suppress noisy low-level logs while keeping higher-level drivers quiet.
#define DEBUG_SUPPRESS_TOUCH_I2C_ERRORS 1

//------------------------------------------------------------------------------
// MOCK HARDWARE DETAILED CONFIGURATION
//------------------------------------------------------------------------------
#define DEBUG_MOCK_FLOW_RATE_GPS 1.9f                                             // Simulated continuous flow rate in grams per second
#define DEBUG_MOCK_CAL_FACTOR -7050.0f                                            // Fixed calibration factor used during mocking
#define DEBUG_MOCK_BASELINE_RAW 0x700000                                          // Baseline raw count around mid-scale for tare offset
#define DEBUG_MOCK_IDLE_NOISE_RAW 60.0f                                           // Peak raw noise when idle (counts)
#define DEBUG_MOCK_GRIND_NOISE_RAW 400.0f                                         // Peak raw noise when grinding (counts)
#define DEBUG_MOCK_FLOW_RAMP_MS 350                                               // Ramp duration for simulated flow transitions
#define DEBUG_MOCK_START_DELAY_MS 500                                             // Delay from motor start command to weight increase (continuous grind)
#define DEBUG_MOCK_STOP_DELAY_MS 400                                              // Delay from motor stop command to weight stop
#define DEBUG_MOCK_MOTOR_LATENCY_MS 42.0f                                         // Hidden minimum pulse duration to produce grounds (for auto-tune testing)

