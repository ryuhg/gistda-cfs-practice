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
** File: imu_app.c
**
** Purpose:
**   This file contains the source code for the IMU_APP App. It reads a
**   (simulated) MPU-6050 IMU over I2C once per second, prints the values
**   and publishes them on the software bus, where DS stores them.
**
*******************************************************************************/

/*
**   Include Files:
*/

#include "imu_app.h"
#include "imu_app_perfids.h"
#include "imu_app_msgids.h"
#include "imu_app_msg.h"
#include "imu_app_events.h"
#include "imu_app_version.h"

#include "sim_i2c.h"

/*
** global data
*/

imu_app_hk_tlm_t  IMU_APP_HkTelemetryPkt;
imu_app_data_tlm_t  IMU_APP_DataPkt;
CFE_SB_PipeId_t  IMU_APP_CommandPipe;
CFE_SB_MsgPtr_t  IMU_APP_MsgPtr;

static CFE_EVS_BinFilter_t  IMU_APP_EventFilters[] =
       {  /* Event ID    mask */
          {IMU_APP_STARTUP_INF_EID,       0x0000},
          {IMU_APP_COMMAND_ERR_EID,       0x0000},
          {IMU_APP_COMMANDNOP_INF_EID,    0x0000},
          {IMU_APP_COMMANDRST_INF_EID,    0x0000},
          {IMU_APP_INVALID_MSGID_ERR_EID, 0x0000},
          {IMU_APP_LEN_ERR_EID,           0x0000},
          {IMU_APP_READ_ERR_EID,          CFE_EVS_FIRST_8_STOP},  /* at most 8 messages */
       };

/*
** IMU_APP_Main() -- Application entry point and main process loop
*/
void IMU_APP_Main( void )
{
    int32  status;
    uint32 RunStatus = CFE_ES_RunStatus_APP_RUN;

    CFE_ES_PerfLogEntry(IMU_APP_PERF_ID);

    IMU_APP_Init();

    /*
    ** IMU_APP Runloop
    */
    while (CFE_ES_RunLoop(&RunStatus) == true)
    {
        CFE_ES_PerfLogExit(IMU_APP_PERF_ID);

        /* Pend on receipt of command packet -- timeout set to 500 millisecs */
        status = CFE_SB_RcvMsg(&IMU_APP_MsgPtr, IMU_APP_CommandPipe, 500);

        CFE_ES_PerfLogEntry(IMU_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            IMU_APP_ProcessCommandPacket();
        }
    }

    CFE_ES_ExitApp(RunStatus);

} /* End of IMU_APP_Main() */

/*
** IMU_APP_Init() -- initialization
*/
void IMU_APP_Init(void)
{
    /*
    ** Register the app with Executive services
    */
    CFE_ES_RegisterApp();

    /*
    ** Register the events
    */
    CFE_EVS_Register(IMU_APP_EventFilters,
                     sizeof(IMU_APP_EventFilters)/sizeof(CFE_EVS_BinFilter_t),
                     CFE_EVS_EventFilter_BINARY);

    /*
    ** Create the Software Bus command pipe and subscribe to the command,
    ** housekeeping request and wakeup messages
    */
    CFE_SB_CreatePipe(&IMU_APP_CommandPipe, IMU_APP_PIPE_DEPTH, "IMU_APP_PIPE");
    CFE_SB_Subscribe(IMU_APP_CMD_MID, IMU_APP_CommandPipe);
    CFE_SB_Subscribe(IMU_APP_SEND_HK_MID, IMU_APP_CommandPipe);
    CFE_SB_Subscribe(IMU_APP_WAKEUP_MID, IMU_APP_CommandPipe);

    /*
    ** Initialize the housekeeping packet (this also clears the counters)
    */
    CFE_SB_InitMsg(&IMU_APP_HkTelemetryPkt,
                   IMU_APP_HK_TLM_MID,
                   IMU_APP_HK_TLM_LNGTH, true);

    CFE_SB_InitMsg(&IMU_APP_DataPkt,
                   IMU_APP_DATA_TLM_MID,
                   IMU_APP_DATA_TLM_LNGTH, true);

    CFE_EVS_SendEvent(IMU_APP_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "IMU_APP App Initialized. Version %d.%d.%d.%d",
                      IMU_APP_MAJOR_VERSION,
                      IMU_APP_MINOR_VERSION,
                      IMU_APP_REVISION,
                      IMU_APP_MISSION_REV);

    IMU_APP_InitDevice();

} /* End of IMU_APP_Init() */

