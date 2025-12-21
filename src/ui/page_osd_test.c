#include "page_osd_test.h"

#include <log/log.h>

#ifdef EMULATOR_BUILD
#include "core/SDLaccess.h"
#endif

#include "core/common.hh"
#include "core/ht.h"
#include "core/osd.h"
#include "core/settings.h"
#include "lang/language.h"
#include "ui/page_common.h"
#include "ui/ui_style.h"

// Current head tracker values for testing
static int16_t test_heading = 0;
static int16_t test_pitch = 0;

static lv_obj_t *label_heading;
static lv_obj_t *label_pitch;
static lv_obj_t *label_controls;
static lv_timer_t *timer;

static lv_coord_t col_dsc[] = {160, 160, 160, 160, 160, 160, 160, 160, LV_GRID_TEMPLATE_LAST};
static lv_coord_t row_dsc[] = {60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, LV_GRID_TEMPLATE_LAST};

static lv_obj_t *page_osd_test_create(lv_obj_t *parent, panel_arr_t *arr) {
    lv_obj_t *page = lv_menu_page_create(parent, NULL);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(page, 1280, 720);
    lv_obj_add_style(page, &style_subpage, LV_PART_MAIN);
    lv_obj_set_style_pad_top(page, 94, 0);

    lv_obj_t *section = lv_menu_section_create(page);
    lv_obj_add_style(section, &style_submenu, LV_PART_MAIN);
    lv_obj_set_size(section, 1280, 600);

    create_text(NULL, section, false, _lang("Head Tracker OSD Test"), LV_MENU_ITEM_BUILDER_VARIANT_2);

    lv_obj_t *cont = lv_obj_create(section);
    lv_obj_set_size(cont, 1280, 500);
    lv_obj_set_pos(cont, 0, 60);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(cont, &style_context, LV_PART_MAIN);

    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);

    // Instructions
    lv_obj_t *label_title = lv_label_create(cont);
    lv_label_set_text(label_title, "OSD Test Mode - No Video Signal");
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_align(label_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label_title, lv_color_make(255, 255, 255), 0);
    lv_obj_set_grid_cell(label_title, LV_GRID_ALIGN_CENTER, 0, 8, LV_GRID_ALIGN_CENTER, 0, 1);

    // Current values display
    label_heading = lv_label_create(cont);
    lv_label_set_text_fmt(label_heading, "Heading: %d°", test_heading);
    lv_obj_set_style_text_font(label_heading, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_color(label_heading, lv_color_make(0, 255, 0), 0);
    lv_obj_set_grid_cell(label_heading, LV_GRID_ALIGN_CENTER, 0, 4, LV_GRID_ALIGN_CENTER, 2, 1);

    label_pitch = lv_label_create(cont);
    lv_label_set_text_fmt(label_pitch, "Pitch: %d°", test_pitch);
    lv_obj_set_style_text_font(label_pitch, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_color(label_pitch, lv_color_make(0, 255, 0), 0);
    lv_obj_set_grid_cell(label_pitch, LV_GRID_ALIGN_CENTER, 4, 4, LV_GRID_ALIGN_CENTER, 2, 1);

    // Controls help text
    label_controls = lv_label_create(cont);
    lv_label_set_text(label_controls,
                      "Head Tracker Status:\n"
                      "  If HT is enabled in settings, real values shown\n"
                      "  If HT is disabled, use keyboard controls\n\n"
                      "Keyboard Controls (when HT disabled):\n\n"
                      "Arrow Keys:\n"
                      "  LEFT/RIGHT: Adjust Heading (±5°)\n"
                      "  UP/DOWN: Adjust Pitch (±5°)\n\n"
                      "Fine Control:\n"
                      "  J/L: Heading (±1°)\n"
                      "  I/K: Pitch (±1°)\n\n"
                      "Number Keys: Cardinal Directions\n"
                      "R: Reset to 0°/0°");
    lv_obj_set_style_text_font(label_controls, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_controls, lv_color_make(200, 200, 200), 0);
    lv_obj_set_grid_cell(label_controls, LV_GRID_ALIGN_START, 0, 8, LV_GRID_ALIGN_START, 4, 6);

    return page;
}

static void page_osd_test_on_roller(uint8_t key) {
    // Use roller for heading adjustment
    if (key == DIAL_KEY_UP) {
        test_heading = (test_heading + 1) % 360;
    } else if (key == DIAL_KEY_DOWN) {
        test_heading = (test_heading - 1 + 360) % 360;
    }
}

static void page_osd_test_on_click(uint8_t key, int sel) {
    // Click to toggle OSD elements on/off
    // Toggle compass visibility
    g_setting.osd.element[OSD_GOGGLE_HEAD_TRACKER_COMPASS].show =
        !g_setting.osd.element[OSD_GOGGLE_HEAD_TRACKER_COMPASS].show;

    // Toggle altitude visibility
    g_setting.osd.element[OSD_GOGGLE_HEAD_TRACKER_ALTITUDE].show =
        !g_setting.osd.element[OSD_GOGGLE_HEAD_TRACKER_ALTITUDE].show;
}

// Public keyboard handler function (called from input_device)
void page_osd_test_handle_keyboard(int key_code) {
#ifdef EMULATOR_BUILD
    switch (key_code) {
    // Arrow keys via WASD (already handled by SDL)
    // Additional keyboard controls
    case SDLK_LEFT:
        test_heading = (test_heading - 5 + 360) % 360;
        break;
    case SDLK_RIGHT:
        test_heading = (test_heading + 5) % 360;
        break;
    case SDLK_UP:
        test_pitch = (test_pitch + 5 > 90) ? 90 : test_pitch + 5;
        break;
    case SDLK_DOWN:
        test_pitch = (test_pitch - 5 < -90) ? -90 : test_pitch - 5;
        break;

    // Number keys for cardinal directions
    case SDLK_1:
        test_heading = 0; // North
        test_pitch = 0;
        break;
    case SDLK_2:
        test_heading = 45; // NE
        test_pitch = 0;
        break;
    case SDLK_3:
        test_heading = 90; // East
        test_pitch = 0;
        break;
    case SDLK_4:
        test_heading = 135; // SE
        test_pitch = 0;
        break;
    case SDLK_5:
        test_heading = 180; // South
        test_pitch = 0;
        break;
    case SDLK_6:
        test_heading = 225; // SW
        test_pitch = 0;
        break;
    case SDLK_7:
        test_heading = 270; // West
        test_pitch = 0;
        break;
    case SDLK_8:
        test_heading = 315; // NW
        test_pitch = 0;
        break;

    // Reset
    case SDLK_r:
        test_heading = 0;
        test_pitch = 0;
        break;

    // Fine adjustments with I/J/K/L
    case SDLK_j:
        test_heading = (test_heading - 1 + 360) % 360;
        break;
    case SDLK_l:
        test_heading = (test_heading + 1) % 360;
        break;
    case SDLK_i:
        test_pitch = (test_pitch + 1 > 90) ? 90 : test_pitch + 1;
        break;
    case SDLK_k:
        test_pitch = (test_pitch - 1 < -90) ? -90 : test_pitch - 1;
        break;
    }
#else
    // On real hardware, keyboard input not available
    // Use roller controls instead (handled in page_osd_test_on_roller)
    (void)key_code;
#endif
}

static void page_osd_test_timer(struct _lv_timer_t *timer) {
    int16_t heading, pitch;

    // Check if head tracker is enabled, use its values
    if (g_setting.ht.enable) {
        // Get actual head tracker values
        float pan = ht_get_pan_angle();
        float tilt = ht_get_tilt_angle();

        // Convert to 0-360 for heading and -90 to 90 for pitch
        heading = (int16_t)pan;
        if (heading < 0)
            heading += 360;
        pitch = (int16_t)tilt;

        // Update test values for keyboard override
        test_heading = heading;
        test_pitch = pitch;
    } else {
        // Use keyboard-controlled test values
        heading = test_heading;
        pitch = test_pitch;
    }

    // Update display labels
    lv_label_set_text_fmt(label_heading, "Heading: %d° %s", heading,
                          g_setting.ht.enable ? "(HT)" : "(KB)");
    lv_label_set_text_fmt(label_pitch, "Pitch: %d° %s", pitch,
                          g_setting.ht.enable ? "(HT)" : "(KB)");

    // Draw OSD elements with current values
    osd_head_tracker_compass_draw(heading);
    osd_head_tracker_altitude_draw(pitch);
}

static void page_osd_test_enter() {
    LOGD("page_osd_test_enter");

    // Enable OSD elements for testing
    g_setting.osd.element[OSD_GOGGLE_HEAD_TRACKER_COMPASS].show = true;
    g_setting.osd.element[OSD_GOGGLE_HEAD_TRACKER_ALTITUDE].show = true;

    // Make sure OSD layer is visible
    osd_show(true);

    // Create update timer (20 FPS)
    timer = lv_timer_create(page_osd_test_timer, 50, NULL);
}

static void page_osd_test_exit() {
    LOGD("page_osd_test_exit");

    // Clean up timer
    if (timer) {
        lv_timer_del(timer);
        timer = NULL;
    }

    // Optionally hide OSD elements on exit
    // g_setting.osd.element[OSD_GOGGLE_HEAD_TRACKER_COMPASS].show = false;
    // g_setting.osd.element[OSD_GOGGLE_HEAD_TRACKER_ALTITUDE].show = false;
}

page_pack_t pp_osd_test = {
    .p_arr = {
        .cur = 0,
        .max = 0,
    },
    .name = "OSD Test",
    .create = page_osd_test_create,
    .enter = page_osd_test_enter,
    .exit = page_osd_test_exit,
    .on_created = NULL,
    .on_update = NULL,
    .on_roller = page_osd_test_on_roller,
    .on_click = page_osd_test_on_click,
    .on_right_button = NULL,
};
