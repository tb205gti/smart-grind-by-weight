#include "confirm_screen.h"
#include <Arduino.h>

void ConfirmScreen::create() {
    screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));
    lv_obj_align(screen, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    // Title label
    title_label = lv_label_create(screen);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_32, 0);  // Smaller font
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, -140);  // Move higher

    // Warning/message label
    warning_label = lv_label_create(screen);
    lv_obj_set_style_text_font(warning_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(warning_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_align(warning_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(warning_label, LV_ALIGN_CENTER, 0, -20);  // Move up slightly

    // Confirm button (bottom left)
    confirm_button = lv_btn_create(screen);
    lv_obj_set_size(confirm_button, THEME_BUTTON_WIDTH_PX, CONFIRM_BUTTON_HEIGHT);
    lv_obj_align(confirm_button, LV_ALIGN_BOTTOM_LEFT, 10, -5);
    lv_obj_set_style_border_width(confirm_button, 0, 0);
    
    confirm_button_label = lv_label_create(confirm_button);
    lv_obj_set_style_text_font(confirm_button_label, &lv_font_montserrat_24, 0);
    lv_obj_center(confirm_button_label);

    // Cancel button (bottom right)
    cancel_button = lv_btn_create(screen);
    lv_obj_set_size(cancel_button, THEME_BUTTON_WIDTH_PX, CONFIRM_BUTTON_HEIGHT);
    lv_obj_align(cancel_button, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
    lv_obj_set_style_bg_color(cancel_button, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_obj_set_style_border_width(cancel_button, 0, 0);
    
    cancel_button_label = lv_label_create(cancel_button);
    lv_obj_set_style_text_font(cancel_button_label, &lv_font_montserrat_24, 0);
    lv_obj_center(cancel_button_label);

    visible = false;
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
}

void ConfirmScreen::show(const char* title, const char* message, 
                        const char* confirm_text, lv_color_t confirm_color,
                        const char* cancel_text) {
    // Set title and color
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_color(title_label, confirm_color, 0);
    
    // Set message
    lv_label_set_text(warning_label, message);
    
    // Set confirm button
    lv_label_set_text(confirm_button_label, confirm_text);
    lv_obj_set_style_bg_color(confirm_button, confirm_color, 0);
    
    // Set cancel button
    lv_label_set_text(cancel_button_label, cancel_text);
    
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = true;
}

void ConfirmScreen::show() {
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = true;
}

void ConfirmScreen::hide() {
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = false;
}