/*
** IMU_APP_ProcessCommandPacket() -- process a packet received on the command pipe
*/
void IMU_APP_ProcessCommandPacket(void)
{
    CFE_SB_MsgId_t  MsgId;

    MsgId = CFE_SB_GetMsgId(IMU_APP_MsgPtr);

    switch (MsgId)
    {
        case IMU_APP_CMD_MID:
            IMU_APP_ProcessGroundCommand();
            break;

        case IMU_APP_SEND_HK_MID:
            IMU_APP_ReportHousekeeping();
            break;

        case IMU_APP_WAKEUP_MID:
            IMU_APP_ReadSensor();
            break;

        default:
            IMU_APP_HkTelemetryPkt.command_error_count++;
            CFE_EVS_SendEvent(IMU_APP_INVALID_MSGID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "IMU_APP: invalid command packet, MID = 0x%x", MsgId);
            break;
    }

} /* End of IMU_APP_ProcessCommandPacket() */

/*
** IMU_APP_ProcessGroundCommand() -- IMU_APP ground commands
*/
void IMU_APP_ProcessGroundCommand(void)
{
    uint16 CommandCode;

    CommandCode = CFE_SB_GetCmdCode(IMU_APP_MsgPtr);

    /* Process "known" IMU_APP app ground commands */
    switch (CommandCode)
    {
        case IMU_APP_NOOP_CC:
            if (IMU_APP_VerifyCmdLength(IMU_APP_MsgPtr, sizeof(IMU_APP_NoArgsCmd_t)))
            {
                IMU_APP_HkTelemetryPkt.command_count++;
                CFE_EVS_SendEvent(IMU_APP_COMMANDNOP_INF_EID,
                                  CFE_EVS_EventType_INFORMATION,
                                  "IMU_APP: NOOP command");
            }
            break;

        case IMU_APP_RESET_COUNTERS_CC:
            if (IMU_APP_VerifyCmdLength(IMU_APP_MsgPtr, sizeof(IMU_APP_NoArgsCmd_t)))
            {
                IMU_APP_ResetCounters();
            }
            break;

        default:
            IMU_APP_HkTelemetryPkt.command_error_count++;
            CFE_EVS_SendEvent(IMU_APP_COMMAND_ERR_EID, CFE_EVS_EventType_ERROR,
                              "IMU_APP: invalid command code, CC = %d", CommandCode);
            break;
    }

} /* End of IMU_APP_ProcessGroundCommand() */

/*
** IMU_APP_ReportHousekeeping() -- send the housekeeping telemetry packet
**
** This function is triggered in response to a housekeeping request
** (IMU_APP_SEND_HK_MID). It time stamps the housekeeping packet and sends it
** on the software bus.
*/
void IMU_APP_ReportHousekeeping(void)
{
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &IMU_APP_HkTelemetryPkt);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &IMU_APP_HkTelemetryPkt);

} /* End of IMU_APP_ReportHousekeeping() */

/*
** IMU_APP_ResetCounters() -- reset the housekeeping counters
*/
void IMU_APP_ResetCounters(void)
{
    IMU_APP_HkTelemetryPkt.command_count       = 0;
    IMU_APP_HkTelemetryPkt.command_error_count = 0;
    IMU_APP_HkTelemetryPkt.i2c_error_count     = 0;

    CFE_EVS_SendEvent(IMU_APP_COMMANDRST_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "IMU_APP: RESET command");

} /* End of IMU_APP_ResetCounters() */

