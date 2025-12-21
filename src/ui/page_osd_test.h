#pragma once

#include <lvgl/lvgl.h>

#include "ui/ui_main_menu.h"

extern page_pack_t pp_osd_test;

// Keyboard handler for OSD test (called from input_device when page is active)
void page_osd_test_handle_keyboard(int key_code);
