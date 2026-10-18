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
** File: imu_events.h
**
** Purpose:
**  Define IMU App Event IDs
**
*************************************************************************/
#ifndef _imu_events_h_
#define _imu_events_h_

#define IMU_RESERVED_EID              0
#define IMU_STARTUP_INF_EID           1
#define IMU_COMMAND_ERR_EID           2
#define IMU_COMMANDNOP_INF_EID        3
#define IMU_COMMANDRST_INF_EID        4
#define IMU_INVALID_MSGID_ERR_EID     5
#define IMU_LEN_ERR_EID               6
#define IMU_READ_ERR_EID              7

#endif /* _imu_events_h_ */
