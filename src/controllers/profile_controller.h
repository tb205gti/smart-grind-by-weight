#pragma once
#include <Preferences.h>
#include "../config/constants.h"

struct Profile {
    char name[USER_PROFILE_NAME_MAX_LENGTH];
    float weight;
};

class ProfileController {
private:
    Profile profiles[USER_PROFILE_COUNT];
    int current_profile;
    Preferences* preferences;

public:
    void init(Preferences* prefs);
    void load_profiles();
    void save_profiles();
    void save_current_profile();
    
    void set_current_profile(int index);
    int get_current_profile() const { return current_profile; }
    float get_current_weight() const { return profiles[current_profile].weight; }
    const char* get_current_name() const { return profiles[current_profile].name; }
    
    void set_profile_weight(int index, float weight);
    float get_profile_weight(int index) const;
    const char* get_profile_name(int index) const;
    
    void update_current_weight(float weight);
    
    // Weight validation methods - single authority for all weight constraints
    bool is_weight_valid(float weight) const;
    float clamp_weight(float weight) const;
};
