#################
# Project setup #
#################
cmake_minimum_required(VERSION 3.19)
project(rucksack C)

#####################
# Set CMake options #
#####################
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

#########################
# 3rdparty dependencies #
#########################
add_subdirectory(3rdparty)

###############
# Subprojects #
###############
add_subdirectory(core)
add_subdirectory(packer)
add_subdirectory(unpacker)
