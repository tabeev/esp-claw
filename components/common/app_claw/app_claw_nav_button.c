/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "app_claw_nav_button.h"

#include "sdkconfig.h"

#include "button_gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "iot_button.h"
#include "system_ui.h"

#define NAV_BUTTON_ACTIVE_LEVEL 0
#define NAV_BUTTON_LONG_PRESS_MS 600

static const char *TAG = "app_claw_nav";

static button_handle_t s_nav_button;

static void nav_button_cb(void *button_handle, void *user_data)
{
    system_ui_nav_t nav = (system_ui_nav_t)(uintptr_t)user_data;

    (void)button_handle;
    if (system_ui_navigate(nav) != ESP_OK) {
        ESP_LOGW(TAG, "UI navigation event dropped");
    }
}

esp_err_t app_claw_nav_button_start(void)
{
#if CONFIG_APP_CLAW_NAV_BUTTON_GPIO >= 0
    const button_config_t button_config = {
        .long_press_time = NAV_BUTTON_LONG_PRESS_MS,
    };
    const button_gpio_config_t gpio_config = {
        .gpio_num = CONFIG_APP_CLAW_NAV_BUTTON_GPIO,
        .active_level = NAV_BUTTON_ACTIVE_LEVEL,
    };

    if (s_nav_button) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(iot_button_new_gpio_device(&button_config, &gpio_config, &s_nav_button),
                        TAG, "create nav button failed");
    ESP_RETURN_ON_ERROR(iot_button_register_cb(s_nav_button, BUTTON_SINGLE_CLICK, NULL,
                                               nav_button_cb, (void *)(uintptr_t)SYSTEM_UI_NAV_NEXT),
                        TAG, "register click failed");
    ESP_RETURN_ON_ERROR(iot_button_register_cb(s_nav_button, BUTTON_LONG_PRESS_START, NULL,
                                               nav_button_cb, (void *)(uintptr_t)SYSTEM_UI_NAV_SELECT),
                        TAG, "register long press failed");
    ESP_LOGI(TAG, "UI navigation button on GPIO%d", CONFIG_APP_CLAW_NAV_BUTTON_GPIO);
#endif
    return ESP_OK;
}
