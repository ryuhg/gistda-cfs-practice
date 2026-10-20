#!/usr/bin/env python3
"""Create a skeleton cFS application under apps/.

Usage:
    python3 create_app.py <app_name>

The new app follows the layout of apps/sample_app (sample_app 1.1.0, cFS 6.7)
and keeps only the essentials: a command pipe, the NOOP and RESET_COUNTERS
commands, and a housekeeping telemetry packet. Tables are not used.

The name may be given in any case: "hello", "HELLO", "HelloWorld" and
"hello_world" are all accepted. Lower case is used for directory, file and
build target names, and upper case for C identifiers, e.g. apps/hello_world/,
hello_world.c, HELLO_WORLD_Main(), HELLO_WORLD_CMD_MID.

Message IDs and the performance ID are picked so they do not collide with
any app already under apps/.
"""

import argparse
import os
import re
import sys

TOP_DIR = os.path.dirname(os.path.abspath(__file__))
APPS_DIR = os.path.join(TOP_DIR, "apps")

# Headers searched for message IDs and performance IDs that are already taken
ID_SCAN_DIRS = [
    APPS_DIR,
    os.path.join(TOP_DIR, "sample_defs"),
    os.path.join(TOP_DIR, "cfe", "cmake", "sample_defs"),
]

# OS_MAX_API_NAME in sample_defs/default_osconfig.h. cFE stores app names,
# entry point names and pipe names in buffers of this size (including the
# terminating NUL), so each of them must be shorter than this.
OS_MAX_API_NAME = 20

# Application IDs (low byte of the message ID) to try for new apps.
# The lab apps use 0x80-0x85, cFE itself uses 0x00-0x1F and 0x60-0x62.
FIRST_APID = 0x90
LAST_APID = 0xFE

# Performance IDs to try for new apps (CFE_MISSION_ES_PERF_MAX_IDS is 128)
FIRST_PERF_ID = 100
LAST_PERF_ID = 127

# Prefixes that belong to the framework itself
RESERVED_PREFIXES = ("cfe", "os", "osal", "psp", "ccsds", "ut")

NASA_HEADER = """\
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
"""

# Templates. Placeholders: @app@ (lower case name), @APP@ (upper case name),
# @CMD_MID@, @SEND_HK_MID@, @HK_TLM_MID@, @PERF_ID@.

CMAKELISTS = """\
cmake_minimum_required(VERSION 2.6.4)
project(CFE_@APP@ C)

include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)

aux_source_directory(fsw/src APP_SRC_FILES)

# Create the app module
add_cfe_app(@app@ ${APP_SRC_FILES})
"""

PERFIDS_H = NASA_HEADER + """\
** File: @app@_perfids.h
**
** Purpose:
**  Define @APP@ App Performance IDs
**
*************************************************************************/
#ifndef _@app@_perfids_h_
#define _@app@_perfids_h_

#define @APP@_PERF_ID              @PERF_ID@

#endif /* _@app@_perfids_h_ */
"""

MSGIDS_H = NASA_HEADER + """\
** File: @app@_msgids.h
**
** Purpose:
**  Define @APP@ App Message IDs
**
*************************************************************************/
#ifndef _@app@_msgids_h_
#define _@app@_msgids_h_

#define @APP@_CMD_MID              @CMD_MID@
#define @APP@_SEND_HK_MID          @SEND_HK_MID@
#define @APP@_HK_TLM_MID           @HK_TLM_MID@

#endif /* _@app@_msgids_h_ */
"""

VERSION_H = NASA_HEADER + """\
** File: @app@_version.h
**
** Purpose:
**  The @APP@ App header file containing version number
**
*************************************************************************/
#ifndef _@app@_version_h_
#define _@app@_version_h_

#define @APP@_MAJOR_VERSION    1
#define @APP@_MINOR_VERSION    0
#define @APP@_REVISION         0
#define @APP@_MISSION_REV      0

#endif /* _@app@_version_h_ */
"""

