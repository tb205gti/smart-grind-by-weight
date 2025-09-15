# Grinder Measurement Definitions

This document provides comprehensive definitions for all measurements collected by the ESP32-S3 coffee grinder system. Each measurement includes its description, calculation method, and timestamp semantics.

**Source Reference**: Based on analysis of `src/controllers/grind_controller.cpp` and `src/logging/grind_logging.h`

---

## 1. GrindEvent Structure

**Purpose**: Discrete, low-frequency events that summarize grinding phases  
**Frequency**: One event per phase transition  
**Timestamp Semantics**: **START** of the phase  
**Source**: `src/logging/grind_logging.h:56`

### Fields

#### `timestamp_ms` (uint32_t)
- **Description**: Milliseconds since session start when this phase **began**
- **Calculation**: `millis() - session_start_time` at phase start
- **Example**: Phase starts 5.2 seconds into session → `timestamp_ms = 5200`
- **Source**: `src/controllers/grind_controller.cpp:278`

#### `phase_id` (uint8_t) 
- **Description**: Numeric ID of the grinding phase
- **Calculation**: Direct mapping from `GrindPhase` enum to `GrindPhaseId`
- **Values**: 0=IDLE, 1=TARING, 2=TARE_CONFIRM, 3=PREDICTIVE, 4=PULSE_DECISION, 5=PULSE_EXECUTE, 6=PULSE_SETTLING, 7=FINAL_SETTLING, 8=COMPLETED, 9=TIMEOUT
- **Source**: `src/logging/grind_logging.h:41-52`

#### `pulse_attempt_number` (uint8_t)
- **Description**: Which pulse correction attempt (1-based), 0 if not a pulse phase
- **Calculation**: Value of `pulse_attempts` counter during PULSE_EXECUTE phase
- **Range**: 0-10 (MAX_PULSE_ATTEMPTS = 10)
- **Source**: `src/controllers/grind_controller.cpp:262`

#### `event_sequence_id` (uint16_t)
- **Description**: Unique sequential ID for this event within the session
- **Calculation**: Auto-incrementing counter starting from 0
- **Example**: First event = 0, second event = 1, etc.
- **Source**: `src/logging/grind_logging.cpp:131`

#### `duration_ms` (uint32_t)
- **Description**: How long this phase lasted in milliseconds
- **Calculation**: `phase_end_time - phase_start_time`
- **Example**: GRINDING phase lasts 8.3 seconds → `duration_ms = 8300`
- **Source**: `src/controllers/grind_controller.cpp:253`

#### `start_weight` (float)
- **Description**: Weight in grams when phase started
- **Calculation**: `load_cell->get_smoothed_weight()` at phase start
- **Example**: Start grinding at 2.1g → `start_weight = 2.1`
- **Source**: `src/controllers/grind_controller.cpp:279`

#### `end_weight` (float)
- **Description**: Weight in grams when phase ended
- **Calculation**: `load_cell->get_smoothed_weight()` at phase end
- **Example**: End grinding at 17.8g → `end_weight = 17.8`
- **Source**: `src/controllers/grind_controller.cpp:254`

#### `motor_stop_target_weight` (float)
- **Description**: Motor stop target weight at the end of this phase
- **Calculation**: Dynamic value computed during predictive phase based on flow and latency
- **Example**: Target weight minus calculated coast offset
- **Source**: `src/logging/grind_logging.h:51`

#### `pulse_duration_ms` (float)
- **Description**: Duration of motor pulse in milliseconds (PULSE_EXECUTE phase only)
- **Calculation**: Error-based lookup: >1.0g→100ms, >0.5g→60ms, >0.2g→40ms, ≤0.2g→20ms
- **Source**: `src/controllers/grind_controller.cpp:232-237`

#### `grind_latency_ms` (uint32_t)
- **Description**: Time from motor start to first detectable weight change (PREDICTIVE phase only)
- **Calculation**: `millis() - phase_start_time` when weight > `MINIMUM_WEIGHT_THRESHOLD`
- **Example**: Motor starts, weight increases after 1.8s → `grind_latency_ms = 1800`
- **Source**: `src/controllers/grind_controller.cpp:153`

#### `settling_duration_ms` (uint32_t)
- **Description**: How long weight took to settle (SETTLING phases only)
- **Calculation**: Same as `duration_ms` for settling phases
- **Source**: `src/controllers/grind_controller.cpp:264`

#### `pulse_flow_rate` (float)
- **Description**: Flow rate from predictive phase used for pulse calculations
- **Calculation**: Flow rate value captured during predictive phase for use in pulse duration decisions
- **Source**: `src/logging/grind_logging.h:55`

#### `loop_count` (uint16_t)
- **Description**: Number of loop iterations for this phase (performance tracking)
- **Calculation**: Counter incremented each control loop iteration during the phase
- **Source**: `src/logging/grind_logging.h:56`

