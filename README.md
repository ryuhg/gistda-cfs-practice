# cFS Hands-on Practice (GISTDA training fork)

This repository is a training fork of the NASA [cFS bundle](https://github.com/nasa/cFS) at release **Aquila (cFS 6.7.0a)**. It is used for the hands-on exercises of a cFS introductory course. Aquila was chosen over later releases (Caelum, Draco) because its configuration is simpler for first-time users.

The framework is the same as the upstream tag `v6.7.0a`: cFE, OSAL, PSP, the lab apps and the tools are git submodules that point to the NASA repositories, and the build files (`Makefile`, `sample_defs/`) are not included. Setting them up is part of the exercise: follow the **Setup** and **Build and Run** sections of the upstream README below, but clone this repository instead of `nasa/cFS`:

```sh
git clone https://github.com/ryuhg/gistda-cfs-practice.git
cd gistda-cfs-practice
```

## Course materials

These are added to the upstream bundle. They are plain directories and files (not submodules), so they are there right after `git clone`. The apps and libraries under `apps/` are not built until you add them to the mission configuration (`sample_defs/`).

| Path | What it is |
|---|---|
| `create_app.py` | Creates a new app from the sample_app layout (see below) |
| `ground.py` | Sends commands, shows telemetry, reads files written by DS (see below) |
| `apps/sim_imu` | Simulated IMU library: the "sensor and driver" for the exercises (see below) |
| `apps/ds` | Data Store app 2.5.2 from [nasa/DS](https://github.com/nasa/DS), without `docs/`, `test_and_ground/` and unit tests. The destination file and packet filter tables in `fsw/tables/` are shortened to five empty entries each. |
| `apps/cfs_lib` | cFS library 2.2.0 from [nasa/cfs_lib](https://github.com/nasa/cfs_lib) (NASA Open Source Agreement), needed by DS. Added a `CMakeLists.txt` and fixed a pointer type error that stops the build with GCC 14. |

cFE, OSAL and PSP are not changed.

## Notes for this course

- Tested on Ubuntu 24.04 as a normal user, with GCC 13.3 and GCC 14.2 (CMake 3.28). Required packages: `sudo apt install build-essential cmake git python3`.
- After `git clone`, the directories `cfe/`, `osal/`, `psp/`, `apps/ci_lab`, `apps/sample_app`, `apps/sample_lib`, `apps/sch_lab`, `apps/to_lab` and `tools/*` are empty until you run `git submodule init` and `git submodule update`.
- If you run cFS as a normal user (not root), do the `OSAL_DEBUG_PERMISSIVE_MODE` step of the Setup section. Without it, `core-cpu1` exits right after starting with `CFE_PSP: OS_API_Init() failure`.
- Run `core-cpu1` from `build/exe/cpu1`, because it loads the startup script and apps from `./cf`. Stop it with Ctrl+C. Run only one cFS at a time on a computer (it uses UDP ports 1234 and 1235).
- The build prints a few `-Waddress-of-packed-member` warnings for apps that send packed telemetry (sample_app, ci_lab and apps made with `create_app.py`). These are expected and harmless.
- Most timestamps on the console read `1980-012-...` because the cFE clock has not been set. This is normal.

## Simulated IMU: apps/sim_imu

A cFE library that stands in for an IMU and its driver, so the exercises can read a sensor on any computer. It runs inside cFS and needs no hardware, network port or extra program.

| | |
|---|---|
| Library name | `sim_imu` (module `sim_imu.so`) |
| Entry point | `int32 SIM_IMU_LibInit(void)`, prints `SIM_IMU Lib Initialized` |
| Header | `apps/sim_imu/fsw/public_inc/sim_imu.h` |
| Read | `int32 SIM_IMU_Read(float *X, float *Y, float *Z)`, returns `SIM_IMU_SUCCESS` or `SIM_IMU_ERROR` |

Each read returns three float values, acceleration in g, for a board lying flat and slowly wobbling: z is about 1.0, and x and y swing between -0.1 and +0.1 over 20 seconds, with a little noise.

## Ground tool: ground.py

`ground.py` sends commands, shows telemetry and reads DS files. It needs only Python 3 (no extra packages, tested with 3.8 to 3.13 on Linux) and uses nothing Linux-specific, so it can also run on a Windows or macOS host. With `core-cpu1` running, open a second terminal in the repository:

```sh
python3 ground.py send 0x1882 0                         # SAMPLE_APP NOOP: the console prints "SAMPLE: NOOP command"
python3 ground.py dump build/exe/cpu1/cf/<file>         # print the packets in a file written by DS
python3 ground.py listen                                # show telemetry sent by TO_LAB (Ctrl+C to stop)
```

`send` takes a message ID and a command code. Parameters are added in order with `--u8`, `--u16`, `--u32`, `--i8`, `--i16`, `--i32`, `--f32` and `--string LEN:TEXT`. If cFS runs on another computer, add `--host <its IP address>`.

`dump` prints the file headers, then one line per stored packet with its cFE time stamp.

`listen` asks TO_LAB to start sending telemetry, and repeats this every 5 seconds while nothing arrives, so it does not matter whether cFS or `ground.py` starts first. It shows only the packets that TO_LAB is configured to send.

Run `python3 ground.py <command> --help` for all options.

## Creating a new app: create_app.py

```sh
python3 create_app.py hello_world
```

creates `apps/hello_world/` from the sample_app layout (command pipe, NOOP and RESET_COUNTERS commands, housekeeping; no tables), with message IDs that do not collide with other apps. It then prints the remaining steps: adding the app to `sample_defs/targets.cmake` and `sample_defs/cpu1_cfe_es_startup.scr`, and optionally to SCH_LAB and TO_LAB for housekeeping.

## Known limitations

- `tools/cFS-GroundSystem` (v2.1.0) needs Python 2 and PyQt4, which are not packaged for Ubuntu 20.04 or later, so the GUI ground system will not start there. Use `ground.py` instead.
- If you use `cmdUtil` from cFS-GroundSystem, `--endian=LE` must come before `--cmdcode` on x86 PCs; otherwise the command code is written to the wrong byte and every command arrives as NOOP. For the same reason the bundled `to-enable-tlm.sh` sends a NOOP instead of enabling telemetry.
- The ES "restart app" command (`CFE_ES_RESTART_APP_CC`) does not restart apps that use the sample_app main loop (SAMPLE_APP, the lab apps and apps made with `create_app.py`): ES deletes the app and logs `Exit Application ... on Error Completed`. This is a bug in cFE 6.7, fixed in later cFE releases, and is not patched here.

---

*The original upstream README of the cFS bundle follows.*

# Core Flight System - BUNDLE

This repository is a bundle of submodules that make up the Core Flight System (cFS) framework.  Note the "lab" apps are intended as examples only, and enable this bundle to build, execute, receive commands, and send telemetry.  This is not a flight distribution, which is typically made up of the cFE, OSAL, PSP, and a selection of flight apps that coorespond to specific mission requirements.

This bundle is not fully verified as an operational system, and is provided as a starting point vs an end product.  Testing of this bundle consists of building, executing, sending setup commands and verifying receipt of telemetry.  Unit testing is also run, but extensive analysis is not performed.  All verification and validation per mission requirements is the responsibility of the mission (although attempts are made in the cFS Framework to provide a testing framework to facilitate the process).

The cFS Framework is a core subset of cFS.  There are additional OSALs, PSPs, and tools as listed below available from a variety of sources.

## Release Notes

See [releases](https://github.com/nasa/cFS/releases) for release history and associated artifacts related to the cFS BUNDLE.

As of release Aquila (aka cFS 6.7.0, which includes cFE 6.7.0 and OSAL 5.0.0) the entire bundle is released under the Apache 2.0 license.

Other elements listed below are released under a varitey of licenses as detailed in their respective repositories.

## Known issues

Major known issues are documented in the version description documents included with releases, and issue history in each of the submodules contain all the details.

## Major future work

  - Certification framework including PSP and OSAL white box testing
  - Open source automated build framework
  - Unit test modernization/standardization
  - Deployment quality of life improvements (configuration)
  - Documentation (updated tracability, APIs/ICDs, general update)
  - Symmetric multi-processing APIs
  - Electronic Data Sheet integration
  - Toolchain updates

## Getting Help

For best results, submit issues:questions or issues:help wanted requests to this repo.

Official cFS page: http://cfs.gsfc.nasa.gov

## Setup

To setup the cFS BUNDLE directly from the latest set of interoperable repositories:

    git clone https://github.com/nasa/cFS.git
    cd cFS
    git submodule init
    git submodule update

Copy in the default makefile and definitions:

    cp cfe/cmake/Makefile.sample Makefile
    cp -r cfe/cmake/sample_defs sample_defs

If running on a standard linux build as a normal user, define OSAL_DEBUG_PERMISSIVE_MODE for best effort message queue depth and task priorities.

    sed -i 's/undef OSAL_DEBUG_PERMISSIVE_MODE/define OSAL_DEBUG_PERMISSIVE_MODE/g' sample_defs/default_osconfig.h

## Build and Run

The cFS Framework including sample applications will build and run on the pc-linux platform support package (should run on most Linux distributions), via the steps described in https://github.com/nasa/cFE/tree/master/cmake/README.md.  Quick-start is below:

To prep, compile, and run (from cFS directory above):

    make prep
    make
    make install
    cd build/exe/cpu1/
    ./core-cpu1

Should see startup messages, and CFE_ES_Main entering OPERATIONAL state.  Note the code must be executed from the build/exe/cpu1 directory to find the startup script and shared objects.

## Send commands, receive telemetry

The cFS-GroundSystem tool can be used to send commands and receive telemetry (see https://github.com/nasa/cFS-GroundSystem/tree/master/Guide-GroundSystem.txt, the Guide-GroundSystem.txt).  Note it depends on PyQt4 and PyZMQ:

1. Install PyQt4 and PyZMQ on your system
2. Compile cmdUtil and start the ground system executable

       cd tools/cFS-GroundSystem/Subsystems/cmdUtil
       make
       cd ../..
       python GroundSystem.py
    
3. Select "Start Command System"
4. Select "Enable Tlm"
5. Enter IP address of system executing cFS, 127.0.0.1 if running locally

Should see telemetry, can send noops and see command counters increment

## Compatible list of cFS apps

The following applications have been tested against this release:
  - TBD

## Other cFS related elements/tools/apps/distributions

The following list is user submitted, and not CCB controlled.  They are released by various orgainizations, under various licenses.

  - Distributions
    - cFS-101: Virtual machine distribution at https://github.com/nasa/CFS-101
    - OpenSatKit: Open source kit for satelite software at https://github.com/OpenSatKit/OpenSatKit
  - Other Ground station software
    - TBD
  - Other Apps
    - CF: CFDP application at https://github.com/nasa/CF
    - HK: Housekeeping application at https://github.com/nasa/HK
    - MD: Memory Dwell application at https://github.com/nasa/MD
    - MM: Memory Manager application at https://github.com/nasa/MM
    - LC: Limit Checker application at https://github.com/nasa/LC
    - CS: Checksum application at https://github.com/nasa/CS
    - FM: File Manager application at https://github.com/nasa/FM
    - SC: Stored Commands application at https://github.com/nasa/SC
    - SCH: Scheduler application at https://github.com/nasa/SCH
    - HS: Health and Safety application at https://github.com/nasa/HS
    - DS: Data Store application at https://github.com/nasa/DS
    - SCA: Stored Command Absolute application at https://github.com/nasa/SCA
  - Other Interfaces
    - SIL: Simulink Interface Layer at https://github.com/nasa/SIL
    - ECI: External Code Interface at https://github.com/nasa/ECI
  - Other Libraries
    - cFS_LIB: at https://github.com/nasa/cfs_lib
    - cFS_IO_LIB: IO library at https://github.com/nasa/CFS_IO_LIB
  - Other Tools
    - CCDD: Command and Data Dictionary Tool at https://github.com/nasa/CCDD
    - Perfutils-java: Java based performance analyzer for cFS at https://github.com/nasa/perfutils-java
    - gen_msgids: Deprecated tool that prints ApIDs
    - gen_sch_tbl: Tool to generated SCH app tables
  - Other OSALs
    - TBD
  - Other PSPs
    - TBD
