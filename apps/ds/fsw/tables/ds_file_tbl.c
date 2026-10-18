/************************************************************************
** File: ds_file_tbl.c 
**
** File: ds_file_tbl.c 
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
**   CFS Data Storage (DS) destination file table, shortened for the cFS
**   hands-on course.
**
**   Each entry describes one file that DS writes. Five entries are written
**   out below and all of them are empty; the remaining entries up to
**   DS_DEST_FILE_CNT are left out and are therefore empty as well.
**   Fill in an entry (path, base name, extension, name type, enable state,
**   size and age limits, first sequence number) to make DS write a file.
**
**   DS updates the file age on each housekeeping request (DS_SEND_HK_MID),
**   so the age limit only works while those requests arrive.
**
*************************************************************************/

#include "cfe.h"
#include "cfe_tbl_filedef.h"
#include "ds_platform_cfg.h"
#include "ds_appdefs.h"
#include "ds_app.h"
#include "ds_msg.h"

/*
** Destination file table data
*/
DS_DestFileTable_t DS_DestFileTable =
{
  /* .Descriptor = */ "Course file table",
  /* .File       = */
  {
    /* File Index 00 -- unused */
    {
#if (DS_MOVE_FILES == TRUE)
      /* .Movename      = */ DS_EMPTY_STRING,
#endif
      /* .Pathname      = */ DS_EMPTY_STRING,
      /* .Basename      = */ DS_EMPTY_STRING,
      /* .Extension     = */ DS_EMPTY_STRING,

      /* .FileNameType  = */ DS_UNUSED,
      /* .EnableState   = */ DS_UNUSED,
      /* .MaxFileSize   = */ DS_UNUSED,
      /* .MaxFileAge    = */ DS_UNUSED,
      /* .SequenceCount = */ DS_UNUSED,
    },
    /* File Index 01 -- unused */
    {
#if (DS_MOVE_FILES == TRUE)
      /* .Movename      = */ DS_EMPTY_STRING,
#endif
      /* .Pathname      = */ DS_EMPTY_STRING,
      /* .Basename      = */ DS_EMPTY_STRING,
      /* .Extension     = */ DS_EMPTY_STRING,

      /* .FileNameType  = */ DS_UNUSED,
      /* .EnableState   = */ DS_UNUSED,
      /* .MaxFileSize   = */ DS_UNUSED,
      /* .MaxFileAge    = */ DS_UNUSED,
      /* .SequenceCount = */ DS_UNUSED,
    },
    /* File Index 02 -- unused */
    {
#if (DS_MOVE_FILES == TRUE)
      /* .Movename      = */ DS_EMPTY_STRING,
#endif
      /* .Pathname      = */ DS_EMPTY_STRING,
      /* .Basename      = */ DS_EMPTY_STRING,
      /* .Extension     = */ DS_EMPTY_STRING,

      /* .FileNameType  = */ DS_UNUSED,
      /* .EnableState   = */ DS_UNUSED,
      /* .MaxFileSize   = */ DS_UNUSED,
      /* .MaxFileAge    = */ DS_UNUSED,
      /* .SequenceCount = */ DS_UNUSED,
    },
    /* File Index 03 -- unused */
    {
#if (DS_MOVE_FILES == TRUE)
      /* .Movename      = */ DS_EMPTY_STRING,
#endif
      /* .Pathname      = */ DS_EMPTY_STRING,
      /* .Basename      = */ DS_EMPTY_STRING,
      /* .Extension     = */ DS_EMPTY_STRING,

      /* .FileNameType  = */ DS_UNUSED,
      /* .EnableState   = */ DS_UNUSED,
      /* .MaxFileSize   = */ DS_UNUSED,
      /* .MaxFileAge    = */ DS_UNUSED,
      /* .SequenceCount = */ DS_UNUSED,
    },
    /* File Index 04 -- unused */
    {
#if (DS_MOVE_FILES == TRUE)
      /* .Movename      = */ DS_EMPTY_STRING,
#endif
      /* .Pathname      = */ DS_EMPTY_STRING,
      /* .Basename      = */ DS_EMPTY_STRING,
      /* .Extension     = */ DS_EMPTY_STRING,

      /* .FileNameType  = */ DS_UNUSED,
      /* .EnableState   = */ DS_UNUSED,
      /* .MaxFileSize   = */ DS_UNUSED,
      /* .MaxFileAge    = */ DS_UNUSED,
      /* .SequenceCount = */ DS_UNUSED,
    },
  }
};

/*
** Destination file table header
*/
CFE_TBL_FILEDEF(DS_DestFileTable, DS.FILE_TBL, DS Destination File Table,ds_file_tbl.tbl)


/************************/
/*  End of File Comment */
/************************/
