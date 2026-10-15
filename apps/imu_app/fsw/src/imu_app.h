/*******************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.7"
**
**      Copyright (c) 2006-2019 United States Government as represented by
**      the Administrator of the National Aeronautics and Space Administration.
**      All Rights Reserved.
**
**      Licensed under the Apache License, Version 2.0 (the "License");
**      you may not use this file except in compliance with the License.
**      You may obtain a copy of the License at
**
**        http://www.apache.org/licenses/LICENSE-2.0
**
**      Unless required by applicable law or agreed to in writing, software
**      distributed under the License is distributed on an "AS IS" BASIS,
**      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**      See the License for the specific language governing permissions and
**      limitations under the License.
**
** File: imu_app.h
**
** Purpose:
**   This file is main hdr file for the IMU_APP application.
**
*******************************************************************************/
#ifndef _imu_app_h_
#define _imu_app_h_

/*
** Required header files.
*/
#include "cfe.h"

/***********************************************************************/

#define IMU_APP_PIPE_DEPTH                     32

/*
** IMU on the (simulated) I2C bus: MPU-6050 register map
*/
#define IMU_APP_I2C_ADDR                     0x68
#define IMU_APP_REG_DATA                     0x3B   /* ACCEL_XOUT_H, first of 14 data bytes */
#define IMU_APP_DATA_LEN                     14
#define IMU_APP_REG_PWR_MGMT_1               0x6B
#define IMU_APP_REG_WHO_AM_I                 0x75
#define IMU_APP_WHO_AM_I_VALUE               0x68

/*
** Scale factors (default full-scale ranges)
*/
#define IMU_APP_ACCEL_COUNTS_PER_G           16384.0f
#define IMU_APP_GYRO_COUNTS_PER_DPS          131.0f
#define IMU_APP_TEMP_COUNTS_PER_C            340.0f
#define IMU_APP_TEMP_OFFSET_C                36.53f

/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (IMU_APP_Main), these
**       functions are not called from any other source module.
*/
void IMU_APP_Main(void);
void IMU_APP_Init(void);
void IMU_APP_ProcessCommandPacket(void);
void IMU_APP_ProcessGroundCommand(void);
void IMU_APP_ReportHousekeeping(void);
void IMU_APP_ResetCounters(void);
void IMU_APP_InitDevice(void);
void IMU_APP_ReadSensor(void);

bool IMU_APP_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength);

#endif /* _imu_app_h_ */
