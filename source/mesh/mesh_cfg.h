/******************************************************************************
* File Name:   mesh_cfg.h
*
* Description: This file has the macros for sensor server model.
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

#ifndef MESH_CFG_H_
#define MESH_CFG_H_

#include "stdint.h"

/******************************************************************************
 *                             Macros
 ******************************************************************************/
#define MESH_PID                                0x3122
#define MESH_VID                                0x0002
#define MESH_CACHE_REPLAY_SIZE                  0x0008
#define MESH_COMPANY_ID                         0x0009

#define MESH_ALS_SENSOR_PROPERTY_ID             WICED_BT_MESH_PROPERTY_PRESENT_AMBIENT_LIGHT_LEVEL
#define MESH_ALS_SENSOR_VALUE_LEN               WICED_BT_MESH_PROPERTY_LEN_PRESENT_AMBIENT_LIGHT_LEVEL

#define MESH_TEMP_SENSOR_PROPERTY_ID            WICED_BT_MESH_PROPERTY_PRESENT_AMBIENT_TEMPERATURE
#define MESH_TEMP_SENSOR_VALUE_LEN              WICED_BT_MESH_PROPERTY_LEN_PRESENT_AMBIENT_TEMPERATURE

// The ALS sensor has a positive and negative tolerance of 1%
#define MESH_ALS_SENSOR_POSITIVE_TOLERANCE      CONVERT_TOLERANCE_PERCENTAGE_TO_MESH(1)
#define MESH_ALS_SENSOR_NEGATIVE_TOLERANCE      CONVERT_TOLERANCE_PERCENTAGE_TO_MESH(1)

// The thermistor has a positive and negative tolerance of 1%
#define MESH_TEMP_SENSOR_POSITIVE_TOLERANCE     CONVERT_TOLERANCE_PERCENTAGE_TO_MESH(1)
#define MESH_TEMP_SENSOR_NEGATIVE_TOLERANCE     CONVERT_TOLERANCE_PERCENTAGE_TO_MESH(1)

#define MESH_ALS_SENSOR_SAMPLING_FUNCTION       WICED_BT_MESH_SENSOR_SAMPLING_FUNCTION_UNKNOWN
#define MESH_ALS_SENSOR_MEASUREMENT_PERIOD      WICED_BT_MESH_SENSOR_VAL_UNKNOWN
#define MESH_ALS_SENSOR_UPDATE_INTERVAL         WICED_BT_MESH_SENSOR_VAL_UNKNOWN

#define MESH_TEMP_SENSOR_SAMPLING_FUNCTION      WICED_BT_MESH_SENSOR_SAMPLING_FUNCTION_UNKNOWN
#define MESH_TEMP_SENSOR_MEASUREMENT_PERIOD     WICED_BT_MESH_SENSOR_VAL_UNKNOWN
#define MESH_TEMP_SENSOR_UPDATE_INTERVAL        WICED_BT_MESH_SENSOR_VAL_UNKNOWN

#define MESH_ALS_SENSOR_ELEMENT_INDEX           (0)
#define MESH_TEMP_SENSOR_ELEMENT_INDEX          (1)

#endif /* MESH_CFG_H_ */
