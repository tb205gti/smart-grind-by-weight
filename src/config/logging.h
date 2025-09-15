#pragma once

#include "../bluetooth/manager.h"
#include "bluetooth_config.h"

// Forward declaration needed for constants
#ifndef ENABLE_GRIND_DEBUG
#define ENABLE_GRIND_DEBUG 1
#endif
#ifndef ENABLE_LOADCELL_DEBUG  
#define ENABLE_LOADCELL_DEBUG 1
#endif
#ifndef ENABLE_UI_DEBUG
#define ENABLE_UI_DEBUG 1
#endif
#ifndef ENABLE_CALIBRATION_DEBUG
#define ENABLE_CALIBRATION_DEBUG 1
#endif
#ifndef ENABLE_SETTLING_DEBUG
#define ENABLE_SETTLING_DEBUG 1
#endif
#ifndef ENABLE_OTA_DEBUG
#define ENABLE_OTA_DEBUG 1
#endif

// Global bluetooth manager instance (defined in main.cpp)
extern BluetoothManager g_bluetooth_manager;

// Global logging function that uses BLE monitor
#define BLE_LOG(format, ...) g_bluetooth_manager.log(format, ##__VA_ARGS__)

// Replace DEBUG macros to use BLE logging instead of Serial
#if DEBUG_SERIAL_OUTPUT
#define DEBUG_PRINTF(format, ...) g_bluetooth_manager.log(format, ##__VA_ARGS__)
#define DEBUG_PRINTLN(str) g_bluetooth_manager.log("%s\n", str)
#define DEBUG_PRINT(str) g_bluetooth_manager.log("%s", str)
#else
#define DEBUG_PRINTF(format, ...)
#define DEBUG_PRINTLN(str)
#define DEBUG_PRINT(str)
#endif

// Conditional debug macros for different subsystems
#if ENABLE_GRIND_DEBUG
#define GRIND_DEBUG_LOG(format, ...) BLE_LOG(format, ##__VA_ARGS__)
#else
#define GRIND_DEBUG_LOG(format, ...)
#endif

#if ENABLE_LOADCELL_DEBUG
#define LOADCELL_DEBUG_LOG(format, ...) BLE_LOG(format, ##__VA_ARGS__)
#else
#define LOADCELL_DEBUG_LOG(format, ...)
#endif

#if ENABLE_UI_DEBUG
#define UI_DEBUG_LOG(format, ...) BLE_LOG(format, ##__VA_ARGS__)
#else
#define UI_DEBUG_LOG(format, ...)
#endif

#if ENABLE_CALIBRATION_DEBUG
#define CALIBRATION_DEBUG_LOG(format, ...) BLE_LOG(format, ##__VA_ARGS__)
#else
#define CALIBRATION_DEBUG_LOG(format, ...)
#endif

#if ENABLE_SETTLING_DEBUG
#define SETTLING_DEBUG_LOG(format, ...) BLE_LOG(format, ##__VA_ARGS__)
#else
#define SETTLING_DEBUG_LOG(format, ...)
#endif

#ifdef ENABLE_BLE_DEBUG_VERBOSE
#define BLE_DEBUG_LOG(format, ...) BLE_LOG(format, ##__VA_ARGS__)
#else
#define BLE_DEBUG_LOG(format, ...)
#endif

#if ENABLE_OTA_DEBUG
#define OTA_DEBUG_LOG(format, ...) do { \
    BLE_LOG("[OTA_DEBUG] " format, ##__VA_ARGS__); \
} while(0)
#else
#define OTA_DEBUG_LOG(format, ...)
#endif