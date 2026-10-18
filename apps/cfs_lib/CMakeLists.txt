cmake_minimum_required(VERSION 2.6.4)
project(CFS_LIB C)

include_directories(fsw/public_inc)

aux_source_directory(fsw/src LIB_SRC_FILES)

# Create the library module
add_cfe_app(cfs_lib ${LIB_SRC_FILES})