EVENTS_H = NASA_HEADER + """\
** File: @app@_events.h
**
** Purpose:
**  Define @APP@ App Event IDs
**
*************************************************************************/
#ifndef _@app@_events_h_
#define _@app@_events_h_

#define @APP@_RESERVED_EID              0
#define @APP@_STARTUP_INF_EID           1
#define @APP@_COMMAND_ERR_EID           2
#define @APP@_COMMANDNOP_INF_EID        3
#define @APP@_COMMANDRST_INF_EID        4
#define @APP@_INVALID_MSGID_ERR_EID     5
#define @APP@_LEN_ERR_EID               6

#endif /* _@app@_events_h_ */
"""

MSG_H = NASA_HEADER + """\
** File: @app@_msg.h
**
** Purpose:
**  Define @APP@ App Messages and info
**
*******************************************************************************/
#ifndef _@app@_msg_h_
#define _@app@_msg_h_

/*
** @APP@ App command codes
*/
#define @APP@_NOOP_CC                 0
#define @APP@_RESET_COUNTERS_CC       1

/*************************************************************************/
/*
** Type definition (generic "no arguments" command)
*/
typedef struct
{
   uint8    CmdHeader[CFE_SB_CMD_HDR_SIZE];

} @APP@_NoArgsCmd_t;

/*************************************************************************/
/*
** Type definition (@APP@ App housekeeping)
**
** Not packed (no OS_PACK), like the cFE core packets: order the fields so
** that the compiler needs no padding between them.
*/
typedef struct
{
    uint8              TlmHeader[CFE_SB_TLM_HDR_SIZE];
    uint8              command_error_count;
    uint8              command_count;
    uint8              spare[2];

}   @app@_hk_tlm_t  ;

#define @APP@_HK_TLM_LNGTH   sizeof ( @app@_hk_tlm_t )

#endif /* _@app@_msg_h_ */
"""

APP_H = NASA_HEADER + """\
** File: @app@.h
**
** Purpose:
**   This file is main hdr file for the @APP@ application.
**
*******************************************************************************/
#ifndef _@app@_h_
#define _@app@_h_

/*
** Required header files.
*/
#include "cfe.h"

/***********************************************************************/

#define @APP@_PIPE_DEPTH                     32

/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (@APP@_Main), these
**       functions are not called from any other source module.
*/
void @APP@_Main(void);
void @APP@_Init(void);
void @APP@_ProcessCommandPacket(void);
void @APP@_ProcessGroundCommand(void);
void @APP@_ReportHousekeeping(void);
void @APP@_ResetCounters(void);

bool @APP@_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength);

#endif /* _@app@_h_ */
"""

