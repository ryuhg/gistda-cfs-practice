/*************************************************************************
** File: sim_i2c.c
**
** Purpose:
**   Simulated I2C bus with one simulated MPU-6050 style IMU at 0x68.
**
**   Register map used (same addresses as the MPU-6050 datasheet):
**     0x3B..0x48  sensor data, 7 signed 16-bit values, high byte first:
**                 ACCEL_X, ACCEL_Y, ACCEL_Z, TEMP, GYRO_X, GYRO_Y, GYRO_Z
**     0x6B        PWR_MGMT_1, bit 6 = SLEEP (set at power on)
**     0x75        WHO_AM_I, always reads 0x68
**
**   Scale factors (default full-scale ranges of the real device):
**     accelerometer  16384 counts per g        (+/-2 g)
**     gyroscope        131 counts per deg/s    (+/-250 deg/s)
**     temperature    degC = counts / 340 + 36.53
**
**   The sensor values are computed from the clock each time the data
**   registers are read, using integer arithmetic only, so the library
**   behaves the same on every computer.
**
**   The bus is not protected by a lock: in this course only one app
**   (IMU_APP) uses it.
**
*************************************************************************/

#include <string.h>

#include "cfe.h"
#include "sim_i2c.h"
#include "sim_i2c_version.h"

/*
** Simulated device
*/
#define SIM_MPU_I2C_ADDR            0x68
#define SIM_MPU_REG_ACCEL_XOUT_H    0x3B   /* first sensor data register */
#define SIM_MPU_REG_ACCEL_YOUT_H    0x3D
#define SIM_MPU_REG_ACCEL_ZOUT_H    0x3F
#define SIM_MPU_REG_TEMP_OUT_H      0x41
#define SIM_MPU_REG_GYRO_XOUT_H     0x43
#define SIM_MPU_REG_GYRO_YOUT_H     0x45
#define SIM_MPU_REG_GYRO_ZOUT_H     0x47
#define SIM_MPU_REG_LAST_DATA       0x48   /* last sensor data register */
#define SIM_MPU_DATA_LEN            14
#define SIM_MPU_REG_PWR_MGMT_1      0x6B
#define SIM_MPU_REG_WHO_AM_I        0x75
#define SIM_MPU_SLEEP_BIT           0x40

#define SIM_I2C_NUM_REGS            256

static uint8  SIM_I2C_Regs[SIM_I2C_NUM_REGS];
static uint32 SIM_I2C_NoiseState;

/*
** One cycle of a sine wave in 64 steps, scaled to +/-1000
*/
static const int16 SIM_I2C_SineTable[64] =
{
        0,    98,   195,   290,   383,   471,   556,   634,
      707,   773,   831,   882,   924,   957,   981,   995,
     1000,   995,   981,   957,   924,   882,   831,   773,
      707,   634,   556,   471,   383,   290,   195,    98,
        0,   -98,  -195,  -290,  -383,  -471,  -556,  -634,
     -707,  -773,  -831,  -882,  -924,  -957,  -981,  -995,
    -1000,  -995,  -981,  -957,  -924,  -882,  -831,  -773,
     -707,  -634,  -556,  -471,  -383,  -290,  -195,   -98,
};

/*
** Sine (+/-1000) of the current point in a cycle of PeriodMs milliseconds.
** PhaseSteps shifts the wave by 1/64 cycle per step (16 steps = cosine).
*/
static int32 SIM_I2C_Sine(uint32 TimeMs, uint32 PeriodMs, uint32 PhaseSteps)
{
    uint32 Index = ((TimeMs % PeriodMs) * 64) / PeriodMs;

    return SIM_I2C_SineTable[(Index + PhaseSteps) % 64];
}

/*
** Small pseudo-random noise in the range -Amplitude..+Amplitude
*/
static int32 SIM_I2C_Noise(int32 Amplitude)
{
    SIM_I2C_NoiseState = SIM_I2C_NoiseState * 1103515245u + 12345u;

    return (int32)((SIM_I2C_NoiseState >> 16) % (uint32)(2 * Amplitude + 1)) - Amplitude;
}

/*
** Store a signed 16-bit value in two registers, high byte first
*/
static void SIM_I2C_PutInt16(uint8 RegAddr, int32 Value)
{
    uint16 Raw = (uint16)(int16)Value;

    SIM_I2C_Regs[RegAddr]     = (uint8)(Raw >> 8);
    SIM_I2C_Regs[RegAddr + 1] = (uint8)(Raw & 0xFF);
}

