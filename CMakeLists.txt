cmake_minimum_required(VERSION 3.20)
project(ColorDock VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt 경로 (aqtinstall 기본 설치 위치)
if(NOT DEFINED CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/mingw_64")
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core Widgets Network Concurrent
)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/views/EffectsView.cpp
    src/views/DevicesView.cpp
    src/views/KeyboardView.cpp
    src/views/ScenesView.cpp
    src/views/AudioView.cpp
    src/views/MonitoringView.cpp
    src/views/ProfilesView.cpp
    src/views/SettingsView.cpp
    src/views/LayoutView.cpp
    src/views/ScheduleView.cpp
    src/views/SdkView.cpp
    src/views/HidScannerDialog.cpp
    src/hardware/DeviceManager.cpp
    src/hardware/OpenRGBClient.cpp
    src/utils/AppSettings.cpp
    src/utils/UpdateChecker.cpp
    src/utils/WasapiCapture.cpp
)

set(HEADERS
    src/MainWindow.h
    src/views/EffectsView.h
    src/views/DevicesView.h
    src/views/KeyboardView.h
    src/views/ScenesView.h
    src/views/AudioView.h
    src/views/MonitoringView.h
    src/views/ProfilesView.h
    src/views/SettingsView.h
    src/views/LayoutView.h
    src/views/ScheduleView.h
    src/views/SdkView.h
    src/views/HidScannerDialog.h
    src/hardware/DeviceManager.h
    src/hardware/OpenRGBClient.h
    src/utils/AppSettings.h
    src/utils/UpdateChecker.h
    src/utils/WasapiCapture.h
)

add_executable(ColorDock WIN32 ${SOURCES} ${HEADERS})

target_link_libraries(ColorDock PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    ole32
    uuid
)

# Windows: 앱 아이콘, 관리자 권한 매니페스트
if(WIN32)
    set_target_properties(ColorDock PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_sources(ColorDock PRIVATE resources/app.rc)
endif()

# 배포용: Qt DLL 자동 복사
if(WIN32)
    add_custom_command(TARGET ColorDock POST_BUILD
        COMMAND ${CMAKE_PREFIX_PATH}/bin/windeployqt.exe
            --no-translations
            $<TARGET_FILE:ColorDock>
        COMMENT "Deploying Qt DLLs..."
    )
endif()
