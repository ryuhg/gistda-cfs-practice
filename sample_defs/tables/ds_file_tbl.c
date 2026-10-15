/************************************************************************
** File: ds_file_tbl.c
**
** Purpose:
**   DS destination file table for the IMU example of the cFS hands-on
**   course. cFE builds this file instead of apps/ds/fsw/tables/ds_file_tbl.c
**   because it is in sample_defs/tables (see add_cfe_tables in
**   cfe/cmake/arch_build.cmake).
**
**   One destination is defined: the IMU_APP sensor data packets selected
**   by ds_filter_tbl.c are written to /cf, which is the directory
**   build/exe/cpu1/cf on the PC. The files are named imu00000001.dat,
**   imu00000002.dat, ... and a new file is started every minute.
**
**   DS updates the file age on each housekeeping request (every 4 seconds
**   from SCH_LAB), so the age limit only works while those requests arrive.
**
**   The numbering starts again at 1 every time cFS starts, so files of
**   the previous run are overwritten.
**
*************************************************************************/

#include "cfe.h"
#include "cfe_tbl_filedef.h"
#include "ds_platform_cfg.h"
#include "ds_appdefs.h"
#include "ds_app.h"
#include "ds_msg.h"

/*
** File table index numbers, also used by ds_filter_tbl.c
*/
#define FILE_IMU_DATA   0

DS_DestFileTable_t DS_DestFileTable =
{
  /* .Descriptor = */ "IMU example file table",
  /* .File       = */
  {
    /* File Index 00 -- IMU_APP sensor data */
    {
#if (DS_MOVE_FILES == true)
      /* .Movename      = */ DS_EMPTY_STRING,           /* do not move closed files */
#endif
      /* .Pathname      = */ "/cf/",
      /* .Basename      = */ "imu",
      /* .Extension     = */ ".dat",

      /* .FileNameType  = */ DS_BY_COUNT,
      /* .EnableState   = */ DS_ENABLED,
      /* .MaxFileSize   = */ (1024 * 64),               /* 64 K-bytes */
      /* .MaxFileAge    = */ 60,                        /* 1 minute */
      /* .SequenceCount = */ 1,
    },
    /* File Index 01..15 are unused (all zero) */
  }
};

/*
** Destination file table header
*/
CFE_TBL_FILEDEF(DS_DestFileTable, DS.FILE_TBL, DS Destination File Table,ds_file_tbl.tbl)
