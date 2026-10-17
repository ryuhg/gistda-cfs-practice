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
** File: imu_msg.h
**
** Purpose:
**  Define IMU App Messages and info
**
*******************************************************************************/
#ifndef _imu_msg_h_
#define _imu_msg_h_

/*
** IMU App command codes
*/
#define IMU_NOOP_CC                 0
#define IMU_RESET_COUNTERS_CC       1

/*************************************************************************/
/*
** Type definition (generic "no arguments" command)
*/
typedef struct
{
   uint8    CmdHeader[CFE_SB_CMD_HDR_SIZE];

} IMU_NoArgsCmd_t;

/*************************************************************************/
/*
** Type definition (IMU App housekeeping)
*/
typedef struct
{
    uint8              TlmHeader[CFE_SB_TLM_HDR_SIZE];
    uint8              command_error_count;
    uint8              command_count;
    uint8              spare[2];

}   OS_PACK imu_hk_tlm_t  ;

#define IMU_HK_TLM_LNGTH   sizeof ( imu_hk_tlm_t )

/*************************************************************************/
/*
** Type definition (IMU measurements, one packet per reading)
*/
typedef struct
{
    uint8              TlmHeader[CFE_SB_TLM_HDR_SIZE];
    float              x;                  /* IMU values from SIM_IMU_Read */
    float              y;
    float              z;

}   OS_PACK imu_measurements_tlm_t  ;

#define IMU_MEASUREMENTS_TLM_LNGTH   sizeof ( imu_measurements_tlm_t )

#endif /* _imu_msg_h_ */
