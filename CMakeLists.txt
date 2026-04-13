cmake_minimum_required(VERSION 3.21)

include(cmake/project.cmake)

project(decor_test LANGUAGES CXX)

include(cmake/utils.cmake)
include(cmake/windows_sdk_tools.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(WINDOWS_APP_SDK_FOUNDATION_VERSION "1.8.260222000" CACHE STRING
    "Version of Microsoft.WindowsAppSDK.Foundation to install from NuGet")
set(WINDOWS_APP_SDK_INTERACTIVE_VERSION "1.8.260125001" CACHE STRING
    "Version of Microsoft.WindowsAppSDK.InteractiveExperiences to install from NuGet")

string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\." windows_app_sdk_version_match
    "${WINDOWS_APP_SDK_FOUNDATION_VERSION}")
if(NOT windows_app_sdk_version_match)
    message(FATAL_ERROR
        "Failed to parse Windows App SDK version '${WINDOWS_APP_SDK_FOUNDATION_VERSION}'")
endif()
set(windows_app_sdk_version_major "${CMAKE_MATCH_1}")
set(windows_app_sdk_version_minor "${CMAKE_MATCH_2}")

set(WINDOWS_APP_SDK_ROOT_DIR "${CMAKE_BINARY_DIR}/windows-app-sdk")
set(WINDOWS_APP_SDK_PACKAGES_DIR "${WINDOWS_APP_SDK_ROOT_DIR}/packages")
set(WINDOWS_APP_SDK_GENERATED_DIR "${WINDOWS_APP_SDK_ROOT_DIR}/include-gen")

acbt_nuget_install_package(
    PACKAGE_NAME "Microsoft.WindowsAppSDK.Foundation"
    PACKAGE_VERSION "${WINDOWS_APP_SDK_FOUNDATION_VERSION}"
    OUTPUT_DIRECTORY "${WINDOWS_APP_SDK_PACKAGES_DIR}")
acbt_nuget_install_package(
    PACKAGE_NAME "Microsoft.WindowsAppSDK.InteractiveExperiences"
    PACKAGE_VERSION "${WINDOWS_APP_SDK_INTERACTIVE_VERSION}"
    OUTPUT_DIRECTORY "${WINDOWS_APP_SDK_PACKAGES_DIR}")

set(WINDOWS_APP_SDK_FOUNDATION_DIR
    "${WINDOWS_APP_SDK_PACKAGES_DIR}/Microsoft.WindowsAppSDK.Foundation.${WINDOWS_APP_SDK_FOUNDATION_VERSION}")
set(WINDOWS_APP_SDK_INTERACTIVE_DIR
    "${WINDOWS_APP_SDK_PACKAGES_DIR}/Microsoft.WindowsAppSDK.InteractiveExperiences.${WINDOWS_APP_SDK_INTERACTIVE_VERSION}")

# Support both NuGet extraction layouts:
# 1) <PackageName>.<Version> (expected)
# 2) <Version> (legacy cache copy layout)
set(WINDOWS_APP_SDK_FOUNDATION_DIR_LEGACY
    "${WINDOWS_APP_SDK_PACKAGES_DIR}/${WINDOWS_APP_SDK_FOUNDATION_VERSION}")
set(WINDOWS_APP_SDK_INTERACTIVE_DIR_LEGACY
    "${WINDOWS_APP_SDK_PACKAGES_DIR}/${WINDOWS_APP_SDK_INTERACTIVE_VERSION}")

if(NOT EXISTS "${WINDOWS_APP_SDK_FOUNDATION_DIR}" AND EXISTS "${WINDOWS_APP_SDK_FOUNDATION_DIR_LEGACY}")
    set(WINDOWS_APP_SDK_FOUNDATION_DIR "${WINDOWS_APP_SDK_FOUNDATION_DIR_LEGACY}")
endif()

if(NOT EXISTS "${WINDOWS_APP_SDK_INTERACTIVE_DIR}" AND EXISTS "${WINDOWS_APP_SDK_INTERACTIVE_DIR_LEGACY}")
    set(WINDOWS_APP_SDK_INTERACTIVE_DIR "${WINDOWS_APP_SDK_INTERACTIVE_DIR_LEGACY}")
endif()

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

acbt_add_cppwinrt_target(
    TARGET_NAME generate_windows_app_sdk_headers
    OUTPUT_DIR "${WINDOWS_APP_SDK_GENERATED_DIR}"
    INPUT_WINMD "${WINDOWS_APP_SDK_UI_WINMD}"
    REF_WINMDS
        "${WINDOWS_APP_SDK_GRAPHICS_WINMD}"
        "${WINDOWS_APP_SDK_FOUNDATION_WINMD}")

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

target_compile_definitions(decor_test PRIVATE
    WINDOWS_APP_SDK_VERSION_MAJOR=${windows_app_sdk_version_major}
    WINDOWS_APP_SDK_VERSION_MINOR=${windows_app_sdk_version_minor})

set(WINDOWS_APP_SDK_BOOTSTRAP_DLL_OUTPUT
    "${CMAKE_CURRENT_BINARY_DIR}/Microsoft.WindowsAppRuntime.Bootstrap.dll")

add_custom_command(
    TARGET decor_test POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${WINDOWS_APP_SDK_BOOTSTRAP_DLL}"
            "$<TARGET_FILE_DIR:decor_test>/Microsoft.WindowsAppRuntime.Bootstrap.dll"
    COMMENT "Copying Windows App Runtime bootstrap DLL next to the executable"
    VERBATIM
)

install(TARGETS decor_test RUNTIME DESTINATION .)

if(EXISTS "${WINDOWS_APP_SDK_BOOTSTRAP_DLL}")
    install(FILES "${WINDOWS_APP_SDK_BOOTSTRAP_DLL}" DESTINATION .)
endif()
