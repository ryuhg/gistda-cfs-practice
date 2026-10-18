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
** File: imu_msgids.h
**
** Purpose:
**  Define IMU App Message IDs
**
*************************************************************************/
#ifndef _imu_msgids_h_
#define _imu_msgids_h_

#define IMU_CMD_MID              0x18C0   /* ground commands */
#define IMU_SEND_HK_MID          0x18C1   /* housekeeping request (from SCH_LAB) */
#define IMU_WAKEUP_MID           0x18C2   /* read the sensor once (from SCH_LAB, 1 Hz) */
#define IMU_HK_TLM_MID           0x08C0   /* housekeeping telemetry */
#define IMU_MEASUREMENTS_TLM_MID 0x08C1   /* IMU measurements, stored by DS */

#endif /* _imu_msgids_h_ */
