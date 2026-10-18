/*************************************************************************
** File: sim_imu.h
**
** Purpose:
**   Public interface of the simulated IMU library.
**
**   The library stands in for an IMU and its driver so the course can
**   read a "sensor" on any computer. Each call returns three float values,
**   x, y and z (acceleration in g): a board lying flat and slowly
**   wobbling, so z is about 1.0 and x and y swing between -0.1 and +0.1
**   over 20 seconds, with a little noise.
**
*************************************************************************/
#ifndef _sim_imu_h_
#define _sim_imu_h_

#include "common_types.h"

/*
** Return codes
*/
#define SIM_IMU_SUCCESS          0
#define SIM_IMU_ERROR           -1   /* NULL pointer given */

/*
** Library entry point, called by cFE ES at startup (see cfe_es_startup.scr)
*/
int32 SIM_IMU_LibInit(void);

/*
** Read the latest x, y and z values
*/
int32 SIM_IMU_Read(float *X, float *Y, float *Z);

#endif /* _sim_imu_h_ */
