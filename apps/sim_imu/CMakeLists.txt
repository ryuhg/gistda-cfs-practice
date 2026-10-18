cmake_minimum_required(VERSION 2.6.4)
project(CFS_SIM_IMU C)

include_directories(fsw/public_inc)

aux_source_directory(fsw/src LIB_SRC_FILES)

# Create the library module
add_cfe_app(sim_imu ${LIB_SRC_FILES})
