target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    comm/Src/cmd_parser.c
    comm/Src/uart_comm.c
    controller/Src/controller.c
    motors/Src/encoder.c
    motors/Src/motor_driver.c
    motors/Src/motor_pid.c
    sensors/Src/battery.c
    sensors/Src/tof_vl53l1x.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/comm/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/controller/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/motors/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/sensors/Inc
)
