#pragma once

//==============================================================================
// INTERNAL SYSTEM CONFIGURATION
//==============================================================================
// This file contains internal system configuration constants that are used
// for build information, validation, and system-internal operations. These
// values are typically auto-generated or system-determined and should not
// require user modification.
//
// Categories:
// - Build Information (version strings, compilation timestamps)
// - System Validation Constants (magic numbers, checksums)
// - Internal Buffer Sizes (system-allocated memory pools)
// - Debug and Development Settings (internal logging, test modes)
//
// All constants use the INTERNAL_ prefix to indicate system-internal parameters.
// These values are primarily used for build tracking and system validation.

#include <cstdio>
#include "git_info.h"
#include "user_config.h"

//------------------------------------------------------------------------------
// BUILD INFORMATION
//------------------------------------------------------------------------------
#define INTERNAL_FIRMWARE_VERSION "1.0.0"                                     // Firmware version string
#define INTERNAL_BUILD_DATE __DATE__                                           // Compiler-provided build date
#define INTERNAL_BUILD_TIME __TIME__                                           // Compiler-provided build time

// Build number is auto-generated in git_info.h
// Git commit ID and branch are auto-generated in git_info.h

//------------------------------------------------------------------------------
// SYSTEM IDENTIFIERS
//------------------------------------------------------------------------------
// These are used internally and should not be changed by users
#define INTERNAL_DEVICE_TYPE "ESP32_GRIND_SCALE"                              // Device type identifier
#define INTERNAL_CONFIG_VERSION 1                                             // Configuration format version

//------------------------------------------------------------------------------
// CALCULATED CONSTANTS
//------------------------------------------------------------------------------
// These are derived from other constants and should not be modified directly

// Legacy debug flags that are now controlled by user_config.h
// These maintain backward compatibility for existing code
#ifndef DEBUG_SERIAL_OUTPUT
#define DEBUG_SERIAL_OUTPUT USER_DEBUG_SERIAL_OUTPUT
#endif

#ifndef ENABLE_GRIND_DEBUG
#define ENABLE_GRIND_DEBUG USER_DEBUG_GRIND_CONTROLLER
#endif

#ifndef ENABLE_LOADCELL_DEBUG  
#define ENABLE_LOADCELL_DEBUG USER_DEBUG_LOAD_CELL
#endif

#ifndef ENABLE_UI_DEBUG
#define ENABLE_UI_DEBUG USER_DEBUG_UI_SYSTEM
#endif

#ifndef ENABLE_CALIBRATION_DEBUG
#define ENABLE_CALIBRATION_DEBUG USER_DEBUG_CALIBRATION
#endif

#ifndef ENABLE_SETTLING_DEBUG
#define ENABLE_SETTLING_DEBUG USER_DEBUG_WEIGHT_SETTLING
#endif

#ifndef ENABLE_PERFORMANCE_MONITORING
#define ENABLE_PERFORMANCE_MONITORING USER_ENABLE_PERFORMANCE_MONITORING
#endif

//------------------------------------------------------------------------------
// INLINE UTILITY FUNCTIONS
//------------------------------------------------------------------------------
// Build information accessors (defined in build_info.h)
#include "build_info.h"

inline int get_build_number() {
    return BUILD_NUMBER;
}

inline const char* get_firmware_version() {
    return INTERNAL_FIRMWARE_VERSION;
}

//------------------------------------------------------------------------------
// SYSTEM VALIDATION CONSTANTS
//------------------------------------------------------------------------------
// Used for internal validation and safety checks
#define INTERNAL_MAX_SAFE_WEIGHT_G 1000.0f                                    // Maximum safe weight for sanity checking
#define INTERNAL_MIN_SAFE_WEIGHT_G -100.0f                                    // Minimum safe weight for sanity checking
#define INTERNAL_MAX_REASONABLE_FLOW_RATE_GPS 10.0f                           // Maximum reasonable flow rate for validation
#define INTERNAL_CONFIG_MAGIC_NUMBER 0xC0FFEE                                 // Magic number for config validation