/*
** IMU_APP_InitDevice() -- find the IMU on the I2C bus and wake it up
**
** The device answers WHO_AM_I with its own address. It powers up asleep,
** so PWR_MGMT_1 is cleared to start the measurements.
*/
void IMU_APP_InitDevice(void)
{
    uint8 WhoAmI = 0;
    uint8 PwrMgmt = 0x00;
    int32 Status;

    IMU_APP_HkTelemetryPkt.device_ready = 0;

    Status = SIM_I2C_Read(IMU_APP_I2C_ADDR, IMU_APP_REG_WHO_AM_I, &WhoAmI, 1);
    if (Status != SIM_I2C_SUCCESS || WhoAmI != IMU_APP_WHO_AM_I_VALUE)
    {
        IMU_APP_HkTelemetryPkt.i2c_error_count++;
        CFE_EVS_SendEvent(IMU_APP_DEVICE_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IMU_APP: no IMU at I2C address 0x%02X (status %d, WHO_AM_I 0x%02X)",
                          IMU_APP_I2C_ADDR, (int)Status, WhoAmI);
        return;
    }

    Status = SIM_I2C_Write(IMU_APP_I2C_ADDR, IMU_APP_REG_PWR_MGMT_1, &PwrMgmt, 1);
    if (Status != SIM_I2C_SUCCESS)
    {
        IMU_APP_HkTelemetryPkt.i2c_error_count++;
        CFE_EVS_SendEvent(IMU_APP_DEVICE_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IMU_APP: could not wake up the IMU (status %d)", (int)Status);
        return;
    }

    IMU_APP_HkTelemetryPkt.device_ready = 1;
    CFE_EVS_SendEvent(IMU_APP_DEVICE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "IMU_APP: IMU found at I2C address 0x%02X and woken up",
                      IMU_APP_I2C_ADDR);

} /* End of IMU_APP_InitDevice() */

/*
** IMU_APP_ReadSensor() -- read the IMU once, print it and send it on the bus
**
** Called on every wakeup message. The data packet goes on the software
** bus, where DS stores it in a file and TO_LAB can send it to the ground.
*/
void IMU_APP_ReadSensor(void)
{
    uint8 Data[IMU_APP_DATA_LEN];
    int32 Status;
    int   i;

    /* Try again to set up the device if that failed before */
    if (IMU_APP_HkTelemetryPkt.device_ready == 0)
    {
        IMU_APP_InitDevice();
        if (IMU_APP_HkTelemetryPkt.device_ready == 0)
        {
            return;
        }
    }

    /* One burst read of all 14 data registers */
    Status = SIM_I2C_Read(IMU_APP_I2C_ADDR, IMU_APP_REG_DATA, Data, IMU_APP_DATA_LEN);
    if (Status != SIM_I2C_SUCCESS)
    {
        IMU_APP_HkTelemetryPkt.i2c_error_count++;
        CFE_EVS_SendEvent(IMU_APP_READ_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IMU_APP: I2C read failed (status %d)", (int)Status);
        return;
    }

    /* Each value is a signed 16-bit number, high byte first */
    for (i = 0; i < 7; i++)
    {
        IMU_APP_DataPkt.raw[i] = (int16)((Data[2 * i] << 8) | Data[2 * i + 1]);
    }

    /* Convert to engineering units */
    for (i = 0; i < 3; i++)
    {
        IMU_APP_DataPkt.accel_g[i]  = IMU_APP_DataPkt.raw[i] / IMU_APP_ACCEL_COUNTS_PER_G;
        IMU_APP_DataPkt.gyro_dps[i] = IMU_APP_DataPkt.raw[4 + i] / IMU_APP_GYRO_COUNTS_PER_DPS;
    }
    IMU_APP_DataPkt.temp_c = IMU_APP_DataPkt.raw[3] / IMU_APP_TEMP_COUNTS_PER_C + IMU_APP_TEMP_OFFSET_C;

    IMU_APP_HkTelemetryPkt.sample_count++;
    IMU_APP_DataPkt.sample_count = IMU_APP_HkTelemetryPkt.sample_count;

    OS_printf("IMU_APP: #%lu accel [%+.3f %+.3f %+.3f] g  gyro [%+.2f %+.2f %+.2f] deg/s  temp %.2f C\n",
              (unsigned long)IMU_APP_DataPkt.sample_count,
              IMU_APP_DataPkt.accel_g[0], IMU_APP_DataPkt.accel_g[1], IMU_APP_DataPkt.accel_g[2],
              IMU_APP_DataPkt.gyro_dps[0], IMU_APP_DataPkt.gyro_dps[1], IMU_APP_DataPkt.gyro_dps[2],
              IMU_APP_DataPkt.temp_c);

    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &IMU_APP_DataPkt);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &IMU_APP_DataPkt);

} /* End of IMU_APP_ReadSensor() */

/*
** IMU_APP_VerifyCmdLength() -- verify command packet length
*/
bool IMU_APP_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength)
{
    bool result = true;

    uint16 ActualLength = CFE_SB_GetTotalMsgLength(msg);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_SB_MsgId_t MessageID   = CFE_SB_GetMsgId(msg);
        uint16         CommandCode = CFE_SB_GetCmdCode(msg);

        CFE_EVS_SendEvent(IMU_APP_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid msg length: ID = 0x%X,  CC = %d, Len = %d, Expected = %d",
                          MessageID, CommandCode, ActualLength, ExpectedLength);
        result = false;
        IMU_APP_HkTelemetryPkt.command_error_count++;
    }

    return result;

} /* End of IMU_APP_VerifyCmdLength() */
