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
**   This file contains the source code for the IMU_APP App. It reads the
**   simulated IMU (sim_imu library) once per second, prints the x, y and z
**   values and publishes them on the software bus, where DS stores them.
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

#include "sim_imu.h"

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

    CFE_EVS_SendEvent(IMU_APP_COMMANDRST_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "IMU_APP: RESET command");

} /* End of IMU_APP_ResetCounters() */

/*
** IMU_APP_ReadSensor() -- read the IMU once, print it and send it on the bus
**
** Called on every wakeup message. The data packet goes on the software
** bus, where DS stores it in a file and TO_LAB can send it to the ground.
*/
void IMU_APP_ReadSensor(void)
{
    float X;
    float Y;
    float Z;

    if (SIM_IMU_Read(&X, &Y, &Z) != SIM_IMU_SUCCESS)
    {
        CFE_EVS_SendEvent(IMU_APP_READ_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IMU_APP: IMU read failed");
        return;
    }

    IMU_APP_HkTelemetryPkt.sample_count++;
    IMU_APP_DataPkt.sample_count = IMU_APP_HkTelemetryPkt.sample_count;
    IMU_APP_DataPkt.x = X;
    IMU_APP_DataPkt.y = Y;
    IMU_APP_DataPkt.z = Z;

    OS_printf("IMU_APP: #%lu  x %+.3f  y %+.3f  z %+.3f\n",
              (unsigned long)IMU_APP_DataPkt.sample_count, X, Y, Z);

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
