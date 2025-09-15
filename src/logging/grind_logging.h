#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "../config/constants.h"
#include "../config/system_config.h"

// Forward declarations
class WeightSensor;
class Grinder;

// Buffer settings (PSRAM staging area) - Dynamic calculation based on actual timing
#define MAX_EVENTS_PER_GRIND 50                             // Max discrete events per session (phases, pulses, etc.)

// Core 0 synchronized logging frequency matches the control loop interval
// Example: 30s * 50Hz = 1500 measurements when control interval is 20ms
// Memory impact scales with frequency; keep aligned with SYS_TASK_GRIND_CONTROL_INTERVAL_MS
#define CORE0_CONTROL_FREQUENCY_HZ (1000 / SYS_TASK_GRIND_CONTROL_INTERVAL_MS)
#define CALCULATE_MAX_MEASUREMENTS_PER_GRIND() \
    (USER_GRIND_TIMEOUT_SEC * CORE0_CONTROL_FREQUENCY_HZ / SYS_LOG_EVERY_N_GRIND_LOOPS)

#define MAX_MEASUREMENTS_PER_GRIND CALCULATE_MAX_MEASUREMENTS_PER_GRIND()
#define EVENT_TEMP_BUFFER_SIZE MAX_EVENTS_PER_GRIND  
#define MEASUREMENT_TEMP_BUFFER_SIZE MAX_MEASUREMENTS_PER_GRIND

// Flash storage settings
#define GRIND_SESSIONS_DIR "/sessions"                      // Directory for individual session files
#define SESSION_FILE_FORMAT "/sessions/session_%lu.bin"    // Individual session file naming format
#define GRIND_LOG_FILE "/grind_sessions.bin"                // Legacy single-file storage (deprecated)
#define MAX_STORED_SESSIONS_FLASH 25                        // Maximum sessions to keep in flash (configurable)

// Time-series session header for flash file
struct TimeSeriesSessionHeader {
    uint32_t session_id;            // Session identifier
    uint16_t event_count;           // Number of discrete events in this session
    uint16_t measurement_count;     // Number of continuous measurements in this session
    uint32_t session_timestamp;     // Unix timestamp when session started
    uint32_t session_size;          // Total size of session, events, and measurements in bytes
    uint32_t checksum;              // Checksum of all data
};

// Discrete, low-frequency events summarizing a phase.
// Padded to a fixed size for simple binary parsing.
struct __attribute__((packed)) GrindEvent {
    uint32_t timestamp_ms;         // Relative to session start
    uint8_t  phase_id;             // Numeric phase ID (see GrindPhaseId enum)
    uint8_t  pulse_attempt_number; // Which pulse attempt (1-10), 0 if not a pulse
    uint16_t event_sequence_id;    // **NEW**: Guaranteed unique ID for the event within the session
    uint32_t duration_ms;          // How long this phase/event lasted
    float    start_weight;         // Weight when phase started
    float    end_weight;           // Weight when phase ended
    float    motor_stop_target_weight; // Value at the end of this phase
    float    pulse_duration_ms;    // For PULSE_EXECUTE phase
    uint32_t grind_latency_ms;     // For PREDICTIVE phase
    uint32_t settling_duration_ms; // For SETTLING phases
    float    pulse_flow_rate;      // Flow rate from predictive phase used for pulse calculations
    uint16_t loop_count;           // Number of loop iterations for this phase (performance tracking)
    char     padding[4];           // **MODIFIED**: Pad to 46 bytes total size (42 bytes data + 4 bytes padding)

    GrindEvent() {
        memset(this, 0, sizeof(GrindEvent));
    }
};

// Continuous, high-frequency time-series measurements.
// Slim and optimized for high-volume logging.
struct __attribute__((packed)) GrindMeasurement {
    uint32_t timestamp_ms;         // Relative to session start
    float    weight_grams;         // Current weight reading
    float    weight_delta;         // Change since last measurement
    float    flow_rate_g_per_s;    // Continuously calculated flow rate
    uint8_t  motor_is_on;          // Motor state at time of measurement (using uint8_t for exact size)
    uint8_t  phase_id;            // Current grinding phase ID (from GrindPhaseId enum)
    float    motor_stop_target_weight; // Current motor stop target weight at time of measurement

    GrindMeasurement() {
        memset(this, 0, sizeof(GrindMeasurement));
    }
};


// Session metadata - configuration snapshot and results summary
struct GrindSession {
    // Primary Key
    uint32_t session_id;           // Auto-increment ID
    uint32_t session_timestamp;    // Unix timestamp (session start)
    
    // Profile & Configuration
    uint8_t profile_id;            // From current system
    float target_weight;           // From current system  
    float tolerance;               // From current system (may override USER_GRIND_ACCURACY_TOLERANCE_G)
    
    // Grind Controller Configuration (snapshot at session start)
    float initial_motor_stop_offset; // Starting motor stop offset (USER_GRIND_UNDERSHOOT_TARGET_G or dynamic)
    uint8_t max_pulse_attempts;    // USER_GRIND_MAX_PULSE_ATTEMPTS config
    float latency_to_coast_ratio;  // USER_LATENCY_TO_COAST_RATIO config
    float flow_rate_threshold;     // USER_GRIND_FLOW_DETECTION_THRESHOLD_GPS config
    
