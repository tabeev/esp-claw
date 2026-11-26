/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "system_ui_private.h"

#include <stdio.h>
#include <string.h>

#include "display_service.h"
#include "esp_check.h"

#define SYSTEM_UI_HOME_TITLE_Y_PCT 20
#define SYSTEM_UI_HOME_AGENT_Y_PCT 68
#define SYSTEM_UI_HOME_TAGLINE_GAP_PX 8
#define SYSTEM_UI_HOME_AGENT_MIN_GAP_PX 34
#define SYSTEM_UI_HOME_TITLE_SHADOW_PX 3
#define SYSTEM_UI_HOME_STRIPE_COUNT 3
#define SYSTEM_UI_HOME_STRIPE_GAP_PX 3
#define SYSTEM_UI_HOME_STRIPE_BASE_PX 6
#define SYSTEM_UI_HOME_FALLBACK_TITLE_H 56
#define SYSTEM_UI_HOME_TITLE_TEXT "BVTS"
#define SYSTEM_UI_HOME_TAGLINE_TEXT "всем тем кто здесь"

typedef struct {
    int32_t pad;
    int32_t header_y;
    int32_t status_h;
    int32_t content_w;
    int32_t title_y;
    int32_t tagline_y;
    int32_t agent_y;
    int32_t stripe_h;
} system_ui_home_layout_t;

static int32_t system_ui_font_height(const lv_font_t *font, int32_t fallback)
{
    return font ? (int32_t)lv_font_get_line_height(font) : fallback;
}

static system_ui_home_layout_t system_ui_home_layout(void)
{
    int32_t width = (int32_t)s_ui.width;
    int32_t height = (int32_t)s_ui.height;
    int32_t short_side = system_ui_short_side_from(s_ui.width, s_ui.height);
    int32_t pad = system_ui_clamp_i32(short_side / 16, 14, 30);
    int32_t title_h = system_ui_font_height(s_ui.title_font, SYSTEM_UI_HOME_FALLBACK_TITLE_H);
    int32_t title_y = height * SYSTEM_UI_HOME_TITLE_Y_PCT / 100;
    int32_t tagline_y = title_y + title_h + SYSTEM_UI_HOME_TAGLINE_GAP_PX;
    int32_t tagline_h = system_ui_font_height(s_ui.tagline_font ? s_ui.tagline_font : s_ui.font, 20);
    int32_t stripe_h = system_ui_clamp_i32(short_side / 40, 3, SYSTEM_UI_HOME_STRIPE_BASE_PX);
    int32_t stripes_h = SYSTEM_UI_HOME_STRIPE_COUNT * (stripe_h + SYSTEM_UI_HOME_STRIPE_GAP_PX);
    int32_t agent_y = system_ui_max_i32(height * SYSTEM_UI_HOME_AGENT_Y_PCT / 100,
                                        tagline_y + tagline_h + SYSTEM_UI_HOME_AGENT_MIN_GAP_PX - 20);

    return (system_ui_home_layout_t) {
        .pad = pad,
        .header_y = system_ui_clamp_i32(short_side / 22, 10, 20),
        .status_h = system_ui_clamp_i32(short_side / 9, 28, 42),
        .content_w = width - pad * 2,
        .title_y = title_y,
        .tagline_y = tagline_y,
        .agent_y = system_ui_clamp_i32(agent_y, 0, height - stripes_h - 40),
        .stripe_h = stripe_h,
    };
}

#define SYSTEM_UI_CHIP_PAD_X 10
#define SYSTEM_UI_CHIP_PAD_Y 4
#define SYSTEM_UI_CHIP_GAP 6
#define SYSTEM_UI_CHIP_DOT_SIZE 8
#define SYSTEM_UI_AGENT_CHIP_PAD_X 16
#define SYSTEM_UI_AGENT_CHIP_PAD_Y 6
#define SYSTEM_UI_AGENT_CHIP_DOT_SIZE 10
#define SYSTEM_UI_AGENT_CHIP_BORDER_PX 2
#define SYSTEM_UI_PULSE_MS 700
#define SYSTEM_UI_CHIP_BAR_GAP 8
#define SYSTEM_UI_WIFI_TEXT "Wi-Fi"
#define SYSTEM_UI_AP_TEXT "AP"

