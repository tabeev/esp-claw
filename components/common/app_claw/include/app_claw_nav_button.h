/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Start the system UI navigation button (no-op unless APP_CLAW_NAV_BUTTON_GPIO >= 0). */
esp_err_t app_claw_nav_button_start(void);

#ifdef __cplusplus
}
#endif