APP_C = NASA_HEADER + """\
** File: @app@.c
**
** Purpose:
**   This file contains the source code for the @APP@ App.
**
*******************************************************************************/

/*
**   Include Files:
*/

#include "@app@.h"
#include "@app@_perfids.h"
#include "@app@_msgids.h"
#include "@app@_msg.h"
#include "@app@_events.h"
#include "@app@_version.h"

/*
** global data
*/

@app@_hk_tlm_t  @APP@_HkTelemetryPkt;
CFE_SB_PipeId_t  @APP@_CommandPipe;
CFE_SB_MsgPtr_t  @APP@_MsgPtr;

static CFE_EVS_BinFilter_t  @APP@_EventFilters[] =
       {  /* Event ID    mask */
          {@APP@_STARTUP_INF_EID,       0x0000},
          {@APP@_COMMAND_ERR_EID,       0x0000},
          {@APP@_COMMANDNOP_INF_EID,    0x0000},
          {@APP@_COMMANDRST_INF_EID,    0x0000},
          {@APP@_INVALID_MSGID_ERR_EID, 0x0000},
          {@APP@_LEN_ERR_EID,           0x0000},
       };

/*
** @APP@_Main() -- Application entry point and main process loop
*/
void @APP@_Main( void )
{
    int32  status;
    uint32 RunStatus = CFE_ES_RunStatus_APP_RUN;

    CFE_ES_PerfLogEntry(@APP@_PERF_ID);

    @APP@_Init();

    /*
    ** @APP@ Runloop
    */
    while (CFE_ES_RunLoop(&RunStatus) == true)
    {
        CFE_ES_PerfLogExit(@APP@_PERF_ID);

        /* Pend on receipt of command packet -- timeout set to 500 millisecs */
        status = CFE_SB_RcvMsg(&@APP@_MsgPtr, @APP@_CommandPipe, 500);

        CFE_ES_PerfLogEntry(@APP@_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            @APP@_ProcessCommandPacket();
        }
    }

    CFE_ES_ExitApp(RunStatus);

} /* End of @APP@_Main() */

/*
** @APP@_Init() -- initialization
*/
void @APP@_Init(void)
{
    /*
    ** Register the app with Executive services
    */
    CFE_ES_RegisterApp();

    /*
    ** Register the events
    */
    CFE_EVS_Register(@APP@_EventFilters,
                     sizeof(@APP@_EventFilters)/sizeof(CFE_EVS_BinFilter_t),
                     CFE_EVS_EventFilter_BINARY);

    /*
    ** Create the Software Bus command pipe and subscribe to the command
    ** and housekeeping request messages
    */
    CFE_SB_CreatePipe(&@APP@_CommandPipe, @APP@_PIPE_DEPTH, "@APP@_PIPE");
    CFE_SB_Subscribe(@APP@_CMD_MID, @APP@_CommandPipe);
    CFE_SB_Subscribe(@APP@_SEND_HK_MID, @APP@_CommandPipe);

    /*
    ** Initialize the housekeeping packet (this also clears the counters)
    */
    CFE_SB_InitMsg(&@APP@_HkTelemetryPkt,
                   @APP@_HK_TLM_MID,
                   @APP@_HK_TLM_LNGTH, true);

    CFE_EVS_SendEvent(@APP@_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "@APP@ App Initialized. Version %d.%d.%d.%d",
                      @APP@_MAJOR_VERSION,
                      @APP@_MINOR_VERSION,
                      @APP@_REVISION,
                      @APP@_MISSION_REV);

} /* End of @APP@_Init() */

/*
** @APP@_ProcessCommandPacket() -- process a packet received on the command pipe
*/
void @APP@_ProcessCommandPacket(void)
{
    CFE_SB_MsgId_t  MsgId;

    MsgId = CFE_SB_GetMsgId(@APP@_MsgPtr);

    switch (MsgId)
    {
        case @APP@_CMD_MID:
            @APP@_ProcessGroundCommand();
            break;

        case @APP@_SEND_HK_MID:
            @APP@_ReportHousekeeping();
            break;

        default:
            @APP@_HkTelemetryPkt.command_error_count++;
            CFE_EVS_SendEvent(@APP@_INVALID_MSGID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "@APP@: invalid command packet, MID = 0x%x", MsgId);
            break;
    }

} /* End of @APP@_ProcessCommandPacket() */

/*
** @APP@_ProcessGroundCommand() -- @APP@ ground commands
*/
void @APP@_ProcessGroundCommand(void)
{
    uint16 CommandCode;

    CommandCode = CFE_SB_GetCmdCode(@APP@_MsgPtr);

    /* Process "known" @APP@ app ground commands */
    switch (CommandCode)
    {
        case @APP@_NOOP_CC:
            if (@APP@_VerifyCmdLength(@APP@_MsgPtr, sizeof(@APP@_NoArgsCmd_t)))
            {
                @APP@_HkTelemetryPkt.command_count++;
                CFE_EVS_SendEvent(@APP@_COMMANDNOP_INF_EID,
                                  CFE_EVS_EventType_INFORMATION,
                                  "@APP@: NOOP command");
            }
            break;

        case @APP@_RESET_COUNTERS_CC:
            if (@APP@_VerifyCmdLength(@APP@_MsgPtr, sizeof(@APP@_NoArgsCmd_t)))
            {
                @APP@_ResetCounters();
            }
            break;

        default:
            @APP@_HkTelemetryPkt.command_error_count++;
            CFE_EVS_SendEvent(@APP@_COMMAND_ERR_EID, CFE_EVS_EventType_ERROR,
                              "@APP@: invalid command code, CC = %d", CommandCode);
            break;
    }

} /* End of @APP@_ProcessGroundCommand() */

/*
** @APP@_ReportHousekeeping() -- send the housekeeping telemetry packet
**
** This function is triggered in response to a housekeeping request
** (@APP@_SEND_HK_MID). It time stamps the housekeeping packet and sends it
** on the software bus.
*/
void @APP@_ReportHousekeeping(void)
{
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &@APP@_HkTelemetryPkt);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &@APP@_HkTelemetryPkt);

} /* End of @APP@_ReportHousekeeping() */

/*
** @APP@_ResetCounters() -- reset the housekeeping counters
*/
void @APP@_ResetCounters(void)
{
    @APP@_HkTelemetryPkt.command_count       = 0;
    @APP@_HkTelemetryPkt.command_error_count = 0;

    CFE_EVS_SendEvent(@APP@_COMMANDRST_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "@APP@: RESET command");

} /* End of @APP@_ResetCounters() */

/*
** @APP@_VerifyCmdLength() -- verify command packet length
*/
bool @APP@_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength)
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

        CFE_EVS_SendEvent(@APP@_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid msg length: ID = 0x%X,  CC = %d, Len = %d, Expected = %d",
                          MessageID, CommandCode, ActualLength, ExpectedLength);
        result = false;
        @APP@_HkTelemetryPkt.command_error_count++;
    }

    return result;

} /* End of @APP@_VerifyCmdLength() */
"""

