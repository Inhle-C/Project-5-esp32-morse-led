/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_timer.h"

#define TAG "MORSE_ADC"
#define ADC_CHANNEL ADC_CHANNEL_2  // Adjust to your pin
#define LIGHT_THRESHOLD 100      // Set based on your environment
#define UNIT_DURATION 200         // Base time unit in milliseconds


/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
//ADC1 Channels
#if CONFIG_IDF_TARGET_ESP32
#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_4
#define EXAMPLE_ADC1_CHAN1          ADC_CHANNEL_5
#else
#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_2
#define EXAMPLE_ADC1_CHAN1          ADC_CHANNEL_3
#endif

#if (SOC_ADC_PERIPH_NUM >= 2) && !CONFIG_IDF_TARGET_ESP32C3
/**
 * On ESP32C3, ADC2 is no longer supported, due to its HW limitation.
 * Search for errata on espressif website for more details.
 */
#define EXAMPLE_USE_ADC2            1
#endif

#if EXAMPLE_USE_ADC2
//ADC2 Channels
#if CONFIG_IDF_TARGET_ESP32
#define EXAMPLE_ADC2_CHAN0          ADC_CHANNEL_0
#else
#define EXAMPLE_ADC2_CHAN0          ADC_CHANNEL_0
#endif
#endif  //#if EXAMPLE_USE_ADC2

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12

static int adc_raw[2][10];
static int voltage[2][10];
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);

// Morse lookup table
const char *MORSE_TABLE[] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", // a-j
    "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-", // k-t
    "..-", "...-", ".--", "-..-", "-.--", "--..", "/",                  // u-z, space
    ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----.", "-----" // 1-0
};

const char ALPHABET[] = "abcdefghijklmnopqrstuvwxyz 1234567890";

void decode_morse(int durations[], int count) {
    char morse_code[200] = {0}; // Morse sequence buffer

    // Build Morse sequence from durations
    for (int i = 0; i < count; i++) {
        if (durations[i] >= 20 && durations[i] <= 150) {
            strcat(morse_code, "."); // Dot
        } else if (durations[i] > 150 && durations[i] <= 600) {
            strcat(morse_code, "-"); // Dash
        } else if (durations[i] > 600 && durations[i] <= 1500) {
            strcat(morse_code, " "); // Character gap
        } else if (durations[i] > 1500) {
            strcat(morse_code, " / "); // Word gap
        } else {
            ESP_LOGW(TAG, "Unrecognized duration: %d ms", durations[i]);
        }
    }

    ESP_LOGI(TAG, "Morse Code: %s", morse_code);

    // Translate Morse to text
    char decoded_message[200] = {0};
    char buffer[10]; // Temporary buffer for Morse characters
    int buffer_index = 0;
    int length = strlen(morse_code);

    for (int i = 0; i <= length; i++) {
        if (morse_code[i] == '.' || morse_code[i] == '-') {
            // Accumulate Morse characters
            buffer[buffer_index++] = morse_code[i];
        } else if (morse_code[i] == ' ' || morse_code[i] == '\0') {
            // End of a Morse character
            buffer[buffer_index] = '\0';
            buffer_index = 0;

            // Translate Morse character
            if (strlen(buffer) > 0) {
                bool found = false;
                for (int j = 0; j < sizeof(MORSE_TABLE) / sizeof(MORSE_TABLE[0]); j++) {
                    if (strcmp(buffer, MORSE_TABLE[j]) == 0) {
                        strncat(decoded_message, &ALPHABET[j], 1);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    ESP_LOGW(TAG, "Unknown Morse sequence: %s", buffer);
                }
            }

            // Handle spaces for word gaps
            if (morse_code[i + 1] == '/' && morse_code[i + 2] == ' ') {
                strncat(decoded_message, " ", sizeof(decoded_message) - strlen(decoded_message) - 1);
                i += 2; // Skip the "/ " sequence
            }
        }
    }

    ESP_LOGI(TAG, "Decoded Message: %s", decoded_message);
}
void app_main(void) {
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));
    ESP_LOGI(TAG, "ADC initialized successfully on channel %d", ADC_CHANNEL);

    int durations[200] = {0}; // Buffer for durations
    int duration_index = 0;
    bool led_on = false;
    int64_t on_start_time = 0;  // Time when LED turns ON
    int64_t off_start_time = 0; // Time when LED turns OFF

    while (1) {
        int adc_value = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_value));

        // Log ADC value for debugging
        ESP_LOGI(TAG, "ADC Value: %d", adc_value);

        if (adc_value > LIGHT_THRESHOLD) {
            if (!led_on) {
                // Transition from OFF to ON
                led_on = true;
                on_start_time = esp_timer_get_time();

                if (off_start_time > 0) {
                    int64_t off_duration = (on_start_time - off_start_time) / 1000;
                    if (off_duration >= 20) {
                        ESP_LOGI(TAG, "LED OFF duration: %lld ms", off_duration);

                        // Classify gaps
                        if (off_duration > 1000) {
                            durations[duration_index++] = 2000; // Word gap
                        } else if (off_duration > 500) {
                            durations[duration_index++] = 1000; // Character gap
                        } else if (off_duration > 100) {
                            durations[duration_index++] = 500; // Intra-character gap
                        }
                    }
                }
                ESP_LOGI(TAG, "LED turned ON");
            }
        } else {
            if (led_on) {
                // Transition from ON to OFF
                led_on = false;
                off_start_time = esp_timer_get_time();
                int64_t on_duration = (off_start_time - on_start_time) / 1000;

                if (on_duration >= 20) {
                    ESP_LOGI(TAG, "LED ON duration: %lld ms", on_duration);

                    // Classify pulses
                    if (on_duration < 250) {
                        durations[duration_index++] = 50; // Dot
                    } else if (on_duration < 1000) {
                        durations[duration_index++] = 200; // Dash
                    }
                }
            }
        }

        // Decode Morse if array is filled or timeout occurs
        if (duration_index > 0 && (esp_timer_get_time() - off_start_time > 3000000)) {
            ESP_LOGI(TAG, "Decoding Morse...");
            decode_morse(durations, duration_index);
            duration_index = 0; // Reset
        }

        // Sampling delay
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}