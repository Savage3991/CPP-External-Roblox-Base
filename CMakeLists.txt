cmake_minimum_required(VERSION 3.28)
project(Savagesploit)

set(CMAKE_CXX_STANDARD 23)

add_executable(Savagesploit main.cpp

        Roblox/offsets.hpp

        Roblox/DataModel/RBXDataModel.cpp
        Roblox/DataModel/RBXDataModel.hpp

        Roblox/RBXInstance.cpp
        Roblox/RBXInstance.hpp

        utils/base64.hpp
        utils/json.hpp
        utils/utils.hpp

        Roblox/RBXBytecode.cpp
        Roblox/RBXBytecode.hpp
        UserMode/unhook.cpp
        UserMode/Memory.cpp
        UserMode/Memory.hpp
        UserMode/unhook.cpp
        UserMode/unhook.hpp

        Mapper/driver.h
        Mapper/intel_driver.cpp
        Mapper/intel_driver.hpp
        Mapper/intel_driver_resource.hpp
        Mapper/kdmapper.cpp
        Mapper/kdmapper.hpp
        Mapper/nt.hpp
        Mapper/portable_executable.cpp
        Mapper/portable_executable.hpp
        Mapper/service.cpp
        Mapper/service.hpp
        Mapper/utils.cpp
        Mapper/utils.hpp
        Driver/Driver.cpp
        Driver/Driver.h


        utils/NTDLL_FAST.cpp
        utils/NTDLL_FAST.hpp
        utils/NTDLL_FAST.cpp
)

target_include_directories(Savagesploit PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(Savagesploit PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies")
# Dependencies

find_package(zstd CONFIG REQUIRED)
find_package(xxhash CONFIG REQUIRED)

# Luau
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Luau")

SET_TARGET_PROPERTIES(Savagesploit PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:CONSOLE")

target_link_libraries(${PROJECT_NAME}
        # Luau
        Luau.Compiler
        Luau.Ast
        Luau.VM
        Luau.VM.Internals

        zstd::libzstd
        xxHash::xxhash
)