TEMPLATES = [
    ("CMakeLists.txt", CMAKELISTS),
    ("fsw/mission_inc/@app@_perfids.h", PERFIDS_H),
    ("fsw/platform_inc/@app@_msgids.h", MSGIDS_H),
    ("fsw/src/@app@.c", APP_C),
    ("fsw/src/@app@.h", APP_H),
    ("fsw/src/@app@_events.h", EVENTS_H),
    ("fsw/src/@app@_msg.h", MSG_H),
    ("fsw/src/@app@_version.h", VERSION_H),
]


def normalize_name(raw):
    """Turn "HelloWorld", "helloWorld" or "HELLO_WORLD" into "hello_world"."""
    name = re.sub(r"(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])", "_", raw.strip())
    return name.lower()


def check_name(name):
    """Return an error message if name cannot be used, otherwise None."""
    if not re.match(r"^[a-z][a-z0-9_]*$", name) or "__" in name or name.endswith("_"):
        return ("the name must start with a letter and contain only letters, digits "
                "and single underscores")
    for prefix in RESERVED_PREFIXES:
        if name == prefix or name.startswith(prefix + "_"):
            return "names starting with '%s' are reserved for the framework" % prefix

    upper = name.upper()
    runtime_names = [
        ("app name", upper),
        ("entry point", upper + "_Main"),
        ("pipe name", upper + "_PIPE"),
    ]
    for what, value in runtime_names:
        if len(value) >= OS_MAX_API_NAME:
            return ("the name is too long: the %s %s has %d characters, but cFE allows "
                    "at most %d" % (what, value, len(value), OS_MAX_API_NAME - 1))
    return None


def scan_defines(suffix, value_pattern):
    """Collect the values of '#define *<suffix>' lines in the scanned headers.

    All numbers on a matching line are collected, so defines like
    'CFE_MISSION_CMD_MID_BASE1 + ... /* 0x1801 */' are covered by their comment.
    """
    define = re.compile(r"#define\s+\w+" + suffix + r"\b")
    number = re.compile(value_pattern)
    values = set()
    for scan_dir in ID_SCAN_DIRS:
        for root, _, files in os.walk(scan_dir):
            for file_name in files:
                if not file_name.endswith(".h"):
                    continue
                with open(os.path.join(root, file_name), encoding="utf-8", errors="replace") as f:
                    for line in f:
                        if define.search(line):
                            values.update(int(v, 0) for v in number.findall(line))
    return values


