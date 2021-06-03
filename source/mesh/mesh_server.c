/******************************************************************************
* File Name:   mesh_server.c
*
* Description: This file shows implementation of sensor model.
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

#include "wiced_bt_uuid.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_mesh_models.h"
#include "wiced_bt_trace.h"
#include "wiced_hal_nvram.h"
#include "mesh_cfg.h"
#include "mesh_server.h"
#include "sensors.h"

/******************************************************************************
 *                              Macros
 ******************************************************************************/
#define MESH_SENSOR_ALS_CADENCE_NVRAM_ID        WICED_NVRAM_VSID_START
#define MESH_SENSOR_TEMP_CADENCE_NVRAM_ID        WICED_NVRAM_VSID_START + 24u

 /* PAYLAOD LEN = SIZE(PROPERTY_ID) + SIZE(PROPERTY_LEN) + SIZE(SENSOR_VALUE) */
#define MESH_SENSOR_ALS_PAYLOAD_LENGTH            MESH_ALS_SENSOR_VALUE_LEN + 4
#define MESH_SENSOR_TEMP_PAYLOAD_LENGTH            MESH_TEMP_SENSOR_VALUE_LEN + 4

/******************************************************************************
 *                          Function Prototypes
 ******************************************************************************/
static void mesh_sensor_publish_als_timer_callback(TIMER_PARAM_TYPE arg);
static void mesh_sensor_publish_temp_timer_callback(TIMER_PARAM_TYPE arg);
static void mesh_sensor_server_restart_timer(uint8_t element_idx, wiced_bt_mesh_core_config_sensor_t *p_sensor);
static void mesh_sensor_server_report_handler(uint16_t event, uint8_t element_idx, void *p_get, void *p_ref_data);
static void mesh_sensor_server_process_cadence_changed(uint8_t element_idx, uint16_t property_id);
static void mesh_sensor_server_process_setting_changed(uint8_t element_idx, uint16_t property_id, uint16_t setting_property_id);
static void mesh_sensor_server_config_change_handler(uint8_t element_idx, uint16_t event, uint16_t property_id, uint16_t setting_prop_id);
static void mesh_sensor_server_status_changed(uint8_t element_idx, uint8_t *p_data, uint32_t length);
static wiced_bool_t mesh_app_notify_period_set(uint8_t element_idx, uint16_t company_id, uint16_t model_id, uint32_t period);
static void mesh_app_factory_reset(void);
extern void mesh_app_init(wiced_bool_t is_provisioned);

/******************************************************************************
 *                          Variables Definitions
 ******************************************************************************/
extern wiced_bt_cfg_settings_t wiced_bt_cfg_settings;

// Present Ambient Light Sensor property.
uint32_t      mesh_sensor_current_lux_value = 0;        // ambient light value
uint32_t      mesh_sensor_sent_lux_value = 0;
uint32_t      mesh_sensor_sent_lux_time;                // time stamp when lux was published
uint32_t      mesh_sensor_publish_lux_period = 0;       // publish period in msec
uint32_t      mesh_sensor_fast_publish_lux_period = 0;  // publish period in msec when values are outside of limit


// Present Ambient Temperature Sensor property.
int8_t        mesh_sensor_current_temp_value = 0;        // ambient temperature value
int8_t        mesh_sensor_sent_temp_value = 0;
uint32_t      mesh_sensor_sent_temp_time;                // time stamp when temp was published
uint32_t      mesh_sensor_publish_temp_period = 0;       // publish period in msec
uint32_t      mesh_sensor_fast_publish_temp_period = 0;  // publish period in msec when values are outside of limit


wiced_timer_t mesh_sensor_cadence_als_timer, mesh_sensor_cadence_temp_timer;

/*
 * Mesh application library will call into application functions if provided by the application.
 */
wiced_bt_mesh_app_func_table_t wiced_bt_mesh_app_func_table =
{
    mesh_app_init,               // application initialization
    NULL,                       // Default SDK platform button processing
    NULL,                       // GATT connection status
    NULL,                       // attention processing
    mesh_app_notify_period_set, // notify period set
    NULL,                       // WICED HCI command
    NULL,                       // LPN sleep
    mesh_app_factory_reset      // factory reset
};

/******************************************************************************
*                                Function Definitions
******************************************************************************/


/**
 * Function         mesh_sensor_init_value
 *
 *                  Read and initialize the sensor values
 *
 * @return                        : None;
 */
