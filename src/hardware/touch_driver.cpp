#include "touch_driver.h"
#include "esp_log.h"

void TouchDriver::init() {

    Wire.begin(HW_TOUCH_I2C_SDA_PIN, HW_TOUCH_I2C_SCL_PIN);
    Wire.setClock(300000);
    
    last_touch = {0, 0, false};
    initialized = true;
    disabled = false;
    
    // Initialize touch tracking
    last_touch_time = millis();
}

void TouchDriver::update() {
    if (!initialized || disabled) return;
    
    uint8_t buf[5] = {0};
    
    Wire.beginTransmission(HW_TOUCH_I2C_ADDRESS);
    Wire.write(0x02); // FT3168_REG_NUM_TOUCHES
    Wire.endTransmission(false);
    
    if (Wire.requestFrom(HW_TOUCH_I2C_ADDRESS, 5) == 5) {
        for (int i = 0; i < 5; i++) {
            buf[i] = Wire.read();
        }
        
        uint8_t touches = buf[0] & 0x0F;
        if (touches > 0) {
            last_touch.x = ((buf[1] & 0x0F) << 8) | buf[2];
            last_touch.y = ((buf[3] & 0x0F) << 8) | buf[4];
            last_touch.pressed = true;
            
            // Update last touch time
            last_touch_time = millis();
        } else {
            last_touch.pressed = false;
        }
    } else {
        last_touch.pressed = false;
    }
}

void TouchDriver::disable() {
    disabled = true;
    last_touch.pressed = false;  // Clear any active touch state
}

void TouchDriver::enable() {
    disabled = false;
}

uint32_t TouchDriver::get_ms_since_last_touch() const {
    if (!initialized || disabled) return 0;
    return millis() - last_touch_time;
}
