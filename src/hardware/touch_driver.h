#pragma once
#include <driver/i2c_master.h>
#include "../config/constants.h"

struct TouchData {
    uint16_t x;
    uint16_t y;
    bool pressed;
};

class TouchDriver {
private:
    TouchData last_touch;
    bool initialized = false;
    bool disabled = false;

    // Touch activity tracking
    uint32_t last_touch_time;

    // Re-init watchdog: triggers recovery after sustained poll failures
    uint32_t consecutive_poll_failures = 0;
    static constexpr uint32_t kMaxConsecutivePollFailures = 50; // ~800ms at 16ms poll rate

    // Retry backoff when chip is not yet ready at boot
    uint32_t last_init_attempt_ms = 0;
    static constexpr uint32_t kInitRetryIntervalMs = 500;

    bool faulted = false;

public:
    void init();
    void update();
    void disable();
    void enable();
    TouchData get_touch_data() const { return last_touch; }
    bool is_pressed() const { return last_touch.pressed; }
    bool is_faulted() const { return faulted; }

    // Touch activity timing
    uint32_t get_ms_since_last_touch() const;

private:
    i2c_master_bus_handle_t bus_handle = nullptr;
    i2c_master_dev_handle_t device_handle = nullptr;

    // Unstick I2C bus from a prior incomplete transaction (9-clock recovery + STOP)
    void recover_i2c_bus();
    // Tear down I2C handles cleanly before a re-init attempt
    void deinit();
};
