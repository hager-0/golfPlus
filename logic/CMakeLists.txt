add_executable(ir_event
    src/ir_event_main.c
)

target_link_libraries(ir_event PRIVATE driver_ir)
target_compile_options(ir_event PRIVATE -Wall -Wextra -Wpedantic -O2)
