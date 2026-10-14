# cFS Hands-on Practice (GISTDA training fork)

This repository is a training fork of the NASA [cFS bundle](https://github.com/nasa/cFS) at release **Aquila (cFS 6.7.0a)**. It is used for the hands-on exercises of a cFS introductory course. Aquila was chosen over later releases (Caelum, Draco) because its configuration is simpler for first-time users.

Apart from this section, the repository is the same as the upstream tag `v6.7.0a`. The components (cFE, OSAL, PSP, the lab apps and the tools) are git submodules that point to the NASA repositories, and the build files (`Makefile`, `sample_defs/`) are not included. Setting them up is part of the exercise: follow the **Setup** and **Build and Run** sections of the upstream README below.

## Notes for this course

- Tested on Ubuntu 24.04 (GCC 13.3, CMake 3.28) as a normal user. Required packages: `sudo apt install build-essential cmake git`.
- After `git clone`, the directories `cfe/`, `osal/`, `psp/`, `apps/*` and `tools/*` are empty until you run `git submodule init` and `git submodule update`.
- If you run cFS as a normal user (not root), do the `OSAL_DEBUG_PERMISSIVE_MODE` step of the Setup section. Without it, `core-cpu1` exits right after starting with `CFE_PSP: OS_API_Init() failure`.
- The build prints a few `-Waddress-of-packed-member` warnings in `sample_app` and `ci_lab`. These are expected and harmless.
- Most timestamps on the console read `1980-012-...` because the cFE clock has not been set. This is normal.

## Sending commands

`tools/cFS-GroundSystem` (v2.1.0) needs **Python 2 and PyQt4**. PyQt4 is not packaged for Ubuntu 20.04 or later, so the GUI ground system will not start there. Use `cmdUtil` instead:

```sh
cd tools/cFS-GroundSystem/Subsystems/cmdUtil
make
```

With `core-cpu1` running, open a second terminal and run:

```sh
# Ask TO_LAB to send telemetry to UDP port 1235 on this machine
./cmdUtil --host=127.0.0.1 --pktid=0x1880 --endian=LE --cmdcode=6 --string="17:127.0.0.1"

# Send a NOOP to SAMPLE_APP. The cFS console prints "SAMPLE: NOOP command"
./cmdUtil --host=127.0.0.1 --pktid=0x1882 --endian=LE --cmdcode=0
```

On x86 PCs, `--endian=LE` is required and must come **before** `--cmdcode`. Without it the command code is written to the wrong byte, so cFS reads it as 0 (NOOP). The bundled `to-enable-tlm.sh` script does not pass this option, so on x86 it sends a NOOP instead of enabling telemetry.

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
