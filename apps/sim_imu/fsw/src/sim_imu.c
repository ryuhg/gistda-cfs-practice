/*************************************************************************
** File: sim_imu.c
**
** Purpose:
**   Simulated IMU: returns mock x, y and z values (acceleration in g).
**
**   The values are computed from the clock on each call, with a sine
**   table instead of the math library, so the library behaves the same
**   on every computer.
**
*************************************************************************/

#include "cfe.h"
#include "sim_imu.h"
#include "sim_imu_version.h"

static uint32 SIM_IMU_NoiseState;

/*
** One cycle of a sine wave in 64 steps, scaled to +/-1000
*/
static const int16 SIM_IMU_SineTable[64] =
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
** Sine (-1.0 .. +1.0) of the current point in a cycle of PeriodMs milliseconds.
** PhaseSteps shifts the wave by 1/64 cycle per step (16 steps = cosine).
*/
static float SIM_IMU_Sine(uint32 TimeMs, uint32 PeriodMs, uint32 PhaseSteps)
{
    uint32 Index = ((TimeMs % PeriodMs) * 64) / PeriodMs;

    return SIM_IMU_SineTable[(Index + PhaseSteps) % 64] / 1000.0f;
}

/*
** Small pseudo-random noise in the range -0.003 .. +0.003
*/
static float SIM_IMU_Noise(void)
{
    SIM_IMU_NoiseState = SIM_IMU_NoiseState * 1103515245u + 12345u;

    return (float)((int32)((SIM_IMU_NoiseState >> 16) % 7) - 3) / 1000.0f;
}

/*
** Library entry point
*/
int32 SIM_IMU_LibInit(void)
{
    SIM_IMU_NoiseState = 12345;

    OS_printf("SIM_IMU Lib Initialized. Version %d.%d.%d.%d\n",
              SIM_IMU_MAJOR_VERSION,
              SIM_IMU_MINOR_VERSION,
              SIM_IMU_REVISION,
              SIM_IMU_MISSION_REV);

    return CFE_SUCCESS;
}

/*
** Read the latest x, y and z values
*/
int32 SIM_IMU_Read(float *X, float *Y, float *Z)
{
    OS_time_t Now;
    uint32    TimeMs;

    if (X == NULL || Y == NULL || Z == NULL)
    {
        return SIM_IMU_ERROR;
    }

    if (OS_GetLocalTime(&Now) != OS_SUCCESS)
    {
        Now.seconds   = 0;
        Now.microsecs = 0;
    }

    /* Milliseconds within the hour; the 20 s period divides one hour,
    ** so the wave stays continuous when this wraps around */
    TimeMs = (Now.seconds % 3600) * 1000 + Now.microsecs / 1000;

    *X = 0.1f * SIM_IMU_Sine(TimeMs, 20000, 0)  + SIM_IMU_Noise();
    *Y = 0.1f * SIM_IMU_Sine(TimeMs, 20000, 16) + SIM_IMU_Noise();
    *Z = 1.0f + SIM_IMU_Noise();

    return SIM_IMU_SUCCESS;
}
