#pragma once

//==============================================================================
// DEPRECATED UI THEME CONFIGURATION FILE
//==============================================================================
// This file has been moved to src/config/theme_config.h
// Include the new location for backward compatibility
//
// All UI theme constants are now organized in theme_config.h with consistent naming:
// - THEME_COLOR_PRIMARY, THEME_COLOR_ACCENT, etc.
// - THEME_BUTTON_HEIGHT_PX, THEME_BUTTON_WIDTH_PX
// - THEME_PROGRESS_ARC_DIAMETER_PX, THEME_CORNER_RADIUS_PX
//
// TODO: Update includes to use "config/theme_config.h" instead

#include "theme_config.h"

// Legacy theme name mappings for backward compatibility
#define COLOR_PRIMARY THEME_COLOR_PRIMARY
#define COLOR_SECONDARY THEME_COLOR_SECONDARY  
#define COLOR_ACCENT THEME_COLOR_ACCENT
#define COLOR_TEXT_PRIMARY THEME_COLOR_TEXT_PRIMARY
#define COLOR_TEXT_SECONDARY THEME_COLOR_TEXT_SECONDARY
#define COLOR_BACKGROUND THEME_COLOR_BACKGROUND
#define COLOR_SUCCESS THEME_COLOR_SUCCESS
#define COLOR_ERROR THEME_COLOR_ERROR
#define COLOR_WARNING THEME_COLOR_WARNING
#define COLOR_NEUTRAL THEME_COLOR_NEUTRAL

#define BUTTON_HEIGHT THEME_BUTTON_HEIGHT_PX
#define BUTTON_WIDTH THEME_BUTTON_WIDTH_PX
#define ARC_SIZE THEME_PROGRESS_ARC_DIAMETER_PX
#define CORNER_RADIUS THEME_CORNER_RADIUS_PX