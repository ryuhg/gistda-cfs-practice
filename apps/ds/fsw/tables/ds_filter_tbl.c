/************************************************************************
** File: ds_filter_tbl.c 
**
** File: ds_filter_tbl.c
**
**  NASA Docket No. GSC-18448-1, and identified as "cFS Data Storage (DS) 
**  application version 2.5.2” 
**  
**  Copyright © 2019 United States Government as represented by the Administrator 
**  of the National Aeronautics and Space Administration.  All Rights Reserved. 
**
**  Licensed under the Apache License, Version 2.0 (the "License"); 
**  you may not use this file except in compliance with the License. 
**  You may obtain a copy of the License at 
**  http://www.apache.org/licenses/LICENSE-2.0 
**  Unless required by applicable law or agreed to in writing, software 
**  distributed under the License is distributed on an "AS IS" BASIS, 
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
**  See the License for the specific language governing permissions and 
**  limitations under the License. 
**  
**
** Purpose:
**   CFS Data Storage (DS) packet filter table, shortened for the cFS
**   hands-on course.
**
**   DS subscribes to every message ID listed here. Each packet can go to
**   up to 4 destination files (ds_file_tbl.c); each filter is
**     { file table index, filter type, N, X, O }
**   and stores N packets out of every X, starting at offset O, so
**   { 0, DS_BY_COUNT, 1, 1, 0 } stores every packet in file 0.
**
**   Five entries are written out below and all of them are empty; the
**   remaining entries up to DS_PACKETS_IN_FILTER_TABLE are left out and
**   are therefore empty as well.
**
*************************************************************************/

#include "cfe.h"
#include "cfe_tbl_filedef.h"
#include "ds_platform_cfg.h"
#include "ds_appdefs.h"
#include "ds_app.h"
#include "ds_msg.h"

/*
** Include the headers that define the message IDs of the packets to store
*/

/*
** Packet filter table data
*/
DS_FilterTable_t DS_FilterTable =
{
  /* .Descriptor = */ "Course filter table",
  /* .Packet     = */
  {
    /* Packet Index 000 -- unused */
    {
      /* .MessageID = */ DS_UNUSED,
      /* .Filter    = */
      {
        /* File table index, filter type, N, X, O */
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED }
      }
    },
    /* Packet Index 001 -- unused */
    {
      /* .MessageID = */ DS_UNUSED,
      /* .Filter    = */
      {
        /* File table index, filter type, N, X, O */
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED }
      }
    },
    /* Packet Index 002 -- unused */
    {
      /* .MessageID = */ DS_UNUSED,
      /* .Filter    = */
      {
        /* File table index, filter type, N, X, O */
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED }
      }
    },
    /* Packet Index 003 -- unused */
    {
      /* .MessageID = */ DS_UNUSED,
      /* .Filter    = */
      {
        /* File table index, filter type, N, X, O */
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED }
      }
    },
    /* Packet Index 004 -- unused */
    {
      /* .MessageID = */ DS_UNUSED,
      /* .Filter    = */
      {
        /* File table index, filter type, N, X, O */
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED }
      }
    }
  }
};

/*
** Packet filter table header
*/
CFE_TBL_FILEDEF(DS_FilterTable, DS.FILTER_TBL, DS Packet Filter Table,ds_filter_tbl.tbl)


/************************/
/*  End of File Comment */
/************************/
