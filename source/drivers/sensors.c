/******************************************************************************
* File Name:   sensors.c
*
* Description: This file shows the implementation of sensor drivers.
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
#include "wiced_hal_adc.h"
#include "wiced_thermistor.h"
#include "max_44009.h"
#include "GeneratedSource/cycfg_pins.h"


/******************************************************************************
 *                              Macros
 ******************************************************************************/

#define SENSOR_TEMP_MIN_RANGE                    (-6400)
#define SENSOR_TEMP_MAX_RANGE                    (6350)
#define SENSOR_TEMP_MIN_VALUE                    (0x80)
#define SENSOR_TEMP_MAX_VALUE                    (0x7F)

/******************************************************************************
 *                              Structures
 ******************************************************************************/

/******************************************************************************
 *                          Function Prototypes
 ******************************************************************************/

/******************************************************************************
 *                          Variables Definitions
 ******************************************************************************/

max44009_user_set_t max44009_cfg;    // configuration structure for ambient light sensor
thermistor_cfg_t  thermistor_cfg;    // configuration structure for thermistor

/******************************************************************************
*                                Function Definitions
******************************************************************************/

/**
 * Function        sensor_init_thermistor
 *
 *                 Function initialize the thermistor.
 *
 * @return                        : None.
 */
void sensor_init_thermistor(void)
{
    // Initialize thermistor
    thermistor_cfg.high_pin = ADC_INPUT_P8;
    thermistor_init();
    WICED_BT_TRACE("Thermistor initialization done!\n");
}

/**
 * Function        sensor_init_als
 *
 *                 Function initialize the ALS sensor.
 *
 * @return                        : None.
 */
void sensor_init_als(void)
{
    // Initialize ambient light sensor
    max44009_cfg.scl_pin = I2C_SCL;
    max44009_cfg.sda_pin = I2C_SDA;
    max44009_cfg.irq_pin = WICED_HAL_GPIO_PIN_UNUSED;

    max44009_init(&max44009_cfg, NULL, NULL);
    WICED_BT_TRACE("ALS sensor initialization done!\n");

}


/**
 * Function        sensor_get_temperature
 *
 *                 Helper function to read temperature from the thermistor and convert temperature in celsius
 *                 to Temperature 8 format.  Unit is degree Celsius with a resolution of 0.5. Minimum: -64.0 Maximum: 63.5.
 *
 * @return                        : Temperature in celsius.
 */
int8_t sensor_get_temperature(void)
{
    int16_t temp_celsius_100 = thermistor_read(&thermistor_cfg);

    if (temp_celsius_100 < SENSOR_TEMP_MIN_RANGE)
    {
        return SENSOR_TEMP_MIN_VALUE;
    }
    else if (temp_celsius_100 >= SENSOR_TEMP_MAX_RANGE)
    {
        return SENSOR_TEMP_MAX_VALUE;
    }
    else
    {
        return (int8_t)((temp_celsius_100 / 50 )); /* divided by 50 to avoid floating values */
    }
}


/**
 * Function        sensor_get_ambient_lux
 *
 *                 Function to read light level from ALS sensor
 *
 * @return                        : Ambient light levels in lux.
 */
uint32_t sensor_get_light_level(void)
{
    return max44009_read_ambient_light();
}


/*END of FILE */