void mesh_sensor_init_value()
{
    wiced_result_t  result = WICED_SUCCESS;

    uint32_t cur_time = wiced_bt_mesh_core_get_tick_count();
    wiced_bt_mesh_core_config_sensor_t *p_sensor = NULL;

    /* Read ambient light level */
    mesh_sensor_current_lux_value = sensor_get_light_level();

    /* Read temperature value */
    mesh_sensor_current_temp_value = sensor_get_temperature();

    mesh_sensor_sent_lux_value = mesh_sensor_current_lux_value;
    mesh_sensor_sent_temp_value = mesh_sensor_current_temp_value;
    mesh_sensor_sent_lux_time = mesh_sensor_sent_temp_time = cur_time;

    p_sensor = &mesh_config.elements[MESH_ALS_SENSOR_ELEMENT_INDEX].sensors[0];
    //restore the cadence for ambient light sensor from NVRAM
    wiced_hal_read_nvram(MESH_SENSOR_ALS_CADENCE_NVRAM_ID, sizeof(wiced_bt_mesh_sensor_config_cadence_t), (uint8_t*)(&p_sensor->cadence), &result);

    p_sensor = &mesh_config.elements[MESH_TEMP_SENSOR_ELEMENT_INDEX].sensors[0];
    //restore the cadence for temperature sensor from NVRAM
    wiced_hal_read_nvram(MESH_SENSOR_TEMP_CADENCE_NVRAM_ID, sizeof(wiced_bt_mesh_sensor_config_cadence_t), (uint8_t*)(&p_sensor->cadence), &result);

    WICED_BT_TRACE("Mesh Sensor values are initialized!\n");
}


/**
 * Function         mesh_sensor_cadence_init_als_timer
 *
 *                  Initialize the cadence timer for ALS sensor
 *
 * @return                        : None;
 */
void mesh_sensor_cadence_init_als_timer()
{
    wiced_result_t result = WICED_ERROR;
    // Initialize the cadence timer for als sensor.  Need a timer for each element because each sensor model can be
    // configured for different publication period.
    result = wiced_init_timer(&mesh_sensor_cadence_als_timer, &mesh_sensor_publish_als_timer_callback,
                    (TIMER_PARAM_TYPE)&mesh_config.elements[MESH_ALS_SENSOR_ELEMENT_INDEX].sensors[0],
                    WICED_MILLI_SECONDS_TIMER);

    if(WICED_SUCCESS == result)
    {
        WICED_BT_TRACE("Cadence timer initialization for ALS sensor done!\n");
    }
    else
    {
        WICED_BT_TRACE("Cadence timer initialization failed for ALS sensor!\n");
    }
}


/**
 * Function         mesh_sensor_cadence_init_temp_timer
 *
 *                  Initialize the cadence timer for thermistor sensor
 *
 * @return                        : None;
 */
void mesh_sensor_cadence_init_temp_timer()
{
    wiced_result_t result = WICED_ERROR;

    // Initialize the cadence timer for thermistor.
    result = wiced_init_timer(&mesh_sensor_cadence_temp_timer, &mesh_sensor_publish_temp_timer_callback,
                    (TIMER_PARAM_TYPE)&mesh_config.elements[MESH_TEMP_SENSOR_ELEMENT_INDEX].sensors[0],
                    WICED_MILLI_SECONDS_TIMER);
    if(WICED_SUCCESS == result)
    {
        WICED_BT_TRACE("Cadence time initialization for thermistor done!\n");
    }
    else
    {
        WICED_BT_TRACE("Cadence timer initialization failed for thermistor!\n");
    }

}


/**
 * Function         mesh_sensor_server_init_model
 *
 *                  Initialize the sensor server model
 *
 * @param[in] is_provisioned    : Provision status
 * @return                        : None;
 */
void mesh_sensor_server_init_model(wiced_bool_t is_provisioned)
{

    // Initialize the sensor server model for ALS sensor.
    wiced_bt_mesh_model_sensor_server_init(MESH_ALS_SENSOR_ELEMENT_INDEX, mesh_sensor_server_report_handler,
                                            mesh_sensor_server_config_change_handler, is_provisioned);
    // Initialize the sensor server model for thermistor.
    wiced_bt_mesh_model_sensor_server_init(MESH_TEMP_SENSOR_ELEMENT_INDEX, mesh_sensor_server_report_handler,
                                            mesh_sensor_server_config_change_handler, is_provisioned);
    WICED_BT_TRACE("Sensor model initialization done!\n");
}


/**
 * Function         mesh_sensor_server_restart_timer
 *
 *                  Start periodic timer depending on the publication period, fast cadence divisor
 *                  and minimum interval.
 *
 * @param[in] element_idx       : Element id value
 * @param[in] p_sensor          : Sensor config value
 * @return                      : None
 */
