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
** File: imu.c
**
** Purpose:
**   This file contains the source code for the IMU App. It reads the
**   simulated IMU (sim_imu library) once per second, prints the x, y and z
**   values and publishes them on the software bus, where DS stores them.
**
*******************************************************************************/

/*
**   Include Files:
*/

#include "imu.h"
#include "imu_perfids.h"
#include "imu_msgids.h"
#include "imu_msg.h"
#include "imu_events.h"
#include "imu_version.h"

#include "sim_imu.h"

/*
** global data
*/

imu_hk_tlm_t  IMU_HkTelemetryPkt;
imu_measurements_tlm_t  IMU_MeasurementsPkt;
CFE_SB_PipeId_t  IMU_CommandPipe;
CFE_SB_MsgPtr_t  IMU_MsgPtr;

static CFE_EVS_BinFilter_t  IMU_EventFilters[] =
       {  /* Event ID    mask */
          {IMU_STARTUP_INF_EID,       0x0000},
          {IMU_COMMAND_ERR_EID,       0x0000},
          {IMU_COMMANDNOP_INF_EID,    0x0000},
          {IMU_COMMANDRST_INF_EID,    0x0000},
          {IMU_INVALID_MSGID_ERR_EID, 0x0000},
          {IMU_LEN_ERR_EID,           0x0000},
       };

/*
** IMU_Main() -- Application entry point and main process loop
*/
void IMU_Main( void )
{
    int32  status;
    uint32 RunStatus = CFE_ES_RunStatus_APP_RUN;

    CFE_ES_PerfLogEntry(IMU_PERF_ID);

    IMU_Init();

    /*
    ** IMU Runloop
    */
    while (CFE_ES_RunLoop(&RunStatus) == true)
    {
        CFE_ES_PerfLogExit(IMU_PERF_ID);

        /* Pend on receipt of command packet -- timeout set to 500 millisecs */
        status = CFE_SB_RcvMsg(&IMU_MsgPtr, IMU_CommandPipe, 500);

        CFE_ES_PerfLogEntry(IMU_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            IMU_ProcessCommandPacket();
        }
    }

    CFE_ES_ExitApp(RunStatus);

} /* End of IMU_Main() */

/*
** IMU_Init() -- initialization
*/
void IMU_Init(void)
{
    /*
    ** Register the app with Executive services
    */
    CFE_ES_RegisterApp();

    /*
    ** Register the events
    */
    CFE_EVS_Register(IMU_EventFilters,
                     sizeof(IMU_EventFilters)/sizeof(CFE_EVS_BinFilter_t),
                     CFE_EVS_EventFilter_BINARY);

    /*
    ** Create the Software Bus command pipe and subscribe to the command,
    ** housekeeping request and wakeup messages
    */
    CFE_SB_CreatePipe(&IMU_CommandPipe, IMU_PIPE_DEPTH, "IMU_PIPE");
    CFE_SB_Subscribe(IMU_CMD_MID, IMU_CommandPipe);
    CFE_SB_Subscribe(IMU_SEND_HK_MID, IMU_CommandPipe);
    CFE_SB_Subscribe(IMU_WAKEUP_MID, IMU_CommandPipe);

    /*
    ** Initialize the housekeeping packet (this also clears the counters)
    */
    CFE_SB_InitMsg(&IMU_HkTelemetryPkt,
                   IMU_HK_TLM_MID,
                   IMU_HK_TLM_LNGTH, true);

    CFE_SB_InitMsg(&IMU_MeasurementsPkt,
                   IMU_MEASUREMENTS_TLM_MID,
                   IMU_MEASUREMENTS_TLM_LNGTH, true);

    CFE_EVS_SendEvent(IMU_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "IMU App Initialized. Version %d.%d.%d.%d",
                      IMU_MAJOR_VERSION,
                      IMU_MINOR_VERSION,
                      IMU_REVISION,
                      IMU_MISSION_REV);

} /* End of IMU_Init() */

