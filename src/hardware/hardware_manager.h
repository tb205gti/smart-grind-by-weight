#pragma once
#include <Preferences.h>
#include "display_manager.h"
#include "WeightSensor.h"
#include "grinder.h"

class GrindController; // Forward declaration

class HardwareManager {
private:
    DisplayManager display_manager;
    WeightSensor weight_sensor;
    Grinder grinder;
    Preferences preferences;
    bool initialized;
    GrindController* grind_controller; // Reference to get grinding state
    
public:
    void init();
    void update();
    void set_grind_controller(GrindController* gc) { grind_controller = gc; }
    
    DisplayManager* get_display() { return &display_manager; }
    WeightSensor* get_weight_sensor() { return &weight_sensor; }
    WeightSensor* get_load_cell() { return &weight_sensor; } // Legacy compatibility
    Grinder* get_grinder() { return &grinder; }
    Preferences* get_preferences() { return &preferences; }
    
    bool is_initialized() const { return initialized; }
};
