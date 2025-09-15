#pragma once

//==============================================================================
// DEPRECATED PINS CONFIGURATION FILE
//==============================================================================
// This file has been moved to src/config/hardware_config.h
// Include the new location for backward compatibility
//
// All pin assignments are now organized in hardware_config.h with consistent naming:
// - HW_TOUCH_I2C_SDA_PIN, HW_TOUCH_I2C_SCL_PIN
// - HW_DISPLAY_CS_PIN, HW_DISPLAY_SCK_PIN, etc.
// - HW_LOADCELL_DOUT_PIN, HW_LOADCELL_SCK_PIN
// - HW_MOTOR_RELAY_PIN
//
// TODO: Update includes to use "config/hardware_config.h" instead

#include "hardware_config.h"

// Legacy pin name mappings for backward compatibility
#define TOUCH_SDA_PIN HW_TOUCH_I2C_SDA_PIN
#define TOUCH_SCL_PIN HW_TOUCH_I2C_SCL_PIN
#define FT3168_ADDR HW_TOUCH_I2C_ADDRESS

#define DISPLAY_CS_PIN HW_DISPLAY_CS_PIN
#define DISPLAY_SCK_PIN HW_DISPLAY_SCK_PIN
#define DISPLAY_D0_PIN HW_DISPLAY_D0_PIN
#define DISPLAY_D1_PIN HW_DISPLAY_D1_PIN
#define DISPLAY_D2_PIN HW_DISPLAY_D2_PIN
#define DISPLAY_D3_PIN HW_DISPLAY_D3_PIN
#define DISPLAY_RST_PIN HW_DISPLAY_RESET_PIN

#define LOAD_CELL_DOUT_PIN HW_LOADCELL_DOUT_PIN
#define LOAD_CELL_SCK_PIN HW_LOADCELL_SCK_PIN

#define MOTOR_PIN HW_MOTOR_RELAY_PIN