    // Pulse Duration Configuration (active during this session)
    float pulse_duration_large;    // HW_PULSE_LARGE_ERROR_MS config
    float pulse_duration_medium;   // HW_PULSE_MEDIUM_ERROR_MS config  
    float pulse_duration_small;    // HW_PULSE_SMALL_ERROR_MS config
    float pulse_duration_fine;     // HW_PULSE_FINE_ERROR_MS config
    
    // Error Threshold Configuration
    float large_error_threshold;   // SYS_GRIND_ERROR_LARGE_THRESHOLD_G config
    float medium_error_threshold;  // SYS_GRIND_ERROR_MEDIUM_THRESHOLD_G config
    float small_error_threshold;   // SYS_GRIND_ERROR_SMALL_THRESHOLD_G config
    
    // Results Summary (calculated from existing data)
    float final_weight;            // From final_cup_weight
    float error_grams;             // target - final 
    uint32_t total_time_ms;        // From total_grind_time_ms
    uint8_t pulse_count;           // From pulse_attempts_count
    char result_status[16];        // From final_phase_result
    
    // Motor Summary
    uint32_t total_motor_on_time_ms;  // Total motor on time for entire session
    
    // Constructor
    GrindSession() {
        memset(this, 0, sizeof(GrindSession));
    }
};

// Time-series grind logging manager
class GrindLogger {
private:
    // Time-series system
    GrindSession* current_session;           // Current session metadata (PSRAM)
    GrindEvent* event_buffer;                // PSRAM temp buffer for events
    GrindMeasurement* measurement_buffer;    // PSRAM temp buffer for measurements
    uint16_t event_count;                    // Current number of events in buffer
    uint16_t measurement_count;              // Current number of measurements in buffer
    uint16_t event_sequence_counter;         // **NEW**: Counter for unique event IDs
    
    char current_phase_name[16];             // Current grinding phase name
    bool logging_active;                     // Whether a grind session is active
    
    // Motor time tracking
    bool last_motor_state;                   // Previous motor state for change detection
    uint32_t motor_start_time;               // When motor last turned on
    uint32_t total_motor_time_ms;            // Accumulated motor on time for session
    
    // Current session data
    uint32_t session_start_time;
    
    // Session ID management
    Preferences* _preferences;
    uint32_t _next_session_id;
    
public:
    bool init(Preferences* prefs);           // Initialize PSRAM buffer
    void cleanup();                          // Free PSRAM buffer
    
    // Session management
    void start_grind_session(float target_weight, uint8_t profile_id, float tolerance);
    void end_grind_session(const char* final_result, float final_weight, uint8_t pulse_count);
    void discard_current_session();         // Discard current session without saving
    
    // Logging methods
    void log_event(GrindEvent& event);       // **MODIFIED**: Takes non-const reference to set sequence ID
    void log_continuous_measurement(uint32_t timestamp_ms, float weight_grams, float weight_delta, 
                                  float flow_rate_g_per_s, uint8_t motor_is_on, uint8_t phase_id, 
                                  float motor_stop_target_weight);
    
    // Flash storage management
    bool flush_session_to_flash();          // Flush current session to flash
    bool rotate_flash_log_if_needed();      // Remove old sessions if limit exceeded
    bool clear_all_sessions_from_flash();   // Purge all stored sessions (for developer purge)
    uint32_t count_sessions_in_flash() const; // Count total sessions in flash file
    uint32_t count_total_events_in_flash() const; // Count total events across all sessions
    uint32_t count_total_measurements_in_flash() const; // Count total measurements across all sessions
    
    // Fixed-length binary export method
    void export_sessions_binary_chunk(uint8_t* buffer, size_t buffer_size,
                                     uint32_t start_pos, uint32_t* next_pos, size_t* actual_size);
    void send_current_session_via_serial();  // Debug output for current session
    
    // Data access
    uint32_t get_total_flash_sessions() const;
    bool is_logging_active() const { return logging_active; }
    
    // Debug output helpers - conditionally compiled based on debug flags (moved to public for BLE access)
#if ENABLE_GRIND_DEBUG
    void print_session_data_table();           // Print sessions in tabular format for debugging
    void print_struct_layout_debug();          // Print struct sizes and offsets for Python parsing
    void print_comprehensive_debug();          // Print full struct contents and raw memory dumps
#else
    // Inline no-op versions when debugging is disabled
    inline void print_session_data_table() {}
    inline void print_struct_layout_debug() {}
    inline void print_comprehensive_debug() {}
#endif
    
private:
    // Time-series system helpers
    void clear_buffers();
    void initialize_session_config();       // Snapshot current config into session
    void reset_export_static_variables();   // Reset static variables used in export function
    
    // Flash storage helpers
    uint32_t calculate_checksum(const uint8_t* data, size_t length); // Simple checksum calculation
    bool write_time_series_session_to_flash(const GrindSession& session, const GrindEvent* events, const GrindMeasurement* measurements);
    bool remove_oldest_sessions(uint32_t sessions_to_remove); // Remove oldest sessions from flash file (legacy)
    
    // Individual session file management
    bool ensure_sessions_directory_exists();    // Create sessions directory if needed
    bool write_individual_session_file(uint32_t session_id, const GrindSession& session, const GrindEvent* events, const GrindMeasurement* measurements);
    bool validate_session_file(uint32_t session_id); // Check if session file is valid/readable
    bool remove_session_file(uint32_t session_id);   // Delete specific session file
    void cleanup_old_session_files(); // Remove old session files to maintain MAX_STORED_SESSIONS_FLASH limit
    
};

// Global logger instance
extern GrindLogger grind_logger;