void mesh_sensor_server_restart_timer(uint8_t element_idx, wiced_bt_mesh_core_config_sensor_t *p_sensor)
{
    // If there are no specific cadence settings, publish every publish period.
    uint32_t timeout = 0;

    if(MESH_ALS_SENSOR_ELEMENT_INDEX == element_idx)
    {
        timeout = mesh_sensor_publish_lux_period;
        wiced_stop_timer(&mesh_sensor_cadence_als_timer);
        if (0 == mesh_sensor_publish_lux_period)
        {
            // The sensor is not interrupt driven.  If client configured sensor to send notification when
            // the value changes, we will need to check periodically if the condition has been satisfied.
            // The cadence.min_interval can be used because we do not need to send data more often than that.
            if ((p_sensor->cadence.min_interval != 0) &&
                ((p_sensor->cadence.trigger_delta_up != 0) || (p_sensor->cadence.trigger_delta_down != 0)))
            {
                timeout = p_sensor->cadence.min_interval;
            }
            else
            {
                WICED_BT_TRACE("Ambient light sensor restart timer period:%d\n", mesh_sensor_publish_lux_period);
                return;
            }
        }
        else
        {
            // If fast cadence period divisor is set, we need to check light level more
            // often than publication period.  Publish if measurement is in specified range
            if (1 < p_sensor->cadence.fast_cadence_period_divisor)
            {
                mesh_sensor_fast_publish_lux_period = mesh_sensor_publish_lux_period / p_sensor->cadence.fast_cadence_period_divisor;
                timeout = mesh_sensor_fast_publish_lux_period;
            }
            else
            {
                mesh_sensor_fast_publish_lux_period = 0;
            }
            // The sensor is not interrupt driven.  If client configured sensor to send notification when
            // the value changes, we may need to check value more often not to miss the trigger.
            // The cadence.min_interval can be used because we do not need to send data more often than that.
            if ((p_sensor->cadence.min_interval < timeout) &&
                ((0 != p_sensor->cadence.trigger_delta_up) || (0 != p_sensor->cadence.trigger_delta_down)))
            {
                timeout = p_sensor->cadence.min_interval;
            }
        }

        WICED_BT_TRACE("Ambient light sensor restart timer timeout:%d\n", timeout);
        wiced_start_timer(&mesh_sensor_cadence_als_timer, timeout);
    }

    if(MESH_TEMP_SENSOR_ELEMENT_INDEX == element_idx)
    {
        timeout = mesh_sensor_publish_temp_period;
        wiced_stop_timer(&mesh_sensor_cadence_temp_timer);
        if (0 == mesh_sensor_publish_temp_period)
        {
            // The thermistor is not interrupt driven.  If client configured sensor to send notification when
            // the value changes, we will need to check periodically if the condition has been satisfied.
            // The cadence.min_interval can be used because we do not need to send data more often than that.
            if ((0 != p_sensor->cadence.min_interval) &&
                ((0 != p_sensor->cadence.trigger_delta_up) || (0 != p_sensor->cadence.trigger_delta_down)))
            {
                timeout = p_sensor->cadence.min_interval;
            }
            else
            {
                WICED_BT_TRACE("Temperature sensor restart timer period:%d\n", mesh_sensor_publish_temp_period);
                return;
            }
        }
        else
        {
            // If fast cadence period divisor is set, we need to check light level more
            // often than publication period.  Publish if measurement is in specified range
            if (1 < p_sensor->cadence.fast_cadence_period_divisor)
            {
                mesh_sensor_fast_publish_temp_period = mesh_sensor_publish_temp_period / p_sensor->cadence.fast_cadence_period_divisor;
                timeout = mesh_sensor_fast_publish_temp_period;
            }
            else
            {
                mesh_sensor_fast_publish_temp_period = 0;
            }
            // The thermistor is not interrupt driven.  If client configured sensor to send notification when
            // the value changes, we may need to check value more often not to miss the trigger.
            // The cadence.min_interval can be used because we do not need to send data more often than that.
            if ((p_sensor->cadence.min_interval < timeout) &&
                ((0 != p_sensor->cadence.trigger_delta_up) || (0 != p_sensor->cadence.trigger_delta_down)))
            {
                timeout = p_sensor->cadence.min_interval;
            }
        }

        WICED_BT_TRACE("Temperature sensor restart timer timeout:%d\n", timeout);
        wiced_start_timer(&mesh_sensor_cadence_temp_timer, timeout);
    }

}


/**
 * Function         mesh_sensor_server_config_change_handler
 *
 *                  Process the configuration changes set by the Sensor Client.
 *
 * @param[in] element_idx       : Element id value
 * @param[in] event             : Event value
 * @param[in] property_id       : property_id value
 * @param[in] property_id       : new setting property_id value
 * @return                      : None
 */
void mesh_sensor_server_config_change_handler(uint8_t element_idx, uint16_t event, uint16_t property_id, uint16_t setting_property_id)
{

    WICED_BT_TRACE("Mesh sensor server config change handler message: %d\n", event);

    switch (event)
    {

    case WICED_BT_MESH_SENSOR_CADENCE_SET:
        mesh_sensor_server_process_cadence_changed(element_idx, property_id);
        break;

    case WICED_BT_MESH_SENSOR_SETTING_SET:
        mesh_sensor_server_process_setting_changed(element_idx, property_id, setting_property_id);
        break;
    default:
        WICED_BT_TRACE("Unknown event\n");
        break;
    }
}


/**
 * Function         mesh_sensor_server_report_handler
 *
 *                  Process get request from Sensor Client and respond with sensor data.
 *
 * @param[in] event             : Event value
 * @param[in] element_idx       : Element id value
 * @param[out] p_get            : get the sensor data
 * @param[in] p_ref_data        : reference data
 * @return                      : None
 */
