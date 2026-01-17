add_library(driver_ir STATIC
    src/ir_sensor_gpiod.c
)

target_include_directories(driver_ir PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)

target_include_directories(driver_ir PRIVATE ${GPIOD_INCLUDE_DIRS})
target_link_directories(driver_ir PRIVATE ${GPIOD_LIBRARY_DIRS})
target_link_libraries(driver_ir PRIVATE ${GPIOD_LIBRARIES})

target_compile_options(driver_ir PRIVATE -Wall -Wextra -Wpedantic -O2)
