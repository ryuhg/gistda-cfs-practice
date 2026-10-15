cmake_minimum_required(VERSION 2.6.4)
project(CFE_IMU_APP C)

include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(${sim_i2c_MISSION_DIR}/fsw/public_inc)

aux_source_directory(fsw/src APP_SRC_FILES)

# Create the app module
add_cfe_app(imu_app ${APP_SRC_FILES})