void mesh_sensor_server_report_handler(uint16_t event, uint8_t element_idx, void *p_get, void *p_ref_data)
{
    wiced_bt_mesh_sensor_get_t *p_sensor_get = (wiced_bt_mesh_sensor_get_t *)p_get;
    WICED_BT_TRACE("Mesh sensor server report handler message: %d\n", event);

    switch (event)
    {
    case WICED_BT_MESH_SENSOR_GET:

        if(MESH_ALS_SENSOR_ELEMENT_INDEX == element_idx)
        {
            mesh_sensor_sent_lux_value = sensor_get_light_level();
            WICED_BT_TRACE("Ambient light level:%d lux\n", mesh_sensor_sent_lux_value);
        }
        if(MESH_TEMP_SENSOR_ELEMENT_INDEX == element_idx)
        {
            mesh_sensor_sent_temp_value = sensor_get_temperature();
            WICED_BT_TRACE("Temperature value:%d.%d DegC\n", (mesh_sensor_sent_temp_value/2),(mesh_sensor_sent_temp_value%2)*5);
        }

        // tell mesh models library that data is ready to be shipped out, the library will get data from mesh_config
        wiced_bt_mesh_model_sensor_server_data(element_idx, p_sensor_get->property_id, p_ref_data);
        break;
    default:
        WICED_BT_TRACE("Unknown event\n");
        break;
    }
}


/**
 * Function         mesh_sensor_server_process_cadence_changed
 *
 *                  Process the cadence change.
 *
 * @param[in] element_idx       : Element id value
 * @param[in] property_id       : Property id value
 * @return                      : None
 */
void mesh_sensor_server_process_cadence_changed(uint8_t element_idx, uint16_t property_id)
{
    wiced_bt_mesh_core_config_sensor_t *p_sensor = NULL;
    uint8_t written_byte = 0;
    wiced_result_t result =  WICED_SUCCESS;
    p_sensor = &mesh_config.elements[element_idx].sensors[0];

    WICED_BT_TRACE("Cadence changed property id:%04x\n", property_id);
    WICED_BT_TRACE("Fast cadence period divisor:%d\n", p_sensor->cadence.fast_cadence_period_divisor);
    WICED_BT_TRACE("Cadence trigger type percent:%d\n", p_sensor->cadence.trigger_type_percentage);
    WICED_BT_TRACE("Trigger delta up:%d\n", p_sensor->cadence.trigger_delta_up);
    WICED_BT_TRACE("Trigger delta down:%d\n", p_sensor->cadence.trigger_delta_down);
    WICED_BT_TRACE("Cadence minimum Interval:%d\n", p_sensor->cadence.min_interval);
    WICED_BT_TRACE("Fast cadence low:%d\n", p_sensor->cadence.fast_cadence_low);
    WICED_BT_TRACE("Fast cadence high:%d\n", p_sensor->cadence.fast_cadence_high);

    if(MESH_ALS_SENSOR_ELEMENT_INDEX == element_idx)
    {
        /* Save ambient light sensor cadence setting to NVRAM */
        written_byte = wiced_hal_write_nvram(MESH_SENSOR_ALS_CADENCE_NVRAM_ID, sizeof(wiced_bt_mesh_sensor_config_cadence_t), (uint8_t*)(&p_sensor->cadence), &result);
        WICED_BT_TRACE("Cadence settings for ALS saved to NVRAM, %d bytes \n", written_byte);
    }

    if(MESH_TEMP_SENSOR_ELEMENT_INDEX == element_idx)
    {
        /* Save ambient light sensor cadence setting to NVRAM */
        written_byte = wiced_hal_write_nvram(MESH_SENSOR_TEMP_CADENCE_NVRAM_ID, sizeof(wiced_bt_mesh_sensor_config_cadence_t), (uint8_t*)(&p_sensor->cadence), &result);
        WICED_BT_TRACE("Cadence settings for thermistor saved to NVRAM, %d bytes \n", written_byte);
    }

    mesh_sensor_server_restart_timer(element_idx,p_sensor);
}


/**
 * Function         mesh_sensor_publish_als_timer_callback
 *
 *                  Publication timer callback for ALS sensor.  Need to send data if publish period
 *                  expired, or if value has changed more than specified in the triggers, or if value
 *                  is in range of fast cadence values.
 *
 * @param[in] arg               : Callback timer parameter
 * @return                      : None
 */