def pick_message_ids():
    used = scan_defines("_MID", r"\b0x[0-9A-Fa-f]+\b")
    for apid in range(FIRST_APID, LAST_APID, 2):
        cmd_mid = 0x1800 | apid
        send_hk_mid = 0x1800 | (apid + 1)
        hk_tlm_mid = 0x0800 | apid
        if not {cmd_mid, send_hk_mid, hk_tlm_mid} & used:
            return cmd_mid, send_hk_mid, hk_tlm_mid
    sys.exit("error: no free message IDs left between 0x%02X and 0x%02X"
             % (FIRST_APID, LAST_APID))


def pick_perf_id():
    used = scan_defines("_PERF_ID", r"\b\d+\b")
    for perf_id in range(FIRST_PERF_ID, LAST_PERF_ID + 1):
        if perf_id not in used:
            return perf_id
    sys.exit("error: no free performance IDs left between %d and %d"
             % (FIRST_PERF_ID, LAST_PERF_ID))


def main():
    parser = argparse.ArgumentParser(
        description="Create a skeleton cFS application under apps/.")
    parser.add_argument("name", help="application name, e.g. hello_world or HelloWorld")
    args = parser.parse_args()

    name = normalize_name(args.name)
    error = check_name(name)
    if error:
        sys.exit("error: cannot use '%s': %s" % (args.name, error))

    app_dir = os.path.join(APPS_DIR, name)
    if os.path.exists(app_dir):
        sys.exit("error: %s already exists" % os.path.relpath(app_dir, TOP_DIR))

    cmd_mid, send_hk_mid, hk_tlm_mid = pick_message_ids()
    perf_id = pick_perf_id()
    upper = name.upper()

    replacements = {
        "@app@": name,
        "@APP@": upper,
        "@CMD_MID@": "0x%04X" % cmd_mid,
        "@SEND_HK_MID@": "0x%04X" % send_hk_mid,
        "@HK_TLM_MID@": "0x%04X" % hk_tlm_mid,
        "@PERF_ID@": str(perf_id),
    }

    def fill(text):
        for key, value in replacements.items():
            text = text.replace(key, value)
        return text

    for rel_path, template in TEMPLATES:
        path = os.path.join(app_dir, fill(rel_path))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(fill(template))
        print("  created apps/%s/%s" % (name, fill(rel_path)))

    print("""
New app "{name}" created in apps/{name}/

  {upper}_CMD_MID      = 0x{cmd:04X}   (commands)
  {upper}_SEND_HK_MID  = 0x{hk_req:04X}   (housekeeping request)
  {upper}_HK_TLM_MID   = 0x{hk_tlm:04X}   (housekeeping telemetry)
  {upper}_PERF_ID      = {perf}

Next steps:
  1. Add {name} to TGT1_APPLIST in sample_defs/targets.cmake
  2. Add this line to sample_defs/cpu1_cfe_es_startup.scr, above the "!" line:
       CFE_APP, /cf/{name}.so, {upper}_Main, {upper}, 90, 16384, 0x0, 0;
  3. Build and install (make, make install), then start core-cpu1 in build/exe/cpu1
  4. Send a NOOP command from tools/cFS-GroundSystem/Subsystems/cmdUtil:
       ./cmdUtil --host=127.0.0.1 --pktid=0x{cmd:04X} --endian=LE --cmdcode=0

To receive housekeeping telemetry on the ground, also:
  - add {{{upper}_SEND_HK_MID, 4, 0}} to apps/sch_lab/fsw/platform_inc/sch_lab_sched_tab.h
  - add {{{upper}_HK_TLM_MID, {{0,0}}, 4}} to apps/to_lab/fsw/platform_inc/to_lab_sub_table.h
  - #include "{name}_msgids.h" in both files, and add
    include_directories(${{{name}_MISSION_DIR}}/fsw/platform_inc)
    to apps/sch_lab/CMakeLists.txt and apps/to_lab/CMakeLists.txt""".format(
        name=name, upper=upper, cmd=cmd_mid, hk_req=send_hk_mid, hk_tlm=hk_tlm_mid,
        perf=perf_id))


if __name__ == "__main__":
    main()
