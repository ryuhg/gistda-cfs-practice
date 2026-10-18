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
** File: imu.h
**
** Purpose:
**   This file is main hdr file for the IMU application.
**
*******************************************************************************/
#ifndef _imu_h_
#define _imu_h_

/*
** Required header files.
*/
#include "cfe.h"

/***********************************************************************/

#define IMU_PIPE_DEPTH                     32

/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (IMU_Main), these
**       functions are not called from any other source module.
*/
void IMU_Main(void);
void IMU_Init(void);
void IMU_ProcessCommandPacket(void);
void IMU_ProcessGroundCommand(void);
void IMU_ReportHousekeeping(void);
void IMU_ResetCounters(void);
void IMU_ReadSensor(void);

bool IMU_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength);

#endif /* _imu_h_ */