void mesh_sensor_publish_als_timer_callback(TIMER_PARAM_TYPE arg)
{
    wiced_bt_mesh_event_t *p_event;
    wiced_bt_mesh_core_config_sensor_t *p_sensor = (wiced_bt_mesh_core_config_sensor_t *)arg;
    wiced_bool_t pub_needed = WICED_FALSE;
    uint32_t cur_time = wiced_bt_mesh_core_get_tick_count();

    /* Read ambient light level */
    mesh_sensor_current_lux_value = sensor_get_light_level();

    if ((cur_time - mesh_sensor_sent_lux_time) < p_sensor->cadence.min_interval)
    {
        WICED_BT_TRACE("Time since last publish of ALS, time:%d ms interval:%d ms\n", (cur_time - mesh_sensor_sent_lux_time), p_sensor->cadence.min_interval);
        wiced_start_timer(&mesh_sensor_cadence_als_timer, (p_sensor->cadence.min_interval - cur_time + mesh_sensor_sent_lux_time));
    }
    else
    {
        // check if publication timer expired
        if ((mesh_sensor_publish_lux_period != 0) && (cur_time - mesh_sensor_sent_lux_time >= mesh_sensor_publish_lux_period))
        {
            WICED_BT_TRACE("Publish needed for ALS \n");
            pub_needed = WICED_TRUE;
        }
        // still need to send if publication timer has not expired, but triggers are configured, and value
        // changed too much
        if (!pub_needed && ((p_sensor->cadence.trigger_delta_up != 0) || (p_sensor->cadence.trigger_delta_down != 0)))
        {
            if (!p_sensor->cadence.trigger_type_percentage)
            {
                WICED_BT_TRACE("Native current ALS value:%d sent:%d delta:%d/%d\n",
                        mesh_sensor_current_lux_value, mesh_sensor_sent_lux_value, p_sensor->cadence.trigger_delta_up, p_sensor->cadence.trigger_delta_down);

                if (((p_sensor->cadence.trigger_delta_up != 0)   && (mesh_sensor_current_lux_value >= (mesh_sensor_sent_lux_value + p_sensor->cadence.trigger_delta_up))) ||
                    ((p_sensor->cadence.trigger_delta_down != 0) && (mesh_sensor_current_lux_value <= (mesh_sensor_sent_lux_value - p_sensor->cadence.trigger_delta_down))))
                {
                    WICED_BT_TRACE("Publish needed native value for ALS\n");
                    pub_needed = WICED_TRUE;
                }
            }
            else
            {
                // need to calculate percentage of the increase or decrease.  The deltas are in 0.01%.
                if ((p_sensor->cadence.trigger_delta_up != 0) && (mesh_sensor_current_lux_value > mesh_sensor_sent_lux_value))
                {
                    WICED_BT_TRACE("Delta up for ALS:%d\n", ((uint32_t)(mesh_sensor_current_lux_value - mesh_sensor_sent_lux_value) * 10000 / mesh_sensor_current_lux_value));
                    if (((uint32_t)(mesh_sensor_current_lux_value - mesh_sensor_sent_lux_value) * 10000 / mesh_sensor_current_lux_value) > p_sensor->cadence.trigger_delta_up)
                    {
                        WICED_BT_TRACE("Publish needed percent delta up for ALS:%d\n", ((mesh_sensor_current_lux_value - mesh_sensor_sent_lux_value) * 10000 / mesh_sensor_current_lux_value));
                        pub_needed = WICED_TRUE;
                    }
                }
                else if ((p_sensor->cadence.trigger_delta_down != 0) && (mesh_sensor_current_lux_value < mesh_sensor_sent_lux_value))
                {
                    WICED_BT_TRACE("Delta down for ALS:%d\n", ((uint32_t)(mesh_sensor_sent_lux_value - mesh_sensor_current_lux_value) * 10000 / mesh_sensor_current_lux_value));
                    if (((uint32_t)(mesh_sensor_sent_lux_value - mesh_sensor_current_lux_value) * 10000 / mesh_sensor_current_lux_value) > p_sensor->cadence.trigger_delta_down)
                    {
                        WICED_BT_TRACE("Publish needed percent delta down for ALS:%d\n", ((mesh_sensor_sent_lux_value - mesh_sensor_current_lux_value) * 10000 / mesh_sensor_current_lux_value));
                        pub_needed = WICED_TRUE;
                    }
                }
            }
        }
        // may still need to send if fast publication is configured
        if (!pub_needed && (mesh_sensor_fast_publish_lux_period != 0))
        {
            // check if fast publish period expired
            if (cur_time - mesh_sensor_sent_lux_time >= mesh_sensor_fast_publish_lux_period)
            {
                // if cadence high is more than cadence low, to publish, the value should be in range
                if (p_sensor->cadence.fast_cadence_high >= p_sensor->cadence.fast_cadence_low)
                {
                    if ((mesh_sensor_current_lux_value >= p_sensor->cadence.fast_cadence_low) &&
                        (mesh_sensor_current_lux_value <= p_sensor->cadence.fast_cadence_high))
                    {
                        WICED_BT_TRACE("Publish needed in range for ALS\n");
                        pub_needed = WICED_TRUE;
                    }
                }
                else if (p_sensor->cadence.fast_cadence_high < p_sensor->cadence.fast_cadence_low)
                {
                    if ((mesh_sensor_current_lux_value > p_sensor->cadence.fast_cadence_low) ||
                        (mesh_sensor_current_lux_value < p_sensor->cadence.fast_cadence_high))
                    {
                        WICED_BT_TRACE("Publish needed out of range for ALS\n");
                        pub_needed = WICED_TRUE;
                    }
                }
            }
        }

        if (pub_needed)
        {
            mesh_sensor_sent_lux_value  = mesh_sensor_current_lux_value;
            mesh_sensor_sent_lux_time   = cur_time;

            WICED_BT_TRACE("Publish value for ALS:%d lux, time:%d ms\n", mesh_sensor_sent_lux_value, mesh_sensor_sent_lux_time);
            wiced_bt_mesh_model_sensor_server_data(MESH_ALS_SENSOR_ELEMENT_INDEX, WICED_BT_MESH_PROPERTY_PRESENT_AMBIENT_LIGHT_LEVEL, NULL);

        }

        mesh_sensor_server_restart_timer(MESH_ALS_SENSOR_ELEMENT_INDEX,p_sensor);
    }
}


