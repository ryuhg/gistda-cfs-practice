# cFS Hands-on Practice (GISTDA training fork)

This repository is a training fork of the NASA [cFS bundle](https://github.com/nasa/cFS) at release **Aquila (cFS 6.7.0a)**. It is used for the hands-on exercises of a cFS introductory course. Aquila was chosen over later releases (Caelum, Draco) because its configuration is simpler for first-time users.

## Differences from upstream

- **No submodules.** All components are stored as plain directories, so a normal `git clone` is enough:

  | Directory | Version | Upstream |
  |---|---|---|
  | `cfe` | 6.7.0 | [nasa/cFE](https://github.com/nasa/cFE) |
  | `osal` | 5.0.0 | [nasa/osal](https://github.com/nasa/osal) |
  | `psp` | 1.4.0 | [nasa/PSP](https://github.com/nasa/PSP) |
  | `apps/ci_lab`, `apps/to_lab`, `apps/sch_lab` | 2.3.0 | [nasa/ci_lab](https://github.com/nasa/ci_lab), [nasa/to_lab](https://github.com/nasa/to_lab), [nasa/sch_lab](https://github.com/nasa/sch_lab) |
  | `apps/sample_app`, `apps/sample_lib` | 1.1.0 | [nasa/sample_app](https://github.com/nasa/sample_app), [nasa/sample_lib](https://github.com/nasa/sample_lib) |
  | `apps/ds` | 2.5.2 | [nasa/DS](https://github.com/nasa/DS) (without `docs/`, `test_and_ground/`, unit tests) |
  | `apps/cfs_lib` | 2.2.0 | [nasa/cfs_lib](https://github.com/nasa/cfs_lib) (NASA Open Source Agreement) |
  | `tools/cFS-GroundSystem` | 2.1.0 | [nasa/cFS-GroundSystem](https://github.com/nasa/cFS-GroundSystem) |
  | `tools/elf2cfetbl` | 3.1.0 | [nasa/elf2cfetbl](https://github.com/nasa/elf2cfetbl) |
  | `tools/tblCRCTool` | 1.1.0 | [nasa/tblCRCTool](https://github.com/nasa/tblCRCTool) |

- **Setup already done.** `Makefile` and `sample_defs/` are already copied to the top level as described in the upstream *Setup* section below, and `OSAL_DEBUG_PERMISSIVE_MODE` is defined so cFS runs without `sudo`.
- **Added for the course:** `create_app.py`, `ground.py`, `apps/sim_imu` and `apps/imu` (see below).
- **cFE, OSAL and PSP are unchanged** from the upstream release.
- **Changes to other upstream code** (see `git log` for details):
  - `cfs_lib`: added `CMakeLists.txt`; fixed a pointer type error that stops the build with GCC 14.
  - `ds`, `sch_lab`: include paths and schedule for the IMU example. DS tables for the example are in `sample_defs/tables/`.

## Quick start

Tested on Ubuntu 24.04 as a normal user, with GCC 13.3 and GCC 14.2 (CMake 3.28).

```sh
sudo apt install build-essential cmake git python3

git clone https://github.com/ryuhg/gistda-cfs-practice.git
cd gistda-cfs-practice
make prep
make
make install

cd build/exe/cpu1
./core-cpu1          # stop with Ctrl+C
```

Startup should reach `CFE_ES_Main entering OPERATIONAL state`, and then the IMU example prints one line per second:

```
IMU: x +0.097  y -0.013  z +1.002
```

Run `core-cpu1` from `build/exe/cpu1`, because it loads the startup script and apps from `./cf`. Most timestamps read `1980-012-...` because the cFE clock has not been set; this is normal. Run only one cFS at a time on a computer (it uses UDP ports 1234 and 1235).

## Ground tool: ground.py

`ground.py` sends commands and shows telemetry. It needs only Python 3 (no extra packages, tested with 3.8 to 3.13 on Linux) and uses nothing Linux-specific, so it can also run on a Windows or macOS host. Open a second terminal in the repository:

```sh
python3 ground.py listen                       # show telemetry (Ctrl+C to stop)
python3 ground.py listen --only IMU EVENT      # only lines containing these words
```

`listen` asks TO_LAB to start sending telemetry, and repeats this every 5 seconds while nothing arrives, so it does not matter whether cFS or `ground.py` starts first. Events are shown with their text, for example `EVENT INFO IMU 3: IMU: NOOP command`.

In a third terminal, send commands with a message ID and a command code:

```sh
python3 ground.py send 0x1882 0                         # SAMPLE_APP NOOP
python3 ground.py send 0x18C0 0                         # IMU NOOP
python3 ground.py send 0x18C0 1                         # IMU RESET_COUNTERS
```

Parameters are added in order with `--u8`, `--u16`, `--u32`, `--i8`, `--i16`, `--i32`, `--f32` and `--string LEN:TEXT`. If cFS runs on another computer, add `--host <its IP address>` to every command.

## IMU example

The IMU app reads a simulated IMU once per second, prints the values and sends them on the software bus, where DS stores them in a file:

```
SCH_LAB --wakeup (1 Hz)--> IMU --SIM_IMU_Read(&x, &y, &z)--> sim_imu
                            |
                            +-- prints one line on the console
                            +-- IMU_MEASUREMENTS_TLM_MID --> DS --> build/exe/cpu1/cf/imuNNNNNNNN.dat
```

- `apps/sim_imu`: a library that stands in for the IMU and its driver. `SIM_IMU_Read(&x, &y, &z)` returns three float values (acceleration in g): z is about 1.0, and x and y swing between -0.1 and +0.1 over 20 seconds, with a little noise. It runs inside cFS and needs no hardware, network port or extra program.
- `apps/imu`: made with `create_app.py imu`, then extended. On each wakeup it calls `SIM_IMU_Read`, prints the values and publishes them.
- `sample_defs/tables/ds_file_tbl.c`, `ds_filter_tbl.c`: DS stores every IMU measurements packet and starts a new file every minute. Numbering restarts at 1 when cFS starts, so files of the previous run are overwritten.

| Message ID | Name | |
|---|---|---|
| `0x18C0` | `IMU_CMD_MID` | commands: 0 NOOP, 1 RESET_COUNTERS |
| `0x18C1` | `IMU_SEND_HK_MID` | housekeeping request (SCH_LAB, 4 s) |
| `0x18C2` | `IMU_WAKEUP_MID` | read the sensor (SCH_LAB, 1 s) |
| `0x08C0` | `IMU_HK_TLM_MID` | housekeeping: command counters |
| `0x08C1` | `IMU_MEASUREMENTS_TLM_MID` | measurements: x, y, z (`imu_measurements_tlm_t`) |

To check what DS stored, read a file with `ground.py`. The lines match the console output of the same samples:

```sh
python3 ground.py dump build/exe/cpu1/cf/imu00000001.dat
```

## Creating a new app: create_app.py

```sh
python3 create_app.py hello_world
```

creates `apps/hello_world/` from the sample_app layout (command pipe, NOOP and RESET_COUNTERS, housekeeping; no tables), with message IDs that do not collide with other apps. It then prints the remaining steps: adding the app to `sample_defs/targets.cmake` and `sample_defs/cpu1_cfe_es_startup.scr`, and optionally to SCH_LAB and TO_LAB for housekeeping.

## Known limitations

- `tools/cFS-GroundSystem` (v2.1.0) needs Python 2 and PyQt4, which are not packaged for Ubuntu 20.04 or later. Use `ground.py` instead.
- If you use `cmdUtil` from cFS-GroundSystem, `--endian=LE` must come before `--cmdcode` on x86 PCs; otherwise every command arrives as NOOP.
- The build prints `-Waddress-of-packed-member` warnings for apps that send packed telemetry (sample_app, ci_lab, imu and apps made with `create_app.py`). They are expected and harmless.
- The ES "restart app" command (`CFE_ES_RESTART_APP_CC`) does not restart apps that use the sample_app main loop (SAMPLE_APP, the lab apps, IMU and apps made with `create_app.py`): ES deletes the app and logs `Exit Application ... on Error Completed`. This is a bug in cFE 6.7, fixed in later cFE releases, and is not patched here.

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