---

## 2. GrindMeasurement Structure

**Purpose**: High-frequency time-series measurements during grinding  
**Frequency**: Every N grind control loops (LOG_EVERY_N_GRIND_LOOPS = 5, 20ms intervals = 100ms)  
**Timestamp Semantics**: **TIME OF MEASUREMENT**  
**Source**: `src/logging/grind_logging.h:77`

### Fields

#### `timestamp_ms` (uint32_t)
- **Description**: Milliseconds since session start when measurement was taken
- **Calculation**: `millis() - session_start_time` at measurement time
- **Example**: Measurement at 3.4 seconds → `timestamp_ms = 3400`
- **Source**: `src/logging/grind_logging.cpp:144`

#### `weight_grams` (float)
- **Description**: Current instant weight reading in grams
- **Calculation**: `load_cell->get_instant_weight()` - raw, unfiltered reading
- **Example**: Current weight 12.3g → `weight_grams = 12.3`
- **Source**: `src/logging/grind_logging.cpp:145`

#### `weight_delta` (float)
- **Description**: Change in weight since last measurement
- **Calculation**: `current_weight - last_weight_for_delta`
- **Example**: Weight increased 0.2g since last → `weight_delta = 0.2`
- **Source**: `src/logging/grind_logging.cpp:146`

#### `flow_rate_g_per_s` (float)
- **Description**: Instantaneous flow rate in grams per second
- **Calculation**: `(current_weight - previous_weight) / (time_delta_ms / 1000.0)`
- **Example**: 0.3g gained in 100ms → `0.3 / 0.1 = 3.0 g/s`
- **Source**: `src/logging/grind_logging.cpp:324-326`

#### `motor_is_on` (uint8_t)
- **Description**: Motor state at time of measurement (1=on, 0=off)
- **Calculation**: `grinder->is_grinding() ? 1 : 0`
- **Source**: `src/logging/grind_logging.cpp:148`

#### `phase_id` (uint8_t)
- **Description**: Current grinding phase at time of measurement
- **Calculation**: Same mapping as GrindEvent phase_id
- **Source**: `src/logging/grind_logging.cpp:149`

---

## 3. GrindSession Structure

**Purpose**: Session metadata, configuration snapshot, and results summary  
**Frequency**: One per grinding session  
**Source**: `src/logging/grind_logging.h:92`

### Primary Key

#### `session_id` (uint32_t)
- **Description**: Unique auto-incrementing session identifier
- **Calculation**: `scan_directory_for_highest_session_id() + 1`
- **Source**: `src/logging/grind_logging.cpp:365-372`

#### `session_timestamp` (uint32_t)
- **Description**: Unix timestamp when session started
- **Calculation**: `millis() / 1000` at session start
- **Source**: `src/logging/grind_logging.cpp:69`

### Profile & Configuration

#### `profile_id` (uint8_t)
- **Description**: Coffee profile used for this session
- **Source**: `src/logging/grind_logging.cpp:71`

#### `target_weight` (float)
- **Description**: Desired final weight in grams
- **Source**: `src/logging/grind_logging.cpp:71`

#### `tolerance` (float)
- **Description**: Acceptable error tolerance in grams (±)
- **Default**: `GRIND_TOLERANCE = 0.05f`
- **Source**: `src/logging/grind_logging.cpp:72`

### Configuration Snapshots

All configuration values are captured at session start from constants.h:

#### `initial_motor_stop_offset` (float)
- **Description**: Starting motor stop offset before dynamic calculation
- **Value**: `USER_GRIND_UNDERSHOOT_TARGET_G` or dynamic value
- **Source**: `src/logging/grind_logging.cpp:323`

#### `max_pulse_attempts` (uint8_t)  
- **Description**: Maximum pulse corrections allowed
- **Value**: `USER_GRIND_MAX_PULSE_ATTEMPTS`
- **Source**: `src/logging/grind_logging.cpp:324`

#### `latency_to_coast_ratio` (float)
- **Description**: Ratio for converting latency to coast time calculation
- **Value**: `USER_LATENCY_TO_COAST_RATIO`
- **Source**: `src/logging/grind_logging.cpp:325`

#### `flow_rate_threshold` (float)
- **Description**: Minimum flow rate to enable dynamic motor stop calculation
- **Value**: `USER_GRIND_FLOW_DETECTION_THRESHOLD_GPS` g/s
- **Source**: `src/logging/grind_logging.cpp:326`

### Pulse Duration Configuration

#### `pulse_duration_large` (float)
- **Description**: Motor pulse duration for large errors
- **Value**: `HW_PULSE_LARGE_ERROR_MS` ms
- **Source**: `src/logging/grind_logging.cpp:327`

#### `pulse_duration_medium` (float)  
- **Description**: Motor pulse duration for medium errors
- **Value**: `HW_PULSE_MEDIUM_ERROR_MS` ms
- **Source**: `src/logging/grind_logging.cpp:328`

