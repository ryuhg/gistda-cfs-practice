/*************************************************************************
** File: sim_i2c.h
**
** Purpose:
**   Public interface of the simulated I2C bus library.
**
**   The library stands in for an I2C bus driver so the course can read a
**   "sensor" on any computer. One device is attached to the bus: a 6-axis
**   IMU at address 0x68 that behaves like an InvenSense MPU-6050 (the same
**   register addresses and data format). It is simulated inside the cFS
**   process, so no hardware, extra program or network port is needed.
**
**   Like a real I2C device, a read or write starts at a register address
**   and continues through the following registers (auto-increment).
**
**   To use real hardware instead, only these two functions need a new
**   implementation (for example on Linux with /dev/i2c-N and ioctl).
**
*************************************************************************/
#ifndef _sim_i2c_h_
#define _sim_i2c_h_

#include "common_types.h"

/*
** Return codes
*/
#define SIM_I2C_SUCCESS          0
#define SIM_I2C_ERROR_NACK      -1   /* No device answered at this address */
#define SIM_I2C_ERROR_PARAM     -2   /* NULL buffer, zero length or register range past 0xFF */

/*
** Library entry point, called by cFE ES at startup (see cfe_es_startup.scr)
*/
int32 SIM_I2C_LibInit(void);

/*
** Read Length bytes from device DevAddr, starting at register RegAddr
*/
int32 SIM_I2C_Read(uint8 DevAddr, uint8 RegAddr, uint8 *Buffer, uint32 Length);

/*
** Write Length bytes to device DevAddr, starting at register RegAddr
*/
int32 SIM_I2C_Write(uint8 DevAddr, uint8 RegAddr, const uint8 *Buffer, uint32 Length);

#endif /* _sim_i2c_h_ */
