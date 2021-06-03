/******************************************************************************
* File Name:   mesh_cfg.c
*
* Description: This file shows the configuration for mesh sensor server model.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2021, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_mesh_models.h"
#include "wiced_bt_trace.h"
#include "wiced_bt_mesh_app.h"
#include "wiced_bt_cfg.h"
#include "mesh_cfg.h"


/*************************************************************************************
* Variables Definitions
*************************************************************************************/
/* variable to hold the sensor current value */
extern uint32_t mesh_sensor_sent_lux_value;
extern int8_t mesh_sensor_sent_temp_value;

uint8_t mesh_mfr_name[WICED_BT_MESH_PROPERTY_LEN_DEVICE_MANUFACTURER_NAME] = { 'I', 'n', 'f', 'i', 'n', 'e', 'o', 'n', 0 };
uint8_t mesh_model_num[WICED_BT_MESH_PROPERTY_LEN_DEVICE_MODEL_NUMBER]     = { '1', '2', '3', '4', 0, 0, 0, 0 };
uint8_t mesh_system_id[8]                                                  = { 0xbb, 0xb8, 0xa1, 0x80, 0x5f, 0x9f, 0x91, 0x71 };

// Optional setting for the sensors, the Total Device Runtime, in Time Hour 24 format
uint8_t mesh_sensor_als_setting_val[] = { 0x01, 0x00, 0x00 }; /*HH, MM, SS */
uint8_t mesh_sensor_temp_setting_val[] = { 0x01, 0x00, 0x00 };

wiced_bt_mesh_sensor_config_setting_t mesh_sensor_als_settings[] =
{
    {
        .setting_property_id = WICED_BT_MESH_PROPERTY_TOTAL_DEVICE_RUNTIME,
        .access              = WICED_BT_MESH_SENSOR_SETTING_READABLE_AND_WRITABLE,
        .value_len           = WICED_BT_MESH_PROPERTY_LEN_TOTAL_DEVICE_RUNTIME,
        .val                 = mesh_sensor_als_setting_val
    },
};

wiced_bt_mesh_sensor_config_setting_t mesh_sensor_temp_settings[] =
{
    {
        .setting_property_id = WICED_BT_MESH_PROPERTY_TOTAL_DEVICE_RUNTIME,
        .access              = WICED_BT_MESH_SENSOR_SETTING_READABLE_AND_WRITABLE,
        .value_len           = WICED_BT_MESH_PROPERTY_LEN_TOTAL_DEVICE_RUNTIME,
        .val                 = mesh_sensor_temp_setting_val
    },
};

wiced_bt_mesh_core_config_model_t mesh_element1_models[] =
{
    WICED_BT_MESH_DEVICE,
    WICED_BT_MESH_MODEL_SENSOR_SERVER,
};

wiced_bt_mesh_core_config_model_t mesh_element2_models[] =
{
    WICED_BT_MESH_MODEL_SENSOR_SERVER,
};

wiced_bt_mesh_core_config_sensor_t mesh_element1_sensors[] =
{
    {
        .property_id = WICED_BT_MESH_PROPERTY_PRESENT_AMBIENT_LIGHT_LEVEL,
        .prop_value_len = WICED_BT_MESH_PROPERTY_LEN_PRESENT_AMBIENT_LIGHT_LEVEL,
        .descriptor =
        {
            .positive_tolerance = MESH_ALS_SENSOR_POSITIVE_TOLERANCE,
            .negative_tolerance = MESH_ALS_SENSOR_NEGATIVE_TOLERANCE,
            .sampling_function  = MESH_ALS_SENSOR_SAMPLING_FUNCTION,
            .measurement_period = MESH_ALS_SENSOR_MEASUREMENT_PERIOD,
            .update_interval    = MESH_ALS_SENSOR_UPDATE_INTERVAL,
        },
        .data = (uint8_t *)&mesh_sensor_sent_lux_value,
        .cadence =
        {
            // Value 1 indicates that cadence does not change depending on the measurements
            .fast_cadence_period_divisor = 1,            // Value of the divisor
            .trigger_type_percentage     = WICED_FALSE,
            .trigger_delta_down          = 0,
            .trigger_delta_up            = 0,
            .min_interval                = (1 << 0x0C),  // ~4 seconds
            .fast_cadence_low            = 0,
            .fast_cadence_high           = 0,
        },
        .num_series     = 0,
        .series_columns = NULL,
        .num_settings   = 1,
        .settings       = mesh_sensor_als_settings,
    },
};


wiced_bt_mesh_core_config_sensor_t mesh_element2_sensors[] =
{

    {
        .property_id = WICED_BT_MESH_PROPERTY_PRESENT_AMBIENT_TEMPERATURE,
        .prop_value_len = WICED_BT_MESH_PROPERTY_LEN_PRESENT_AMBIENT_TEMPERATURE,
        .descriptor =
        {
            .positive_tolerance = MESH_TEMP_SENSOR_POSITIVE_TOLERANCE,
            .negative_tolerance = MESH_TEMP_SENSOR_NEGATIVE_TOLERANCE,
            .sampling_function  = MESH_TEMP_SENSOR_SAMPLING_FUNCTION,
            .measurement_period = MESH_TEMP_SENSOR_MEASUREMENT_PERIOD,
            .update_interval    = MESH_TEMP_SENSOR_UPDATE_INTERVAL,
        },
        .data = (uint8_t *)&mesh_sensor_sent_temp_value,
        .cadence =
        {
            // Value 1 indicates that cadence does not change depending on the measurements
            .fast_cadence_period_divisor = 1,            // Value of the divisor
            .trigger_type_percentage     = WICED_FALSE,
            .trigger_delta_down          = 0,
            .trigger_delta_up            = 0,
            .min_interval                = (1 << 0x0C),  // ~4 seconds
            .fast_cadence_low            = 0,
            .fast_cadence_high           = 0,
        },
        .num_series     = 0,
        .series_columns = NULL,
        .num_settings   = 1,
        .settings       = mesh_sensor_temp_settings,
    },

};


