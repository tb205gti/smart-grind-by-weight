# C++ to Python Struct Alignment Guide

This document provides essential guidance for maintaining data structure alignment between the ESP32 C++ firmware and Python BLE data processing tools. **Following this guide prevents parsing failures and data corruption during binary data exchange.**

## Critical Warning: Data Structure Changes Require Purge

⚠️ **MANDATORY**: Every time you modify any struct in `grind_logging.h`, you MUST purge all flash data using the "PURGE HISTORY" button on the ESP32 device before testing. Old corrupted data will cause alignment issues and false debugging results.

**Session ID Reset**: The purge function automatically resets all cached session tracking variables, ensuring session IDs restart from 1 after a purge.

## Struct Alignment Process

### Step 1: ESP32 Struct Layout Debugging

The ESP32 firmware includes debug functions to output actual memory layout:

```cpp
// In grind_logging.h
void print_struct_layout_debug();      // Shows sizes and field offsets for alignment debugging
```

**When debugging alignment issues:**
1. Flash updated firmware to ESP32
2. **PURGE HISTORY** to clear all old data
3. Trigger BLE export - this automatically prints struct layout debug info
4. Use the output to verify/update Python parsing

### Step 2: Python Parsing Update

Update `tools/grinder-ble` parsing based on ESP32 debug output:

```python
# Key structs to maintain alignment for:
GRIND_SESSION_SIZE = 100  # Update based on sizeof(GrindSession)
GRIND_EVENT_SIZE = 46     # Update based on sizeof(GrindEvent) 
GRIND_MEASUREMENT_SIZE = 22  # Update based on sizeof(GrindMeasurement)

# Binary parsing format strings
GRIND_SESSION_FORMAT = '<IIffffff...'  # Update field by field
```

### Step 3: Verification

Essential debug output from ESP32 shows:
```
=== STRUCT LAYOUT DEBUG ===
TimeSeriesSessionHeader: 20 bytes
GrindSession: 100 bytes (current size)
GrindEvent: 46 bytes  
GrindMeasurement: 22 bytes
```

## Current Struct Definitions (Reference)

### GrindSession (100 bytes)
```cpp
struct GrindSession {
    uint32_t session_id;           // 4 bytes, offset 0
    uint32_t session_timestamp;    // 4 bytes, offset 4  
    uint8_t profile_id;            // 1 byte,  offset 8
    float target_weight;           // 4 bytes, offset 9
    // ... (see grind_logging.h for complete definition)
    char result_status[16];        // 16 bytes, offset 77
    uint32_t total_motor_on_time_ms; // 4 bytes, offset 93
    // Total: 100 bytes (97 + 3 padding)
};
```

### GrindEvent (46 bytes)  
```cpp
struct __attribute__((packed)) GrindEvent {
    uint32_t timestamp_ms;         // 4 bytes, offset 0
    uint8_t  phase_id;            // 1 byte, offset 4
    uint8_t  pulse_attempt_number; // 1 byte, offset 5
    uint16_t event_sequence_id;    // 2 bytes, offset 6
    uint32_t duration_ms;          // 4 bytes, offset 8
    float    start_weight;         // 4 bytes, offset 12
    float    end_weight;           // 4 bytes, offset 16
    float    motor_stop_target_weight; // 4 bytes, offset 20
    float    pulse_duration_ms;    // 4 bytes, offset 24
    uint32_t grind_latency_ms;     // 4 bytes, offset 28
    uint32_t settling_duration_ms; // 4 bytes, offset 32
    float    pulse_flow_rate;      // 4 bytes, offset 36
    uint16_t loop_count;           // 2 bytes, offset 40
    char padding[4];               // 4 bytes padding
    // Total: 46 bytes (42 bytes data + 4 bytes padding)
};
```

### GrindMeasurement (22 bytes)
```cpp
struct __attribute__((packed)) GrindMeasurement {
    uint32_t timestamp_ms;         // 4 bytes
    float    weight_grams;         // 4 bytes
    float    weight_delta;         // 4 bytes
    float    flow_rate_g_per_s;    // 4 bytes
    uint8_t  motor_is_on;          // 1 byte
    uint8_t  phase_id;            // 1 byte
    float    motor_stop_target_weight; // 4 bytes
    // Total: 22 bytes
};
```

## Common Alignment Issues

### Issue: Python Parsing Failures
**Symptoms**: 
- `struct.error: unpack_from requires a buffer of X bytes`
- Incorrect field values in parsed data

**Solution**:
1. Check ESP32 debug output for actual struct sizes
2. Update Python `*_SIZE` constants
3. Verify format strings match field order and types

### Issue: Field Offset Misalignment  
**Symptoms**:
- Fields contain garbage data
- String fields show binary data

**Solution**:
1. Use ESP32 `print_struct_layout_debug()` to see struct sizes and field offsets
2. Manually verify each field offset in Python parsing
3. Account for compiler padding between fields

### Issue: Old Corrupted Data
**Symptoms**: 
- Parsing works sometimes but fails other times
- Debug output shows mixed clean/corrupted data

**Solution**:
1. **Always purge flash data after struct changes**
2. Test only with fresh grind sessions
3. Verify purge worked by checking session count

## Development Workflow

1. **Modify structs** in `grind_logging.h`
2. **Build and flash** firmware: `pio run --target upload`
3. **PURGE HISTORY** via ESP32 developer screen  
4. **Perform test grind** to generate fresh data
5. **Trigger BLE export** - review struct debug output
6. **Update Python parsing** in `tools/grinder-ble`
7. **Test full export pipeline** to verify alignment

## Key Files

- **ESP32 Structs**: `src/logging/grind_logging.h`
- **ESP32 Debug Functions**: `src/logging/grind_logging.cpp`
- **Python Parser**: `tools/grinder-ble` 
- **SQL Schema**: Updated automatically by Python parser

## Debug Commands

```bash
# ESP32 serial monitor (shows struct debug during export)
pio device monitor --baud 115200

# Test BLE export and parsing  
cd tools && python3 grinder-ble --connect --auto
```

Remember: **Every struct change = Mandatory flash purge + Python parsing update**