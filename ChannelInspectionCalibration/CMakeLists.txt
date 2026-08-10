cmake_minimum_required(VERSION 3.16)
project(ChannelInspectionCalibration VERSION 2.0.0 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Release optimization: O2 + omit frame pointer
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Qt6 configuration
find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

set(SOURCES
    main.cpp
    ui/MainWindow.cpp
    ui/ObstacleTableModel.cpp
    ui/WorkLogTableModel.cpp
    ui/AlarmPanelWidget.cpp
    ui/CameraMonitorWidget.cpp
    database/AlarmHistoryModel.cpp
    visualization/ChannelMapWidget.cpp
    services/DeviceStateMachine.cpp
    services/AlarmManager.cpp
    services/SimulationEngine.cpp
    services/CameraService.cpp
    hardware/SimulationSensor.cpp
    hardware/SimulationCamera.cpp
    hardware/CANInterface.cpp
    hardware/SerialInterface.cpp
    hardware/MQTTInterface.cpp
)

set(HEADERS
    config/DeviceConfig.h
    models/DeviceState.h
    models/AlarmEvent.h
    models/WorkLog.h
    ui/MainWindow.h
    ui/ObstacleTableModel.h
    ui/WorkLogTableModel.h
    ui/AlarmPanelWidget.h
    database/AlarmHistoryModel.h
    visualization/ChannelMapWidget.h
    services/DeviceStateMachine.h
    services/AlarmManager.h
    services/SimulationEngine.h
    services/CameraService.h
    hardware/SensorInterface.h
    hardware/SimulationSensor.h
    hardware/CANInterface.h
    hardware/SerialInterface.h
    hardware/MQTTInterface.h
    hardware/CameraInterface.h
    hardware/SimulationCamera.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_sources(${PROJECT_NAME} PRIVATE visualization/resources.qrc)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Core
)

# Windows-specific: suppress console window in release
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