/*
** IMU_ProcessCommandPacket() -- process a packet received on the command pipe
*/
void IMU_ProcessCommandPacket(void)
{
    CFE_SB_MsgId_t  MsgId;

    MsgId = CFE_SB_GetMsgId(IMU_MsgPtr);

    switch (MsgId)
    {
        case IMU_CMD_MID:
            IMU_ProcessGroundCommand();
            break;

        case IMU_SEND_HK_MID:
            IMU_ReportHousekeeping();
            break;

        case IMU_WAKEUP_MID:
            IMU_ReadSensor();
            break;

        default:
            IMU_HkTelemetryPkt.command_error_count++;
            CFE_EVS_SendEvent(IMU_INVALID_MSGID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "IMU: invalid command packet, MID = 0x%x", MsgId);
            break;
    }

} /* End of IMU_ProcessCommandPacket() */

/*
** IMU_ProcessGroundCommand() -- IMU ground commands
*/
void IMU_ProcessGroundCommand(void)
{
    uint16 CommandCode;

    CommandCode = CFE_SB_GetCmdCode(IMU_MsgPtr);

    /* Process "known" IMU app ground commands */
    switch (CommandCode)
    {
        case IMU_NOOP_CC:
            if (IMU_VerifyCmdLength(IMU_MsgPtr, sizeof(IMU_NoArgsCmd_t)))
            {
                IMU_HkTelemetryPkt.command_count++;
                CFE_EVS_SendEvent(IMU_COMMANDNOP_INF_EID,
                                  CFE_EVS_EventType_INFORMATION,
                                  "IMU: NOOP command");
            }
            break;

        case IMU_RESET_COUNTERS_CC:
            if (IMU_VerifyCmdLength(IMU_MsgPtr, sizeof(IMU_NoArgsCmd_t)))
            {
                IMU_ResetCounters();
            }
            break;

        default:
            IMU_HkTelemetryPkt.command_error_count++;
            CFE_EVS_SendEvent(IMU_COMMAND_ERR_EID, CFE_EVS_EventType_ERROR,
                              "IMU: invalid command code, CC = %d", CommandCode);
            break;
    }

} /* End of IMU_ProcessGroundCommand() */

/*
** IMU_ReportHousekeeping() -- send the housekeeping telemetry packet
**
** This function is triggered in response to a housekeeping request
** (IMU_SEND_HK_MID). It time stamps the housekeeping packet and sends it
** on the software bus.
*/
void IMU_ReportHousekeeping(void)
{
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &IMU_HkTelemetryPkt);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &IMU_HkTelemetryPkt);

} /* End of IMU_ReportHousekeeping() */

/*
** IMU_ResetCounters() -- reset the housekeeping counters
*/
void IMU_ResetCounters(void)
{
    IMU_HkTelemetryPkt.command_count       = 0;
    IMU_HkTelemetryPkt.command_error_count = 0;

    CFE_EVS_SendEvent(IMU_COMMANDRST_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "IMU: RESET command");

} /* End of IMU_ResetCounters() */

/*
** IMU_ReadSensor() -- read the IMU once, print it and send it on the bus
**
** Called on every wakeup message. The data packet goes on the software
** bus, where DS stores it in a file.
*/
void IMU_ReadSensor(void)
{
    float X;
    float Y;
    float Z;

    if (SIM_IMU_Read(&X, &Y, &Z) != SIM_IMU_SUCCESS)
    {
        CFE_EVS_SendEvent(IMU_READ_ERR_EID, CFE_EVS_EventType_ERROR,
                          "IMU: IMU read failed");
        return;
    }

    IMU_MeasurementsPkt.x = X;
    IMU_MeasurementsPkt.y = Y;
    IMU_MeasurementsPkt.z = Z;

    OS_printf("IMU: x %+.3f  y %+.3f  z %+.3f\n", X, Y, Z);

    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &IMU_MeasurementsPkt);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &IMU_MeasurementsPkt);

} /* End of IMU_ReadSensor() */

/*
** IMU_VerifyCmdLength() -- verify command packet length
*/
bool IMU_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength)
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

        CFE_EVS_SendEvent(IMU_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid msg length: ID = 0x%X,  CC = %d, Len = %d, Expected = %d",
                          MessageID, CommandCode, ActualLength, ExpectedLength);
        result = false;
        IMU_HkTelemetryPkt.command_error_count++;
    }

    return result;

} /* End of IMU_VerifyCmdLength() */
