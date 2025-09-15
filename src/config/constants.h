#pragma once

//==============================================================================
// MAIN CONFIGURATION FILE
//==============================================================================
// This file includes all the organized configuration files for the ESP32
// coffee grinder system. After the migration, this provides clean access
// to all configuration constants through their new organized structure.
//
// Configuration structure:
// - user_config.h: User-tunable parameters (USER_ prefix)
// - hardware_config.h: Hardware-specific constants (HW_ prefix)  
// - system_config.h: Core system constants (SYS_ prefix)
// - bluetooth_config.h: BLE configuration (BLE_ prefix)
// - theme_config.h: UI styling constants (THEME_ prefix)
// - internal_config.h: System-internal constants (INTERNAL_ prefix)

#include "user_config.h"
#include "hardware_config.h"
#include "system_config.h"
#include "bluetooth_config.h"
#include "theme_config.h"
#include "internal_config.h"