static const char *system_ui_agent_default_text(system_ui_agent_state_t state)
{
    switch (state) {
    case SYSTEM_UI_AGENT_PREPARING:
        return "Preparing";
    case SYSTEM_UI_AGENT_THINKING:
        return "Thinking";
    case SYSTEM_UI_AGENT_TOOL:
        return "Working";
    case SYSTEM_UI_AGENT_REPLYING:
        return "Replying";
    case SYSTEM_UI_AGENT_IDLE:
    default:
        return "Idle";
    }
}

static uint32_t system_ui_agent_color(system_ui_agent_state_t state)
{
    switch (state) {
    case SYSTEM_UI_AGENT_PREPARING:
        return SYSTEM_UI_COLOR_WARN;
    case SYSTEM_UI_AGENT_THINKING:
        return SYSTEM_UI_COLOR_THINK;
    case SYSTEM_UI_AGENT_TOOL:
        return SYSTEM_UI_COLOR_TOOL;
    case SYSTEM_UI_AGENT_REPLYING:
        return SYSTEM_UI_COLOR_OK;
    case SYSTEM_UI_AGENT_IDLE:
    default:
        return SYSTEM_UI_COLOR_FAINT;
    }
}

static esp_err_t system_ui_chip_create(lv_obj_t *parent, system_ui_chip_t *chip,
                                       int32_t pad_x, int32_t pad_y, int32_t dot_size)
{
    chip->obj = lv_obj_create(parent);
    ESP_RETURN_ON_FALSE(chip->obj != NULL, ESP_ERR_NO_MEM, SYSTEM_UI_TAG, "create chip failed");
    lv_obj_set_size(chip->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_radius(chip->obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(chip->obj, system_ui_color(SYSTEM_UI_COLOR_SURFACE), 0);
    lv_obj_set_style_bg_opa(chip->obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(chip->obj, system_ui_color(SYSTEM_UI_COLOR_LINE), 0);
    lv_obj_set_style_border_width(chip->obj, 1, 0);
    lv_obj_set_style_pad_hor(chip->obj, pad_x, 0);
    lv_obj_set_style_pad_ver(chip->obj, pad_y, 0);
    lv_obj_set_style_pad_column(chip->obj, SYSTEM_UI_CHIP_GAP, 0);
    lv_obj_set_flex_flow(chip->obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(chip->obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(chip->obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    chip->dot = lv_obj_create(chip->obj);
    ESP_RETURN_ON_FALSE(chip->dot != NULL, ESP_ERR_NO_MEM, SYSTEM_UI_TAG, "create chip dot failed");
    lv_obj_set_size(chip->dot, dot_size, dot_size);
    lv_obj_set_style_radius(chip->dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(chip->dot, 0, 0);
    lv_obj_set_style_bg_opa(chip->dot, LV_OPA_COVER, 0);
    lv_obj_clear_flag(chip->dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    chip->label = lv_label_create(chip->obj);
    ESP_RETURN_ON_FALSE(chip->label != NULL, ESP_ERR_NO_MEM, SYSTEM_UI_TAG, "create chip label failed");
    lv_label_set_long_mode(chip->label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(chip->label, system_ui_color(SYSTEM_UI_COLOR_TEXT), 0);
    system_ui_apply_font(chip->label);
    return ESP_OK;
}

static void system_ui_chip_set(system_ui_chip_t *chip, const char *text, uint32_t dot_color)
{
    if (!chip->obj) {
        return;
    }
    lv_label_set_text(chip->label, text);
    lv_obj_set_style_bg_color(chip->dot, system_ui_color(dot_color), 0);
}

static void system_ui_pulse_exec_cb(void *obj, int32_t value)
{
    lv_obj_set_style_bg_opa((lv_obj_t *)obj, (lv_opa_t)value, 0);
}

static void system_ui_home_update_agent_locked(void)
{
    system_ui_chip_t *chip = &s_ui.agent_chip;
    const char *text = s_ui.agent_text[0] ? s_ui.agent_text
                                          : system_ui_agent_default_text(s_ui.agent_state);
    uint32_t color = system_ui_agent_color(s_ui.agent_state);

    if (!chip->obj) {
        return;
    }
    system_ui_chip_set(chip, text, color);
    lv_obj_set_style_border_color(chip->obj,
                                  system_ui_color(s_ui.agent_state == SYSTEM_UI_AGENT_IDLE
                                                      ? SYSTEM_UI_COLOR_LINE : color), 0);

    lv_anim_delete(chip->dot, system_ui_pulse_exec_cb);
    lv_obj_set_style_bg_opa(chip->dot, LV_OPA_COVER, 0);
    if (s_ui.agent_state != SYSTEM_UI_AGENT_IDLE) {
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, chip->dot);
        lv_anim_set_exec_cb(&anim, system_ui_pulse_exec_cb);
        lv_anim_set_values(&anim, LV_OPA_30, LV_OPA_COVER);
        lv_anim_set_duration(&anim, SYSTEM_UI_PULSE_MS);
        lv_anim_set_reverse_duration(&anim, SYSTEM_UI_PULSE_MS);
        lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&anim);
    }
}

static void system_ui_home_update_status_locked(void)
{
    if (s_ui.sta_connected) {
        system_ui_chip_set(&s_ui.wifi_chip, SYSTEM_UI_WIFI_TEXT, SYSTEM_UI_COLOR_OK);
    } else if (s_ui.ap_ssid[0]) {
        system_ui_chip_set(&s_ui.wifi_chip, SYSTEM_UI_AP_TEXT, SYSTEM_UI_COLOR_WARN);
    } else {
        system_ui_chip_set(&s_ui.wifi_chip, SYSTEM_UI_WIFI_TEXT, SYSTEM_UI_COLOR_ERR);
    }

    if (s_ui.service_chip.obj) {
        if (s_ui.service_text[0]) {
            lv_obj_remove_flag(s_ui.service_chip.obj, LV_OBJ_FLAG_HIDDEN);
            system_ui_chip_set(&s_ui.service_chip, s_ui.service_text,
                               s_ui.service_ok ? SYSTEM_UI_COLOR_OK : SYSTEM_UI_COLOR_ERR);
        } else {
            lv_obj_add_flag(s_ui.service_chip.obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void system_ui_home_update_locked(void)
{
    system_ui_home_update_status_locked();
    system_ui_home_update_agent_locked();
}

static esp_err_t system_ui_home_create_status_bar_locked(const system_ui_home_layout_t *layout)
{
    s_ui.status_bar = lv_obj_create(s_ui.home_tile);
    ESP_RETURN_ON_FALSE(s_ui.status_bar != NULL, ESP_ERR_NO_MEM, SYSTEM_UI_TAG, "create status bar failed");
    lv_obj_set_size(s_ui.status_bar, (int32_t)s_ui.width - layout->pad * 2, layout->status_h);
    lv_obj_align(s_ui.status_bar, LV_ALIGN_TOP_MID, 0, layout->header_y - 2);
    lv_obj_set_style_bg_opa(s_ui.status_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_ui.status_bar, 0, 0);
    lv_obj_set_style_pad_all(s_ui.status_bar, 0, 0);
    lv_obj_set_style_pad_column(s_ui.status_bar, SYSTEM_UI_CHIP_BAR_GAP, 0);
    lv_obj_set_flex_flow(s_ui.status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_ui.status_bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(s_ui.status_bar, LV_OBJ_FLAG_SCROLLABLE);

    ESP_RETURN_ON_ERROR(system_ui_chip_create(s_ui.status_bar, &s_ui.wifi_chip, SYSTEM_UI_CHIP_PAD_X,
                                              SYSTEM_UI_CHIP_PAD_Y, SYSTEM_UI_CHIP_DOT_SIZE),
                        SYSTEM_UI_TAG, "create wifi chip failed");
    ESP_RETURN_ON_ERROR(system_ui_chip_create(s_ui.status_bar, &s_ui.service_chip, SYSTEM_UI_CHIP_PAD_X,
                                              SYSTEM_UI_CHIP_PAD_Y, SYSTEM_UI_CHIP_DOT_SIZE),
                        SYSTEM_UI_TAG, "create service chip failed");
    return ESP_OK;
}

static esp_err_t system_ui_home_create_stripes_locked(const system_ui_home_layout_t *layout)
{
    static const uint32_t colors[SYSTEM_UI_HOME_STRIPE_COUNT] = {
        SYSTEM_UI_COLOR_SUN, SYSTEM_UI_COLOR_NEON_PINK, SYSTEM_UI_COLOR_DUSK,
    };
    int32_t y = (int32_t)s_ui.height;

    /* Sunset stripes hug the bottom edge, growing thinner towards the horizon. */
    for (int32_t i = 0; i < SYSTEM_UI_HOME_STRIPE_COUNT; i++) {
        int32_t h = system_ui_max_i32(2, layout->stripe_h - i);
        lv_obj_t *bar = lv_obj_create(s_ui.home_tile);
        ESP_RETURN_ON_FALSE(bar != NULL, ESP_ERR_NO_MEM, SYSTEM_UI_TAG, "create stripe failed");
        y -= h;
        lv_obj_set_size(bar, (int32_t)s_ui.width, h);
        lv_obj_set_pos(bar, 0, y);
        lv_obj_set_style_radius(bar, 0, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_bg_color(bar, system_ui_color(colors[i]), 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        y -= SYSTEM_UI_HOME_STRIPE_GAP_PX;
    }
    return ESP_OK;
}

static lv_obj_t *system_ui_home_create_title_label_locked(const system_ui_home_layout_t *layout,
                                                          uint32_t color, int32_t offset)
{
    lv_obj_t *label = lv_label_create(s_ui.home_tile);
    if (!label) {
        return NULL;
    }
    lv_label_set_text(label, SYSTEM_UI_HOME_TITLE_TEXT);
    lv_obj_set_style_text_color(label, system_ui_color(color), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    system_ui_apply_title_font(label);
    lv_obj_align(label, LV_ALIGN_TOP_MID, offset, layout->title_y + offset);
    return label;
}

static esp_err_t system_ui_create_home_tile_locked(void)
{
    system_ui_home_layout_t layout = system_ui_home_layout();
    lv_dir_t dir = s_ui.launcher_app_count > 0 ? LV_DIR_RIGHT : LV_DIR_NONE;

    /* The title page is the first tile; launcher pages, when present, sit to the right. */
    s_ui.home_tile = lv_tileview_add_tile(s_ui.tileview, 0, 0, dir);
    ESP_RETURN_ON_FALSE(s_ui.home_tile != NULL, ESP_ERR_NO_MEM, SYSTEM_UI_TAG, "create home tile failed");
    lv_obj_set_style_bg_color(s_ui.home_tile, system_ui_color(SYSTEM_UI_COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_ui.home_tile, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ui.home_tile, 0, 0);
    lv_obj_set_style_pad_all(s_ui.home_tile, 0, 0);
    lv_obj_clear_flag(s_ui.home_tile, LV_OBJ_FLAG_SCROLLABLE);
    system_ui_apply_font(s_ui.home_tile);

    ESP_RETURN_ON_ERROR(system_ui_home_create_stripes_locked(&layout), SYSTEM_UI_TAG, "create stripes failed");
    ESP_RETURN_ON_ERROR(system_ui_home_create_status_bar_locked(&layout), SYSTEM_UI_TAG, "create status bar failed");

    /* Cyan drop shadow first so the pink title sits on top of it. */
    s_ui.title_shadow = system_ui_home_create_title_label_locked(&layout, SYSTEM_UI_COLOR_ACCENT,
                                                                 SYSTEM_UI_HOME_TITLE_SHADOW_PX);
    s_ui.title_label = system_ui_home_create_title_label_locked(&layout, SYSTEM_UI_COLOR_NEON_PINK, 0);
    ESP_RETURN_ON_FALSE(s_ui.title_shadow != NULL && s_ui.title_label != NULL, ESP_ERR_NO_MEM,
                        SYSTEM_UI_TAG, "create title failed");

    s_ui.tagline_label = lv_label_create(s_ui.home_tile);
    ESP_RETURN_ON_FALSE(s_ui.tagline_label != NULL, ESP_ERR_NO_MEM, SYSTEM_UI_TAG, "create tagline failed");
    lv_label_set_text(s_ui.tagline_label, SYSTEM_UI_HOME_TAGLINE_TEXT);
    lv_obj_set_width(s_ui.tagline_label, layout.content_w);
    lv_label_set_long_mode(s_ui.tagline_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_ui.tagline_label, system_ui_color(SYSTEM_UI_COLOR_ACCENT), 0);
    lv_obj_set_style_text_align(s_ui.tagline_label, LV_TEXT_ALIGN_CENTER, 0);
    system_ui_apply_tagline_font(s_ui.tagline_label);
    lv_obj_align(s_ui.tagline_label, LV_ALIGN_TOP_MID, 0, layout.tagline_y);

    ESP_RETURN_ON_ERROR(system_ui_chip_create(s_ui.home_tile, &s_ui.agent_chip, SYSTEM_UI_AGENT_CHIP_PAD_X,
                                              SYSTEM_UI_AGENT_CHIP_PAD_Y, SYSTEM_UI_AGENT_CHIP_DOT_SIZE),
                        SYSTEM_UI_TAG, "create agent chip failed");
    lv_obj_set_style_border_width(s_ui.agent_chip.obj, SYSTEM_UI_AGENT_CHIP_BORDER_PX, 0);
    lv_obj_align(s_ui.agent_chip.obj, LV_ALIGN_TOP_MID, 0, layout.agent_y);

    system_ui_home_update_locked();
    return ESP_OK;
}

esp_err_t system_ui_create_home_locked(void)
{
    ESP_RETURN_ON_ERROR(system_ui_launcher_load_locked(), SYSTEM_UI_TAG, "load launcher failed");

    s_ui.home_screen = lv_obj_create(NULL);
    ESP_RETURN_ON_FALSE(s_ui.home_screen != NULL, ESP_ERR_NO_MEM, SYSTEM_UI_TAG, "create home failed");
    lv_obj_set_style_bg_color(s_ui.home_screen, system_ui_color(SYSTEM_UI_COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_ui.home_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ui.home_screen, 0, 0);
    lv_obj_set_style_pad_all(s_ui.home_screen, 0, 0);
    lv_obj_clear_flag(s_ui.home_screen, LV_OBJ_FLAG_SCROLLABLE);

    s_ui.tileview = lv_tileview_create(s_ui.home_screen);
    ESP_RETURN_ON_FALSE(s_ui.tileview != NULL, ESP_ERR_NO_MEM, SYSTEM_UI_TAG, "create tileview failed");
    lv_obj_set_size(s_ui.tileview, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_ui.tileview, system_ui_color(SYSTEM_UI_COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_ui.tileview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ui.tileview, 0, 0);
    lv_obj_set_style_pad_all(s_ui.tileview, 0, 0);
    lv_obj_set_scrollbar_mode(s_ui.tileview, LV_SCROLLBAR_MODE_OFF);

    ESP_RETURN_ON_ERROR(system_ui_create_home_tile_locked(), SYSTEM_UI_TAG, "create home tile failed");
    ESP_RETURN_ON_ERROR(system_ui_launcher_create_pages_locked(), SYSTEM_UI_TAG, "create launcher pages failed");
    lv_tileview_set_tile(s_ui.tileview, s_ui.home_tile, LV_ANIM_OFF);
    system_ui_load_screen_locked(s_ui.home_screen);
    return ESP_OK;
}

esp_err_t system_ui_show_home(void)
{
    ESP_RETURN_ON_FALSE(s_ui.started && s_ui.home_screen, ESP_ERR_INVALID_STATE,
                        SYSTEM_UI_TAG, "runtime not started");
    ESP_RETURN_ON_FALSE(!display_service_has_exclusive_session(), ESP_ERR_INVALID_STATE,
                        SYSTEM_UI_TAG, "exclusive display session active");
    ESP_RETURN_ON_ERROR(system_ui_lock(), SYSTEM_UI_TAG, "lock failed");
    if (s_ui.tileview && s_ui.home_tile) {
        lv_tileview_set_tile(s_ui.tileview, s_ui.home_tile, LV_ANIM_OFF);
    }
    system_ui_load_screen_locked(s_ui.home_screen);
    system_ui_unlock();
    return ESP_OK;
}

esp_err_t system_ui_reload_home(void)
{
    ESP_RETURN_ON_FALSE(s_ui.started, ESP_ERR_INVALID_STATE,
                        SYSTEM_UI_TAG, "runtime not started");
    ESP_RETURN_ON_ERROR(system_ui_lock(), SYSTEM_UI_TAG, "lock failed");
    if (!s_ui.started) {
        system_ui_unlock();
        ESP_LOGE(SYSTEM_UI_TAG, "runtime stopped before home reload");
        return ESP_ERR_INVALID_STATE;
    }
    if (display_service_has_exclusive_session()) {
        /* Keep the current default screen valid until the exclusive owner releases it. */
        s_ui.home_reload_pending = true;
        system_ui_unlock();
        ESP_LOGI(SYSTEM_UI_TAG, "home reload deferred until exclusive display exits");
        return ESP_OK;
    }
    s_ui.home_reload_pending = false;
    system_ui_delete_home_locked();
    esp_err_t err = system_ui_create_home_locked();
    if (err == ESP_OK) {
        display_service_set_default_screen_locked(s_ui.home_screen);
    } else {
        system_ui_delete_home_locked();
        display_service_set_default_screen_locked(NULL);
    }
    system_ui_unlock();
    return err;
}

void system_ui_delete_home_locked(void)
{
    if (s_ui.home_screen) {
        lv_obj_delete(s_ui.home_screen);
    }
    system_ui_launcher_delete_locked();
    s_ui.home_screen = NULL;
    s_ui.home_tile = NULL;
    s_ui.tileview = NULL;
    s_ui.launcher_first_tile = NULL;
    s_ui.title_shadow = NULL;
    s_ui.title_label = NULL;
    s_ui.tagline_label = NULL;
    s_ui.status_bar = NULL;
    s_ui.wifi_chip = (system_ui_chip_t){0};
    s_ui.service_chip = (system_ui_chip_t){0};
    s_ui.agent_chip = (system_ui_chip_t){0};
}

esp_err_t system_ui_set_network_status(bool sta_connected, const char *ap_ssid)
{
    system_ui_work_event_t event = {
        .type = SYSTEM_UI_WORK_EVENT_NETWORK_STATUS,
    };

    /* Network callbacks run from the system event task, so keep this path non-blocking and let the UI event task take the LVGL lock. */
    ESP_RETURN_ON_FALSE(s_ui.started, ESP_ERR_INVALID_STATE, SYSTEM_UI_TAG, "runtime not started");
    event.generation = s_ui.generation;
    event.network_status.sta_connected = sta_connected;
    strlcpy(event.network_status.ap_ssid, ap_ssid ? ap_ssid : "", sizeof(event.network_status.ap_ssid));
    return system_ui_post_work_event(&event, pdMS_TO_TICKS(100));
}

esp_err_t system_ui_post_agent_state(system_ui_agent_state_t state, const char *text)
{
    system_ui_work_event_t event = {
        .type = SYSTEM_UI_WORK_EVENT_AGENT_STATE,
    };

    ESP_RETURN_ON_FALSE(s_ui.started, ESP_ERR_INVALID_STATE, SYSTEM_UI_TAG, "runtime not started");
    event.generation = s_ui.generation;
    event.agent_state.state = state;
    strlcpy(event.agent_state.text, text ? text : "", sizeof(event.agent_state.text));
    return system_ui_post_work_event(&event, 0);
}

esp_err_t system_ui_post_service_status(const char *name, bool ok)
{
    system_ui_work_event_t event = {
        .type = SYSTEM_UI_WORK_EVENT_SERVICE_STATUS,
    };

    ESP_RETURN_ON_FALSE(s_ui.started, ESP_ERR_INVALID_STATE, SYSTEM_UI_TAG, "runtime not started");
    event.generation = s_ui.generation;
    strlcpy(event.service_status.text, name ? name : "", sizeof(event.service_status.text));
    event.service_status.ok = ok;
    return system_ui_post_work_event(&event, 0);
}
