cmake_minimum_required(VERSION 3.10)
project(ConnectTool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# ImGui sources (imgui doesn't have CMakeLists.txt by default)
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/imgui")
file(GLOB IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC 
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# Add nanoid_cpp
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(nanoid_cpp)

# Steamworks SDK path
set(STEAMWORKS_DIR "${CMAKE_SOURCE_DIR}/sdk")

# Platform-specific Steamworks library
if(WIN32)
    set(STEAM_LIB "${STEAMWORKS_DIR}/redistributable_bin/win64/steam_api64.lib")
elseif(APPLE)
    set(STEAM_LIB "${STEAMWORKS_DIR}/redistributable_bin/osx/libsteam_api.dylib")
else()
    set(STEAM_LIB "${STEAMWORKS_DIR}/redistributable_bin/linux64/libsteam_api.so")
endif()

# TUN module sources (platform-specific)
set(TUN_SOURCES
    tun/tun_factory.cpp
)

if(UNIX AND NOT APPLE)
    # Linux
    list(APPEND TUN_SOURCES tun/tun_linux.cpp)
    message(STATUS "Building TUN module for Linux")
elseif(APPLE)
    # macOS
    list(APPEND TUN_SOURCES tun/tun_macos.cpp)
    message(STATUS "Building TUN module for macOS")
elseif(WIN32)
    # Windows
    list(APPEND TUN_SOURCES tun/tun_windows.cpp)
    message(STATUS "Building TUN module for Windows (Wintun)")
endif()

# Add Online Game Tool executable
add_executable(OnlineGameTool 
    ConnectTool/online_game_tool.cpp 
    ConnectTool/steam/steam_message_handler.cpp 
    ConnectTool/steam/steam_networking_manager.cpp 
    ConnectTool/steam/steam_room_manager.cpp 
    ConnectTool/steam/steam_utils.cpp 
    ConnectTool/steam/steam_vpn_bridge.cpp
    ConnectTool/net/tcp_server.cpp 
    ConnectTool/net/multiplex_manager.cpp
    ${TUN_SOURCES}
)

# Set compile options for UTF-8 support
if(MSVC)
    target_compile_options(OnlineGameTool PRIVATE /utf-8)
endif()

# Include directories
target_include_directories(OnlineGameTool PRIVATE
    ${CMAKE_SOURCE_DIR}/ConnectTool
    ${CMAKE_SOURCE_DIR}/ConnectTool/net
    ${CMAKE_SOURCE_DIR}/ConnectTool/steam
    ${CMAKE_SOURCE_DIR}/nanoid_cpp/inc
    ${CMAKE_SOURCE_DIR}/tun
    ${STEAMWORKS_DIR}/public/steam
)

# Include Wintun (Windows only)
if(WIN32)
    target_include_directories(OnlineGameTool PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/wintun/api
    )
endif()

# Link libraries - platform specific
if(WIN32)
    target_link_libraries(OnlineGameTool PRIVATE 
        imgui 
        nanoid 
        ${STEAM_LIB}
        Boost::system 
        ws2_32
        Threads::Threads
    )
elseif(APPLE)
    target_link_libraries(OnlineGameTool PRIVATE 
        imgui 
        nanoid 
        ${STEAM_LIB}
        Boost::system
        Threads::Threads
    )
else()
    target_link_libraries(OnlineGameTool PRIVATE 
        imgui 
        nanoid 
        ${STEAM_LIB}
        Boost::system
        Threads::Threads
    )
endif()

# Copy Steam API DLL/SO/DYLIB to output directory
if(WIN32)
    add_custom_command(TARGET OnlineGameTool POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${STEAMWORKS_DIR}/redistributable_bin/win64/steam_api64.dll"
        $<TARGET_FILE_DIR:OnlineGameTool>)
elseif(APPLE)
    add_custom_command(TARGET OnlineGameTool POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${STEAMWORKS_DIR}/redistributable_bin/osx/libsteam_api.dylib"
        $<TARGET_FILE_DIR:OnlineGameTool>)
else()
    add_custom_command(TARGET OnlineGameTool POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${STEAMWORKS_DIR}/redistributable_bin/linux64/libsteam_api.so"
        $<TARGET_FILE_DIR:OnlineGameTool>)
endif()

# Add TUN example executable (optional)
option(BUILD_TUN_EXAMPLE "Build TUN example program" OFF)
if(BUILD_TUN_EXAMPLE)
    add_executable(TunExample tun/example_tun.cpp ${TUN_SOURCES})
    
    # Platform-specific libraries for TUN example
    if(WIN32)
        target_link_libraries(TunExample PRIVATE iphlpapi ws2_32 ole32)
    elseif(UNIX)
        target_link_libraries(TunExample PRIVATE pthread)
    endif()
    
    message(STATUS "TUN example will be built")
endif()