/**
 * Function         mesh_sensor_publish_temp_timer_callback
 *
 *                  Publication timer callback for temperature sensor.  Need to send data if publish
 *                  period expired, or if value has changed more than specified in the triggers, or
 *                  if value is in range of fast cadence values.
 *
 * @param[in] arg               : Callback timer parameter
 * @return                      : None
 */
void mesh_sensor_publish_temp_timer_callback(TIMER_PARAM_TYPE arg)
{
    wiced_bt_mesh_event_t *p_event;
    wiced_bt_mesh_core_config_sensor_t *p_sensor = (wiced_bt_mesh_core_config_sensor_t *)arg;
    wiced_bool_t pub_needed = WICED_FALSE;
    uint32_t cur_time = wiced_bt_mesh_core_get_tick_count();

    /* Read the temperature */
    mesh_sensor_current_temp_value = sensor_get_temperature();

    if ((cur_time - mesh_sensor_sent_temp_time) < p_sensor->cadence.min_interval)
    {
        WICED_BT_TRACE("Time since last publish of temperature, time:%d ms interval:%d ms\n", (cur_time - mesh_sensor_sent_temp_time), p_sensor->cadence.min_interval);
        wiced_start_timer(&mesh_sensor_cadence_temp_timer, p_sensor->cadence.min_interval - cur_time + mesh_sensor_sent_temp_time);
    }
    else
    {
        // check if publication timer expired
        if ((mesh_sensor_publish_temp_period != 0) && ((cur_time - mesh_sensor_sent_temp_time) >= mesh_sensor_publish_temp_period))
        {
            WICED_BT_TRACE("Publish needed for temperature sensor\n");
            pub_needed = WICED_TRUE;
        }
        // still need to send if publication timer has not expired, but triggers are configured, and value
        // changed too much
        if (!pub_needed && ((p_sensor->cadence.trigger_delta_up != 0) || (p_sensor->cadence.trigger_delta_down != 0)))
        {
            if (!p_sensor->cadence.trigger_type_percentage)
            {
                WICED_BT_TRACE("Native current value of for temperature:%d sent:%d delta:%d/%d\n",
                        mesh_sensor_current_temp_value, mesh_sensor_sent_temp_value, p_sensor->cadence.trigger_delta_up, p_sensor->cadence.trigger_delta_down);

                if (((p_sensor->cadence.trigger_delta_up != 0)   && (mesh_sensor_current_temp_value >= (mesh_sensor_sent_temp_value + p_sensor->cadence.trigger_delta_up))) ||
                    ((p_sensor->cadence.trigger_delta_down != 0) && (mesh_sensor_current_temp_value <= (mesh_sensor_sent_temp_value - p_sensor->cadence.trigger_delta_down))))
                {
                    WICED_BT_TRACE("Publish needed native value for temperature\n");
                    pub_needed = WICED_TRUE;
                }
            }
            else
            {
                // need to calculate percentage of the increase or decrease.  The deltas are in 0.01%.
                if ((p_sensor->cadence.trigger_delta_up != 0) && (mesh_sensor_current_temp_value > mesh_sensor_sent_temp_value))
                {
                    WICED_BT_TRACE("Delta up for temperature sensor:%d\n", ((uint32_t)(mesh_sensor_current_temp_value - mesh_sensor_sent_temp_value) * 10000 / mesh_sensor_current_temp_value));
                    if (((uint32_t)(mesh_sensor_current_temp_value - mesh_sensor_sent_temp_value) * 10000 / mesh_sensor_current_temp_value) > p_sensor->cadence.trigger_delta_up)
                    {
                        WICED_BT_TRACE("Publish needed percent delta up for temperature:%d\n", ((mesh_sensor_current_temp_value - mesh_sensor_sent_temp_value) * 10000 / mesh_sensor_current_temp_value));
                        pub_needed = WICED_TRUE;
                    }
                }
                else if ((p_sensor->cadence.trigger_delta_down != 0) && (mesh_sensor_current_temp_value < mesh_sensor_sent_temp_value))
                {
                    WICED_BT_TRACE("Delta down for temperature:%d\n", ((uint32_t)(mesh_sensor_sent_temp_value - mesh_sensor_current_temp_value) * 10000 / mesh_sensor_current_temp_value));
                    if (((uint32_t)(mesh_sensor_sent_temp_value - mesh_sensor_current_temp_value) * 10000 / mesh_sensor_current_temp_value) > p_sensor->cadence.trigger_delta_down)
                    {
                        WICED_BT_TRACE("Pub needed percent delta down for temperature:%d\n", ((mesh_sensor_sent_temp_value - mesh_sensor_current_temp_value) * 10000 / mesh_sensor_current_temp_value));
                        pub_needed = WICED_TRUE;
                    }
                }
            }
        }
        // may still need to send if fast publication is configured
        if (!pub_needed && (mesh_sensor_fast_publish_temp_period != 0))
        {
            // check if fast publish period expired
            if (cur_time - mesh_sensor_sent_temp_time >= mesh_sensor_fast_publish_temp_period)
            {
                // if cadence high is more than cadence low, to publish, the value should be in range
                if (p_sensor->cadence.fast_cadence_high >= p_sensor->cadence.fast_cadence_low)
                {
                    if ((mesh_sensor_current_temp_value >= p_sensor->cadence.fast_cadence_low) &&
                        (mesh_sensor_current_temp_value <= p_sensor->cadence.fast_cadence_high))
                    {
                        WICED_BT_TRACE("Publish needed in range for temperature\n");
                        pub_needed = WICED_TRUE;
                    }
                }
                else if (p_sensor->cadence.fast_cadence_high < p_sensor->cadence.fast_cadence_low)
                {
                    if ((mesh_sensor_current_temp_value > p_sensor->cadence.fast_cadence_low) ||
                        (mesh_sensor_current_temp_value < p_sensor->cadence.fast_cadence_high))
                    {
                        WICED_BT_TRACE("Publish needed out of range for temperature\n");
                        pub_needed = WICED_TRUE;
                    }
                }
            }
        }

        if (pub_needed)
        {
            mesh_sensor_sent_temp_value  = mesh_sensor_current_temp_value;
            mesh_sensor_sent_temp_time   = cur_time;

            WICED_BT_TRACE("Publish temperature value:%d, time:%d ms\n", mesh_sensor_sent_temp_value, mesh_sensor_sent_temp_time);
            wiced_bt_mesh_model_sensor_server_data(MESH_TEMP_SENSOR_ELEMENT_INDEX, WICED_BT_MESH_PROPERTY_PRESENT_AMBIENT_TEMPERATURE, NULL);
        }

        mesh_sensor_server_restart_timer(MESH_TEMP_SENSOR_ELEMENT_INDEX,p_sensor);
    }
}