/*
** Refresh the sensor data registers
*/
static void SIM_I2C_UpdateSensorData(void)
{
    OS_time_t Now;
    uint32    TimeMs;

    /* A sleeping device does not measure anything */
    if (SIM_I2C_Regs[SIM_MPU_REG_PWR_MGMT_1] & SIM_MPU_SLEEP_BIT)
    {
        memset(&SIM_I2C_Regs[SIM_MPU_REG_ACCEL_XOUT_H], 0, SIM_MPU_DATA_LEN);
        return;
    }

    if (OS_GetLocalTime(&Now) != OS_SUCCESS)
    {
        Now.seconds   = 0;
        Now.microsecs = 0;
    }

    /* Milliseconds within the hour. All periods below divide one hour,
    ** so the waves stay continuous when this wraps around. */
    TimeMs = (Now.seconds % 3600) * 1000 + Now.microsecs / 1000;

    /* Board lying flat and slowly wobbling:
    ** about 1 g on Z and +/-0.1 g on X and Y, 20 second cycle */
    SIM_I2C_PutInt16(SIM_MPU_REG_ACCEL_XOUT_H,
                     SIM_I2C_Sine(TimeMs, 20000, 0) * 1638 / 1000 + SIM_I2C_Noise(8));
    SIM_I2C_PutInt16(SIM_MPU_REG_ACCEL_YOUT_H,
                     SIM_I2C_Sine(TimeMs, 20000, 16) * 1638 / 1000 + SIM_I2C_Noise(8));
    SIM_I2C_PutInt16(SIM_MPU_REG_ACCEL_ZOUT_H,
                     16384 + SIM_I2C_Noise(8));

    /* Temperature about 25 C, +/-0.5 C, 60 second cycle */
    SIM_I2C_PutInt16(SIM_MPU_REG_TEMP_OUT_H,
                     -3920 + SIM_I2C_Sine(TimeMs, 60000, 0) * 170 / 1000 + SIM_I2C_Noise(2));

    /* Rotation: +/-3 deg/s on X and Y (10 second cycle), steady 1 deg/s on Z */
    SIM_I2C_PutInt16(SIM_MPU_REG_GYRO_XOUT_H,
                     SIM_I2C_Sine(TimeMs, 10000, 0) * 393 / 1000 + SIM_I2C_Noise(4));
    SIM_I2C_PutInt16(SIM_MPU_REG_GYRO_YOUT_H,
                     SIM_I2C_Sine(TimeMs, 10000, 16) * 393 / 1000 + SIM_I2C_Noise(4));
    SIM_I2C_PutInt16(SIM_MPU_REG_GYRO_ZOUT_H,
                     131 + SIM_I2C_Noise(4));
}

/*
** Library entry point
*/
int32 SIM_I2C_LibInit(void)
{
    /* Power-on state of the device: asleep */
    memset(SIM_I2C_Regs, 0, sizeof(SIM_I2C_Regs));
    SIM_I2C_Regs[SIM_MPU_REG_PWR_MGMT_1] = SIM_MPU_SLEEP_BIT;
    SIM_I2C_Regs[SIM_MPU_REG_WHO_AM_I]   = SIM_MPU_I2C_ADDR;

    SIM_I2C_NoiseState = 12345;

    OS_printf("SIM_I2C Lib Initialized. Version %d.%d.%d.%d, simulated IMU at 0x%02X\n",
              SIM_I2C_MAJOR_VERSION,
              SIM_I2C_MINOR_VERSION,
              SIM_I2C_REVISION,
              SIM_I2C_MISSION_REV,
              SIM_MPU_I2C_ADDR);

    return CFE_SUCCESS;
}

/*
** Read registers
*/
int32 SIM_I2C_Read(uint8 DevAddr, uint8 RegAddr, uint8 *Buffer, uint32 Length)
{
    if (DevAddr != SIM_MPU_I2C_ADDR)
    {
        return SIM_I2C_ERROR_NACK;
    }

    if (Buffer == NULL || Length == 0 || (uint32)RegAddr + Length > SIM_I2C_NUM_REGS)
    {
        return SIM_I2C_ERROR_PARAM;
    }

    /* Take a new measurement when the sensor data registers are read */
    if (RegAddr <= SIM_MPU_REG_LAST_DATA && (uint32)RegAddr + Length > SIM_MPU_REG_ACCEL_XOUT_H)
    {
        SIM_I2C_UpdateSensorData();
    }

    memcpy(Buffer, &SIM_I2C_Regs[RegAddr], Length);

    return SIM_I2C_SUCCESS;
}

/*
** Write registers
*/
int32 SIM_I2C_Write(uint8 DevAddr, uint8 RegAddr, const uint8 *Buffer, uint32 Length)
{
    uint32 i;
    uint32 Reg;

    if (DevAddr != SIM_MPU_I2C_ADDR)
    {
        return SIM_I2C_ERROR_NACK;
    }

    if (Buffer == NULL || Length == 0 || (uint32)RegAddr + Length > SIM_I2C_NUM_REGS)
    {
        return SIM_I2C_ERROR_PARAM;
    }

    for (i = 0; i < Length; i++)
    {
        Reg = (uint32)RegAddr + i;

        /* WHO_AM_I and the sensor data registers are read-only */
        if (Reg != SIM_MPU_REG_WHO_AM_I &&
            (Reg < SIM_MPU_REG_ACCEL_XOUT_H || Reg > SIM_MPU_REG_LAST_DATA))
        {
            SIM_I2C_Regs[Reg] = Buffer[i];
        }
    }

    return SIM_I2C_SUCCESS;
}
