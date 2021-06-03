/******************************************************************************
* File Name:   main.c
*
* Description: This file has main application initialization and LED/PWM
*              functions.
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

#include "wiced_platform.h"
#include "wiced_hal_gpio.h"
#include "wiced_hal_pwm.h"
#include "wiced_hal_aclk.h"
#include "wiced_bt_trace.h"
#include "wiced_bt_mesh_core.h"
#include "wiced_bt_mesh_models.h"
#include "mesh_server.h"
#include "sensors.h"
#include "GeneratedSource/cycfg_pins.h"


/******************************************************************************
 *                              Macros
 ******************************************************************************/

#define PWM_CHANNEL                             PWM0
#define PWM_INP_CLK_IN_HZ                       (32*1000)    /* PWM Input Clock Frequency*/

#define PWM_FREQ_IN_HZ                          (2)
#define PWM_DUTY_CYCLE                          (50)

/******************************************************************************
 *                              Structures
 ******************************************************************************/

/******************************************************************************
 *                          Function Prototypes
 ******************************************************************************/
static void app_set_status_led(uint8_t status, uint8_t frequency, uint8_t dutycycle);

/******************************************************************************
 *                          Variables Definitions
 ******************************************************************************/
wiced_pwm_config_t pwm_config;

/******************************************************************************
*                                Function Definitions
******************************************************************************/
/**
 * Function         mesh_app_init
 *
 *                  This function initialize the application
 *
 * @param[in] is_provisioned       : Provisioned status
 *
 * @return                         : None
 */
void mesh_app_init(wiced_bool_t is_provisioned)
{

    WICED_BT_TRACE("\n***** Mesh SensorHub *****\n");

    if (!is_provisioned)
    {
        // Adv Data is fixed. Spec allows to put URI, Name, Appearance and Tx Power in the Scan Response Data.
        (void) mesh_app_adv_config((uint8_t*)"Mesh SensorHub", APPEARANCE_SENSOR_GENERIC);
    }

    /* Enable PWM Clock*/
    wiced_hal_aclk_enable(PWM_INP_CLK_IN_HZ, WICED_ACLK1, WICED_ACLK_FREQ_24_MHZ);

    app_set_status_led(!is_provisioned, PWM_FREQ_IN_HZ, PWM_DUTY_CYCLE);


    if (!is_provisioned)
        return;

    /* Initialization of sensor */
    sensor_init_als();
    sensor_init_thermistor();

    /* Initialization of timers */
    mesh_sensor_cadence_init_als_timer();
    mesh_sensor_cadence_init_temp_timer();

    /* Initialization of mesh model */
    mesh_sensor_server_init_model(is_provisioned);

    /* Initial reading of sensors */
    mesh_sensor_init_value();

}


/**
 * Function        app_set_status_led
 *
 *                 Helper function to Control LED with PWM for advertisement status.
 * @param[in] status              : LED status value to set (ON/OFF)
 * @param[in] frequency           : PWM frequency value
 * @param[in] dutycycle           : PWM Dutycycle value
 * @return                        : None
 */
void app_set_status_led(uint8_t status, uint8_t frequency, uint8_t dutycycle)
{

    if (status)
    {
        wiced_hal_gpio_select_function(LED1, WICED_PWM0);

        wiced_hal_pwm_get_params(PWM_INP_CLK_IN_HZ,
                                 dutycycle,
                                 frequency,
                                 &pwm_config);

        wiced_hal_pwm_start(PWM_CHANNEL,
                            PMU_CLK,
                            pwm_config.toggle_count,
                            pwm_config.init_count,
                            0);

        wiced_hal_pwm_enable(PWM_CHANNEL);
    }
    else
    {
        wiced_hal_pwm_disable(PWM_CHANNEL);
        wiced_hal_gpio_select_function(LED1, WICED_GPIO);
        wiced_hal_gpio_set_pin_output(LED1, 0u);
    }
}


/*END of FILE */