/**
 * Function         mesh_sensor_server_process_setting_changed
 *
 *                 Process setting change.  Library already copied the new value to the mesh_config.
 *                 Add additional processing here if needed.
 *
 * @param[in] element_idx           : Element id value
 * @param[in] property_id           : Property id value
 * @param[in] setting_property_id   : new setting property_id value
 * @return                          : None
 */
void mesh_sensor_server_process_setting_changed(uint8_t element_idx, uint16_t property_id, uint16_t setting_property_id)
{
    WICED_BT_TRACE("Mesh sensor setting changed, property id:%x, setting property id:%x\n", property_id, setting_property_id);
}


/**
 * Function        mesh_sensor_server_status_changed
 *
 *                 Process the server status change.
 *
 * @param[in] element_idx         : Element id value
 * @param[in] p_data              : sensor data value
 * @param[in] length              : length of data
 * @return                        : None
 */
void mesh_sensor_server_status_changed(uint8_t element_idx, uint8_t *p_data, uint32_t length)
{
    uint16_t property_id;
    uint16_t prop_value_len;
    uint32_t cur_time = wiced_bt_mesh_core_get_tick_count();
    wiced_bt_mesh_core_config_sensor_t *p_sensor = NULL;

    STREAM_TO_UINT16(property_id, p_data);
    STREAM_TO_UINT16(prop_value_len, p_data);

    if ((MESH_SENSOR_ALS_PAYLOAD_LENGTH <= length) && (0 == element_idx) && (property_id == MESH_ALS_SENSOR_PROPERTY_ID) &&
                                                (prop_value_len == MESH_ALS_SENSOR_VALUE_LEN))
    {
        mesh_sensor_current_lux_value = p_data[0];
        WICED_BT_TRACE("New ambient light value:%d lux\n", mesh_sensor_current_lux_value);

        p_sensor = &mesh_config.elements[element_idx].sensors[0];

        // Cannot send pubs more often than cadence.min_interval
        if ((cur_time - mesh_sensor_sent_lux_time) < p_sensor->cadence.min_interval)
        {
            WICED_BT_TRACE("Not enough time since last ambient light value published\n");

            // if timer is running, the value will be sent, when needed, otherwise, start the time.
            wiced_start_timer(&mesh_sensor_cadence_als_timer, p_sensor->cadence.min_interval + mesh_sensor_sent_lux_time - cur_time);
        }
        else
        {
            // the timer callback function sends value change notification if it is appropriate
            mesh_sensor_publish_als_timer_callback((TIMER_PARAM_TYPE)p_sensor);
        }
    }
    else if ((MESH_SENSOR_TEMP_PAYLOAD_LENGTH <= length) && (0 == element_idx) && (property_id == MESH_TEMP_SENSOR_PROPERTY_ID) &&
                                                     (prop_value_len == MESH_TEMP_SENSOR_VALUE_LEN))
    {
        mesh_sensor_current_temp_value = p_data[0];
        WICED_BT_TRACE("New temperature value:%d.%d DegC\n", (mesh_sensor_sent_temp_value/2),(mesh_sensor_sent_temp_value%2)*5);
        p_sensor = &mesh_config.elements[element_idx].sensors[0];

        // Cannot send pubs more often than cadence.min_interval
        if ((cur_time - mesh_sensor_sent_temp_time) < p_sensor->cadence.min_interval)
        {
            WICED_BT_TRACE("Not enough time since last temperature published\n");

            // if timer is running, the value will be sent, when needed, otherwise, start the time.
            wiced_start_timer(&mesh_sensor_cadence_temp_timer, p_sensor->cadence.min_interval + mesh_sensor_sent_temp_time - cur_time);
        }
        else
        {
            // the timer callback function sends value change notification if it is appropriate
            mesh_sensor_publish_temp_timer_callback((TIMER_PARAM_TYPE)p_sensor);
        }
    }
    else
    {
        WICED_BT_TRACE("Mesh sensor server invalid params idx:%d prop:%04x len:%d\n", element_idx, property_id, prop_value_len);
    }
}