#### `pulse_duration_small` (float)
- **Description**: Motor pulse duration for small errors
- **Value**: `HW_PULSE_SMALL_ERROR_MS` ms
- **Source**: `src/logging/grind_logging.cpp:329`

#### `pulse_duration_fine` (float)
- **Description**: Motor pulse duration for fine errors
- **Value**: `HW_PULSE_FINE_ERROR_MS` ms
- **Source**: `src/logging/grind_logging.cpp:330`

### Error Thresholds

#### `large_error_threshold` (float)
- **Value**: `SYS_GRIND_ERROR_LARGE_THRESHOLD_G` g
- **Source**: `src/logging/grind_logging.cpp:331`

#### `medium_error_threshold` (float)
- **Value**: `SYS_GRIND_ERROR_MEDIUM_THRESHOLD_G` g
- **Source**: `src/logging/grind_logging.cpp:332`

#### `small_error_threshold` (float)
- **Value**: `SYS_GRIND_ERROR_SMALL_THRESHOLD_G` g
- **Source**: `src/logging/grind_logging.cpp:333`

### Results Summary

#### `final_weight` (float)
- **Description**: Final settled weight after grinding completed
- **Calculation**: `load_cell->get_smoothed_weight()` at session end
- **Source**: `src/logging/grind_logging.cpp:93`

#### `error_grams` (float)
- **Description**: Final error (positive = undershoot, negative = overshoot)
- **Calculation**: `target_weight - final_weight`
- **Example**: Target 18.0g, final 17.8g → `error_grams = +0.2` (undershoot)
- **Source**: `src/logging/grind_logging.cpp:94`

#### `total_time_ms` (uint32_t)
- **Description**: Total session duration from start to completion
- **Calculation**: `millis() - session_start_time` at session end
- **Source**: `src/logging/grind_logging.cpp:95`

#### `pulse_count` (uint8_t)
- **Description**: Number of pulse corrections attempted
- **Calculation**: Value of `pulse_attempts` counter at session end
- **Source**: `src/logging/grind_logging.cpp:96`

#### `result_status` (char[16])
- **Description**: Final session outcome
- **Values**: "COMPLETE", "OVERSHOOT", "COMPLETE - MAX PULSES", "TIMEOUT", "STOPPED_BY_USER"
- **Source**: `src/controllers/grind_controller.cpp:205-212`

#### `total_motor_on_time_ms` (uint32_t)
- **Description**: Cumulative time motor was actually running
- **Calculation**: Sum of all motor-on periods during session
- **Example**: Motor runs 3.2s + 0.1s + 0.05s = 3.35s → `total_motor_on_time_ms = 3350`
- **Source**: `src/logging/grind_logging.cpp:102-104`

---

## Key Calculation Details

### Dynamic Motor Stop Target Weight
```pseudocode
if (current_flow_rate >= USER_GRIND_FLOW_DETECTION_THRESHOLD_GPS && !flow_start_confirmed):
    grind_latency_ms = current_time - phase_start_time
    flow_start_confirmed = true
    
if (flow_start_confirmed && current_flow_rate > USER_GRIND_FLOW_DETECTION_THRESHOLD_GPS):
    motor_stop_target_weight = ((grind_latency_ms * USER_LATENCY_TO_COAST_RATIO) / 1000.0) * current_flow_rate
```

### Flow Rate (FlowRateData rolling window)
```pseudocode
// Used by GrindController for dynamic motor stop calculation
time_span_ms = newest_sample_time - oldest_sample_time
weight_span = newest_weight - oldest_weight
flow_rate = (weight_span / time_span_ms) * 1000.0
```

### Flow Rate (GrindLogger simple delta)
```pseudocode  
// Used by GrindMeasurement logging - pre-calculated by GrindController
flow_rate = weight_sensor->get_flow_rate()  // Passed as parameter
```

### Motor Time Tracking
```pseudocode
on_motor_start():
    motor_start_time = current_time
    
on_motor_stop():
    if motor_start_time > 0:
        total_motor_time += (current_time - motor_start_time)
```

---

## Important Notes

1. **Timestamp Semantics Differ**: 
   - GrindEvent timestamps = **phase start time**
   - GrindMeasurement timestamps = **measurement time**

2. **Error Sign Convention**: 
   - Positive error = undershoot (need more coffee)
   - Negative error = overshoot (too much coffee)

3. **Two Flow Rate Methods**:
   - GrindController uses rolling window over 10 samples
   - GrindLogger uses simple delta between measurements

4. **Phase Duration vs Total Time**:
   - GrindEvent duration_ms = individual phase duration
   - GrindSession total_time_ms = entire session duration

5. **Motor Time vs Session Time**:
   - total_motor_on_time_ms = actual grinding time
   - total_time_ms = session duration (includes settling, etc.)