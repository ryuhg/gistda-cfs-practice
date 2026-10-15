/************************************************************************
** File: ds_filter_tbl.c
**
** Purpose:
**   DS packet filter table for the IMU example of the cFS hands-on
**   course. cFE builds this file instead of apps/ds/fsw/tables/ds_filter_tbl.c
**   because it is in sample_defs/tables (see add_cfe_tables in
**   cfe/cmake/arch_build.cmake).
**
**   DS subscribes to every message ID listed here. Each packet can go to
**   up to 4 destination files; each filter is
**     { file table index, filter type, N, X, O }
**   and stores N packets out of every X, starting at offset O.
**   { FILE_IMU_DATA, DS_BY_COUNT, 1, 1, 0 } stores every packet.
**
*************************************************************************/

#include "cfe.h"
#include "cfe_tbl_filedef.h"
#include "ds_platform_cfg.h"
#include "ds_appdefs.h"
#include "ds_app.h"
#include "ds_msg.h"

#include "imu_app_msgids.h"

/*
** File table index numbers, defined in ds_file_tbl.c
*/
#define FILE_IMU_DATA   0

DS_FilterTable_t DS_FilterTable =
{
  /* .Descriptor = */ "IMU example filter table",
  /* .Packet     = */
  {
    /* Packet Index 000 -- IMU_APP sensor data */
    {
      /* .MessageID = */ IMU_APP_DATA_TLM_MID,
      /* .Filter    = */
      {
        /* File table index, filter type, N, X, O */
        { FILE_IMU_DATA, DS_BY_COUNT, 1, 1, 0 },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED },
        { DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED, DS_UNUSED }
      }
    },
    /* Packet Index 001..255 are unused (all zero) */
  }
};

/*
** Packet filter table header
*/
CFE_TBL_FILEDEF(DS_FilterTable, DS.FILTER_TBL, DS Packet Filter Table,ds_filter_tbl.tbl)
