cmake_minimum_required(VERSION 3.16)
project(texi_ir_platform C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(driver)
add_subdirectory(logic)
