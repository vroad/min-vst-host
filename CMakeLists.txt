cmake_minimum_required(VERSION 3.25.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/vst3sdk/cmake/modules")
include(SMTG_VST3_SDK)

project(min-vst-host)

set(VST_SDK TRUE)
set(SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/libs/vst3sdk")
add_subdirectory(${SDK_ROOT}/pluginterfaces)
add_subdirectory(${SDK_ROOT}/base)
add_subdirectory(${SDK_ROOT}/public.sdk)

smtg_setup_platform_toolset()
smtg_setup_symbol_visibility()

set(min_vst_host_sources
  ${SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.cpp
  ${SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.h
  source/pluginloader.cpp
  source/pluginloader.h
  source/editorhost.cpp
  source/editorhost.h
  source/platform/appinit.h
  source/platform/iapplication.h
  source/platform/iplatform.h
  source/platform/iwindow.h
  source/usediids.cpp
)

if(SMTG_LINUX)
  find_package(X11 REQUIRED)
  include_directories(${X11_INCLUDE_DIR})
  set(min_vst_host_sources
    ${min_vst_host_sources}
    source/platform/linux/platform.cpp
    source/platform/linux/runloop.h
    source/platform/linux/runloop.cpp
    source/platform/linux/window.h
    source/platform/linux/window.cpp
    ${SDK_ROOT}/public.sdk/source/vst/hosting/module_linux.cpp
  )
  set(min_vst_host_PLATFORM_LIBS
    ${X11_LIBRARIES}
  )
else()
  message(FATAL_ERROR "This project supports only Linux platforms.")
endif()

add_executable(min-vst-host ${min_vst_host_sources})
target_compile_features(min-vst-host
  PUBLIC
    cxx_std_17
)
target_link_libraries(min-vst-host
  PRIVATE
    sdk_hosting
    ${min_vst_host_PLATFORM_LIBS}
)

target_include_directories(min-vst-host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