/**
 * Function         mesh_app_adv_config
 *
 *                  Sets device name for mesh device.
 *
 * @param[in] device_name       : Device name
 * @param[in] appearance        : Appearance type
 * @return    WICED_TRUE        : on success;
 *            WICED_FALSE       : on failure;
 */
wiced_bool_t mesh_app_adv_config(uint8_t *device_name, uint16_t appearance)
{
    wiced_bt_ble_advert_elem_t  adv_elem[3];
    uint8_t                     buf[2];
    uint8_t                     num_elem = 0;
    wiced_bool_t result = WICED_FALSE;

    if(NULL == device_name)
        return WICED_FALSE;

    wiced_bt_cfg_settings.device_name = (uint8_t *)device_name;
    wiced_bt_cfg_settings.gatt_cfg.appearance = (wiced_bt_gatt_appearance_t)appearance;

    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE;
    adv_elem[num_elem].len = (uint16_t)strlen((const char*)wiced_bt_cfg_settings.device_name);
    adv_elem[num_elem].p_data = wiced_bt_cfg_settings.device_name;
    num_elem++;

    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_APPEARANCE;
    adv_elem[num_elem].len = 2;
    buf[0] = (uint8_t)wiced_bt_cfg_settings.gatt_cfg.appearance;
    buf[1] = (uint8_t)(wiced_bt_cfg_settings.gatt_cfg.appearance >> 8);
    adv_elem[num_elem].p_data = buf;
    num_elem++;

    result = wiced_bt_mesh_set_raw_scan_response_data(num_elem, adv_elem);
    if(WICED_TRUE == result)
    {
        WICED_BT_TRACE("Advertising in the name \"%s\"\n",wiced_bt_cfg_settings.device_name);
    }
    else
    {
        WICED_BT_TRACE("Failed to set scan response data \n");
    }

    return result;
}


/**
 * Function         mesh_app_notify_period_set
 *
 *                  New publication period is set. If it is for the sensor model, this application
 *                  should take care of it.The period may need to be adjusted based on the divisor.
 *
 * @param[in] element_idx       : Element id value
 * @param[in] company_id        : Company id value
 * @param[in] model_id          : Model id value
 * @param[in] period            : Publish period interval value
 * @return    WICED_TRUE        : on success;
 *            WICED_FALSE       : if an parameter mismatch occurred
 */
wiced_bool_t mesh_app_notify_period_set(uint8_t element_idx, uint16_t company_id, uint16_t model_id, uint32_t period)
{

    if (((element_idx != MESH_ALS_SENSOR_ELEMENT_INDEX ) && (element_idx != MESH_TEMP_SENSOR_ELEMENT_INDEX)) ||
            (company_id != MESH_COMPANY_ID_BT_SIG) || (model_id != WICED_BT_MESH_CORE_MODEL_ID_SENSOR_SRV))

    {
        return WICED_FALSE;
    }

    if(MESH_ALS_SENSOR_ELEMENT_INDEX == element_idx)
    {
        WICED_BT_TRACE("Ambient light sensor data send period:%d ms\n", period);
        mesh_sensor_publish_lux_period = period;
    }

    if(MESH_TEMP_SENSOR_ELEMENT_INDEX == element_idx)
    {
        WICED_BT_TRACE("Temperature sensor data send period:%d ms\n", period);
        mesh_sensor_publish_temp_period = period;
    }

    mesh_sensor_server_restart_timer(element_idx,&mesh_config.elements[element_idx].sensors[0]);

    return WICED_TRUE;
}


/**
 * Function        mesh_app_factory_reset
 *
 *                 Application is notified that factory reset is executed.
 *
 * @param[in]                       : None
 * @return                        : None
 */
void mesh_app_factory_reset(void)
{
    wiced_hal_delete_nvram(MESH_SENSOR_ALS_CADENCE_NVRAM_ID, NULL);
    wiced_hal_delete_nvram(MESH_SENSOR_TEMP_CADENCE_NVRAM_ID, NULL);
}

/*END of FILE */
