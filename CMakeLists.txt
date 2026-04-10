cmake_minimum_required(VERSION 3.21)

project(decor_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(WINDOWS_APP_SDK_FOUNDATION_VERSION "1.8.260222000" CACHE STRING
    "Version of Microsoft.WindowsAppSDK.Foundation to install from NuGet")
set(WINDOWS_APP_SDK_INTERACTIVE_VERSION "1.8.260125001" CACHE STRING
    "Version of Microsoft.WindowsAppSDK.InteractiveExperiences to install from NuGet")

set(WINDOWS_APP_SDK_PACKAGES_DIR "${CMAKE_BINARY_DIR}/packages")
set(WINDOWS_APP_SDK_GENERATED_DIR "${CMAKE_BINARY_DIR}/include-gen")

find_program(NUGET_EXE NAMES nuget nuget.exe
    HINTS "${CMAKE_SOURCE_DIR}" "$ENV{NUGET_EXE}")
if(NOT NUGET_EXE)
    message(FATAL_ERROR "nuget.exe was not found. Put it in PATH or next to CMakeLists.txt.")
endif()

find_program(CPPWINRT_EXE NAMES cppwinrt cppwinrt.exe)
if(NOT CPPWINRT_EXE)
    message(FATAL_ERROR "cppwinrt.exe was not found. Install C++/WinRT tooling or add it to PATH.")
endif()

file(MAKE_DIRECTORY "${WINDOWS_APP_SDK_PACKAGES_DIR}")

function(install_nuget_package package_name package_version)
    set(package_root "${WINDOWS_APP_SDK_PACKAGES_DIR}/${package_name}.${package_version}")
    if(NOT EXISTS "${package_root}")
        message(STATUS "Installing ${package_name} ${package_version} into ${WINDOWS_APP_SDK_PACKAGES_DIR}")
        execute_process(
            COMMAND "${NUGET_EXE}" install "${package_name}"
                -Version "${package_version}"
                -OutputDirectory "${WINDOWS_APP_SDK_PACKAGES_DIR}"
                -DependencyVersion HighestPatch
                -NonInteractive
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()
endfunction()

install_nuget_package("Microsoft.WindowsAppSDK.Foundation" "${WINDOWS_APP_SDK_FOUNDATION_VERSION}")
install_nuget_package("Microsoft.WindowsAppSDK.InteractiveExperiences" "${WINDOWS_APP_SDK_INTERACTIVE_VERSION}")

set(WINDOWS_APP_SDK_FOUNDATION_DIR
    "${WINDOWS_APP_SDK_PACKAGES_DIR}/Microsoft.WindowsAppSDK.Foundation.${WINDOWS_APP_SDK_FOUNDATION_VERSION}")
set(WINDOWS_APP_SDK_INTERACTIVE_DIR
    "${WINDOWS_APP_SDK_PACKAGES_DIR}/Microsoft.WindowsAppSDK.InteractiveExperiences.${WINDOWS_APP_SDK_INTERACTIVE_VERSION}")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
        set(WINDOWS_APP_SDK_ARCH "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64EC|arm64ec)$")
        set(WINDOWS_APP_SDK_ARCH "arm64ec")
    else()
        set(WINDOWS_APP_SDK_ARCH "x64")
    endif()
else()
    set(WINDOWS_APP_SDK_ARCH "x86")
endif()

file(GLOB WINDOWS_APP_SDK_METADATA_CANDIDATES
    LIST_DIRECTORIES true
    "${WINDOWS_APP_SDK_INTERACTIVE_DIR}/metadata/*")
list(FILTER WINDOWS_APP_SDK_METADATA_CANDIDATES INCLUDE REGEX ".*/[0-9][0-9.]*$")
list(SORT WINDOWS_APP_SDK_METADATA_CANDIDATES COMPARE NATURAL ORDER DESCENDING)
list(LENGTH WINDOWS_APP_SDK_METADATA_CANDIDATES WINDOWS_APP_SDK_METADATA_COUNT)
if(WINDOWS_APP_SDK_METADATA_COUNT EQUAL 0)
    message(FATAL_ERROR
        "No metadata version directories were found under "
        "${WINDOWS_APP_SDK_INTERACTIVE_DIR}/metadata")
endif()
list(GET WINDOWS_APP_SDK_METADATA_CANDIDATES 0 WINDOWS_APP_SDK_METADATA_BASE)

set(WINDOWS_APP_SDK_FOUNDATION_WINMD
    "${WINDOWS_APP_SDK_FOUNDATION_DIR}/metadata/Microsoft.Windows.Foundation.winmd")
set(WINDOWS_APP_SDK_GRAPHICS_WINMD
    "${WINDOWS_APP_SDK_METADATA_BASE}/Microsoft.Graphics.winmd")
set(WINDOWS_APP_SDK_UI_WINMD
    "${WINDOWS_APP_SDK_METADATA_BASE}/Microsoft.UI.winmd")
set(WINDOWS_APP_SDK_BOOTSTRAP_DLL
    "${WINDOWS_APP_SDK_FOUNDATION_DIR}/runtimes/win-${WINDOWS_APP_SDK_ARCH}/native/Microsoft.WindowsAppRuntime.Bootstrap.dll")

foreach(required_path IN ITEMS
    "${WINDOWS_APP_SDK_FOUNDATION_DIR}/include/MddBootstrap.h"
    "${WINDOWS_APP_SDK_INTERACTIVE_DIR}/include/Microsoft.UI.Interop.h"
    "${WINDOWS_APP_SDK_FOUNDATION_DIR}/lib/native/${WINDOWS_APP_SDK_ARCH}/Microsoft.WindowsAppRuntime.lib"
    "${WINDOWS_APP_SDK_FOUNDATION_DIR}/lib/native/${WINDOWS_APP_SDK_ARCH}/Microsoft.WindowsAppRuntime.Bootstrap.lib"
    "${WINDOWS_APP_SDK_FOUNDATION_WINMD}"
    "${WINDOWS_APP_SDK_GRAPHICS_WINMD}"
    "${WINDOWS_APP_SDK_UI_WINMD}")
    if(NOT EXISTS "${required_path}")
        message(FATAL_ERROR "Required Windows App SDK file was not found: ${required_path}")
    endif()
endforeach()

set(WINDOWS_APP_SDK_GENERATED_STAMP "${WINDOWS_APP_SDK_GENERATED_DIR}/.stamp")
add_custom_command(
    OUTPUT "${WINDOWS_APP_SDK_GENERATED_STAMP}"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${WINDOWS_APP_SDK_GENERATED_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${WINDOWS_APP_SDK_GENERATED_DIR}"
    COMMAND "${CPPWINRT_EXE}"
        -out "${WINDOWS_APP_SDK_GENERATED_DIR}"
        -ref sdk
        -ref "${WINDOWS_APP_SDK_GRAPHICS_WINMD}"
        -ref "${WINDOWS_APP_SDK_FOUNDATION_WINMD}"
        -in "${WINDOWS_APP_SDK_UI_WINMD}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${WINDOWS_APP_SDK_GENERATED_STAMP}"
    DEPENDS
        "${WINDOWS_APP_SDK_GRAPHICS_WINMD}"
        "${WINDOWS_APP_SDK_FOUNDATION_WINMD}"
        "${WINDOWS_APP_SDK_UI_WINMD}"
    COMMENT "Generating C++/WinRT headers for Windows App SDK"
    VERBATIM
)

add_custom_target(generate_windows_app_sdk_headers
    DEPENDS "${WINDOWS_APP_SDK_GENERATED_STAMP}")

add_executable(decor_test main.cpp)
add_dependencies(decor_test generate_windows_app_sdk_headers)

target_include_directories(decor_test PRIVATE
    "${WINDOWS_APP_SDK_FOUNDATION_DIR}/include"
    "${WINDOWS_APP_SDK_INTERACTIVE_DIR}/include"
    "${WINDOWS_APP_SDK_GENERATED_DIR}")

target_link_directories(decor_test PRIVATE
    "${WINDOWS_APP_SDK_FOUNDATION_DIR}/lib/native/${WINDOWS_APP_SDK_ARCH}")

target_link_libraries(decor_test PRIVATE
    dwmapi
    gdi32
    ole32
    oleaut32
    runtimeobject
    user32
    Microsoft.WindowsAppRuntime
    Microsoft.WindowsAppRuntime.Bootstrap)

set(WINDOWS_APP_SDK_BOOTSTRAP_DLL_OUTPUT
    "${CMAKE_CURRENT_BINARY_DIR}/Microsoft.WindowsAppRuntime.Bootstrap.dll")

add_custom_command(
    OUTPUT "${WINDOWS_APP_SDK_BOOTSTRAP_DLL_OUTPUT}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${WINDOWS_APP_SDK_BOOTSTRAP_DLL}"
        "${WINDOWS_APP_SDK_BOOTSTRAP_DLL_OUTPUT}"
    DEPENDS
        decor_test
        "${WINDOWS_APP_SDK_BOOTSTRAP_DLL}"
    COMMENT "Copying Windows App Runtime bootstrap DLL next to the executable"
    VERBATIM)

add_custom_target(copy_windows_app_sdk_bootstrap_dll ALL
    DEPENDS "${WINDOWS_APP_SDK_BOOTSTRAP_DLL_OUTPUT}")

install(TARGETS decor_test RUNTIME DESTINATION .)

if(EXISTS "${WINDOWS_APP_SDK_BOOTSTRAP_DLL}")
    install(FILES "${WINDOWS_APP_SDK_BOOTSTRAP_DLL}" DESTINATION .)
endif()
