cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED IME_WINDOWS_SERVICE_SUPERBUILD)
    set(IME_WINDOWS_SERVICE_SUPERBUILD ON CACHE BOOL "Build ime-core before the Windows service")
endif()

if(IME_WINDOWS_SERVICE_SUPERBUILD)
    # Each child has its own configure step and manifest. Avoid creating an
    # unused third vcpkg tree for this orchestration-only configure.
    set(VCPKG_MANIFEST_INSTALL OFF CACHE BOOL "Install superbuild dependencies" FORCE)

    project(ime_windows_service VERSION 0.1.0 LANGUAGES NONE)

    include(ExternalProject)

    if(NOT WIN32)
        message(FATAL_ERROR "ime-windows-service is Windows-only")
    endif()
    if(NOT CMAKE_TOOLCHAIN_FILE)
        message(FATAL_ERROR "ime-windows-service requires CMAKE_TOOLCHAIN_FILE")
    endif()

    set(IME_WINDOWS_SERVICE_CONFIGURATION
        "Release"
        CACHE STRING
        "Configuration used by child builds"
    )
    set_property(
        CACHE IME_WINDOWS_SERVICE_CONFIGURATION
        PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel
    )
    set(IME_WINDOWS_SERVICE_VCPKG_TRIPLET
        "x64-windows"
        CACHE STRING
        "vcpkg triplet used by ime-core and the Windows service"
    )
    set(IME_WINDOWS_SERVICE_CORE_SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/ime-core"
        CACHE PATH
        "ime-core submodule source directory"
    )

    if(NOT EXISTS "${IME_WINDOWS_SERVICE_CORE_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "ime-core was not found at ${IME_WINDOWS_SERVICE_CORE_SOURCE_DIR}. "
            "Initialize the ime-core submodule."
        )
    endif()

    set(_ime_core_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/ime-core-build")
    set(_ime_core_install_dir "${CMAKE_CURRENT_BINARY_DIR}/ime-core-install")
    set(_ime_service_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/service-build")

    ExternalProject_Add(ime-core-build
        SOURCE_DIR "${IME_WINDOWS_SERVICE_CORE_SOURCE_DIR}"
        BINARY_DIR "${_ime_core_binary_dir}"
        INSTALL_DIR "${_ime_core_install_dir}"
        CMAKE_ARGS
            "-DCMAKE_BUILD_TYPE:STRING=${IME_WINDOWS_SERVICE_CONFIGURATION}"
            "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>"
            "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}"
            "-DVCPKG_MANIFEST_INSTALL:BOOL=ON"
            "-DVCPKG_TARGET_TRIPLET:STRING=${IME_WINDOWS_SERVICE_VCPKG_TRIPLET}"
        BUILD_COMMAND
            "${CMAKE_COMMAND}"
                --build "<BINARY_DIR>"
                --config "${IME_WINDOWS_SERVICE_CONFIGURATION}"
        INSTALL_COMMAND
            "${CMAKE_COMMAND}"
                --install "<BINARY_DIR>"
                --config "${IME_WINDOWS_SERVICE_CONFIGURATION}"
        BUILD_ALWAYS TRUE
        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD TRUE
        USES_TERMINAL_INSTALL TRUE
    )

    ExternalProject_Add(ime-windows-service-build
        DEPENDS ime-core-build
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        BINARY_DIR "${_ime_service_binary_dir}"
        INSTALL_DIR "${CMAKE_INSTALL_PREFIX}"
        CMAKE_ARGS
            "-DIME_WINDOWS_SERVICE_SUPERBUILD:BOOL=OFF"
            "-DCMAKE_BUILD_TYPE:STRING=${IME_WINDOWS_SERVICE_CONFIGURATION}"
            "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>"
            "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}"
            "-DVCPKG_MANIFEST_DIR:PATH=${CMAKE_CURRENT_SOURCE_DIR}"
            "-DVCPKG_MANIFEST_INSTALL:BOOL=ON"
            "-DVCPKG_TARGET_TRIPLET:STRING=${IME_WINDOWS_SERVICE_VCPKG_TRIPLET}"
            "-Dime-core_DIR:PATH=${_ime_core_install_dir}/lib/cmake/ime-core"
            "-DIME_CORE_VCPKG_PREFIX:PATH=${_ime_core_binary_dir}/vcpkg_installed/${IME_WINDOWS_SERVICE_VCPKG_TRIPLET}"
        BUILD_COMMAND
            "${CMAKE_COMMAND}"
                --build "<BINARY_DIR>"
                --config "${IME_WINDOWS_SERVICE_CONFIGURATION}"
        INSTALL_COMMAND
            "${CMAKE_COMMAND}"
                --install "<BINARY_DIR>"
                --config "${IME_WINDOWS_SERVICE_CONFIGURATION}"
        BUILD_ALWAYS TRUE
        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD TRUE
        USES_TERMINAL_INSTALL TRUE
    )

    add_custom_target(ime-windows-service-artifacts ALL
        DEPENDS ime-windows-service-build
    )

    install(DIRECTORY
        "${_ime_core_binary_dir}/vcpkg_installed/${IME_WINDOWS_SERVICE_VCPKG_TRIPLET}/bin/"
        DESTINATION bin
        FILES_MATCHING PATTERN "*.dll"
    )
    install(DIRECTORY "${_ime_core_install_dir}/share/ime-core/tables/"
        DESTINATION tables
    )
    install(FILES LICENSE DESTINATION licenses COMPONENT Runtime)
    install(FILES "${IME_WINDOWS_SERVICE_CORE_SOURCE_DIR}/LICENSE"
        DESTINATION licenses
        RENAME ime-core-LICENSE.txt
        COMPONENT Runtime
    )

    return()
endif()

project(ime_windows_service VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "ime-windows-service is Windows-only")
endif()

set(IME_CORE_VCPKG_PREFIX
    ""
    CACHE PATH
    "vcpkg installed prefix used by the separately configured ime-core project"
)
if(NOT IME_CORE_VCPKG_PREFIX)
    message(FATAL_ERROR "IME_CORE_VCPKG_PREFIX is required")
endif()
list(PREPEND CMAKE_PREFIX_PATH "${IME_CORE_VCPKG_PREFIX}")

find_package(asio CONFIG REQUIRED)
find_package(ime-core CONFIG REQUIRED)

add_executable(llavon-ime-service
    src/main.cpp
    src/platform/windows_named_pipe_server.hpp
)

target_compile_features(llavon-ime-service PRIVATE cxx_std_23)
set_target_properties(llavon-ime-service PROPERTIES
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
target_compile_definitions(llavon-ime-service PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
)
target_compile_options(llavon-ime-service PRIVATE
    /W4
    /WX
    /utf-8
    /await:strict
)
target_include_directories(llavon-ime-service PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(llavon-ime-service PRIVATE
    ime-core::ime-core
    asio::asio
    advapi32
    ole32
    oleaut32
    ws2_32
)

install(TARGETS llavon-ime-service RUNTIME DESTINATION bin)