wiced_bt_mesh_core_config_element_t mesh_elements[] =
{
    {
        .location = MESH_ELEM_LOC_MAIN,                                  // location description as defined in the GATT Bluetooth Namespace Descriptors section of the Bluetooth SIG Assigned Numbers
        .default_transition_time = MESH_DEFAULT_TRANSITION_TIME_IN_MS,   // Default transition time for models of the element in milliseconds
        .onpowerup_state = WICED_BT_MESH_ON_POWER_UP_STATE_RESTORE,      // Default element behavior on power up
        .default_level = 0,                                              // Default value of the variable controlled on this element (for example power, lightness, temperature, hue...)
        .range_min = 1,                                                  // Minimum value of the variable controlled on this element (for example power, lightness, temperature, hue...)
        .range_max = 0xffff,                                             // Maximum value of the variable controlled on this element (for example power, lightness, temperature, hue...)
        .move_rollover = 0,                                              // If true when level gets to range_max during move operation, it switches to min, otherwise move stops.
        .properties_num = 0,                                             // Number of properties in the array models
        .properties = NULL,                                              // Array of properties in the element.
        .sensors_num = 1,                                                // Number of properties in the array models
        .sensors = mesh_element1_sensors,                                // Array of properties in the element.
        .models_num = sizeof(mesh_element1_models) / sizeof(wiced_bt_mesh_core_config_model_t),                               // Number of models in the array models
        .models = mesh_element1_models,                                  // Array of models located in that element. Model data is defined by structure wiced_bt_mesh_core_config_model_t
    },
    {
        .location = MESH_ELEM_LOC_MAIN,                                  // location description as defined in the GATT Bluetooth Namespace Descriptors section of the Bluetooth SIG Assigned Numbers
        .default_transition_time = MESH_DEFAULT_TRANSITION_TIME_IN_MS,   // Default transition time for models of the element in milliseconds
        .onpowerup_state = WICED_BT_MESH_ON_POWER_UP_STATE_RESTORE,      // Default element behavior on power up
        .default_level = 0,                                              // Default value of the variable controlled on this element (for example power, lightness, temperature, hue...)
        .range_min = 1,                                                  // Minimum value of the variable controlled on this element (for example power, lightness, temperature, hue...)
        .range_max = 0xffff,                                             // Maximum value of the variable controlled on this element (for example power, lightness, temperature, hue...)
        .move_rollover = 0,                                              // If true when level gets to range_max during move operation, it switches to min, otherwise move stops.
        .properties_num = 0,                                             // Number of properties in the array models
        .properties = NULL,                                              // Array of properties in the element.
        .sensors_num = 1,                                                // Number of properties in the array models
        .sensors = mesh_element2_sensors,                                // Array of properties in the element.
        .models_num = sizeof(mesh_element2_models) / sizeof(wiced_bt_mesh_core_config_model_t),                               // Number of models in the array models
        .models = mesh_element2_models,                                  // Array of models located in that element. Model data is defined by structure wiced_bt_mesh_core_config_model_t
    },
};

wiced_bt_mesh_core_config_t  mesh_config =
{
    .company_id         = MESH_COMPANY_ID,                  // Company identifier assigned by the Bluetooth SIG
    .product_id         = MESH_PID,                                 // Vendor-assigned product identifier
    .vendor_id          = MESH_VID,                                 // Vendor-assigned product version identifier
    .replay_cache_size  = MESH_CACHE_REPLAY_SIZE,                   // Number of replay protection entries, i.e. maximum number of mesh devices that can send application messages to this device.
    .features           = WICED_BT_MESH_CORE_FEATURE_BIT_FRIEND | WICED_BT_MESH_CORE_FEATURE_BIT_RELAY | WICED_BT_MESH_CORE_FEATURE_BIT_GATT_PROXY_SERVER,   // In Friend mode support friend, relay
    .friend_cfg         =                                           // Configuration of the Friend Feature(Receive Window in Ms, messages cache)
    {
        .receive_window        = 20,
        .cache_buf_len         = 300,                               // Length of the buffer for the cache
        .max_lpn_num           = 4                                  // Max number of Low Power Nodes with established friendship. Must be > 0 if Friend feature is supported.
    },
    .low_power          =                                           // Configuration of the Low Power Feature
    {
        .rssi_factor           = 0,                                 // contribution of the RSSI measured by the Friend node used in Friend Offer Delay calculations.
        .receive_window_factor = 0,                                 // contribution of the supported Receive Window used in Friend Offer Delay calculations.
        .min_cache_size_log    = 0,                                 // minimum number of messages that the Friend node can store in its Friend Cache.
        .receive_delay         = 0,                                 // Receive delay in 1 ms units to be requested by the Low Power node.
        .poll_timeout          = 0                                  // Poll timeout in 100ms units to be requested by the Low Power node.
    },
    .gatt_client_only          = WICED_FALSE,                       // Can connect to mesh over GATT or ADV
    .elements_num  = (uint8_t)(sizeof(mesh_elements) / sizeof(mesh_elements[0])),   // number of elements on this device
    .elements      = mesh_elements                                  // Array of elements for this device
};


/*END of FILE */
