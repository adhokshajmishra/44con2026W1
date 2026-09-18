cmake_minimum_required(VERSION 3.16)
project(teamtnt_native_automation CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libcurl REQUIRED IMPORTED_TARGET libcurl)

include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_SUBDIR json
)
FetchContent_GetProperties(nlohmann_json)
if(NOT nlohmann_json_POPULATED)
  FetchContent_Populate(nlohmann_json)
endif()

# Tar layout varies: .../include or .../json/include depending on CMake version.
set(NLOHMANN_JSON_INCLUDE "")
foreach(_candidate
    "${nlohmann_json_SOURCE_DIR}/include"
    "${nlohmann_json_SOURCE_DIR}/json/include"
    "${nlohmann_json_SOURCE_DIR}/single_include")
  if(EXISTS "${_candidate}/nlohmann/json.hpp")
    set(NLOHMANN_JSON_INCLUDE "${_candidate}")
    break()
  endif()
endforeach()
if(NOT NLOHMANN_JSON_INCLUDE)
  message(FATAL_ERROR "nlohmann/json.hpp not found under ${nlohmann_json_SOURCE_DIR}")
endif()

add_library(nlohmann_json_iface INTERFACE)
target_include_directories(nlohmann_json_iface SYSTEM INTERFACE ${NLOHMANN_JSON_INCLUDE})

add_executable(teamtnt_native
  src/main.cpp
  src/config.cpp
  src/http_client.cpp
  src/docker_client.cpp
  src/net_scanner.cpp
  src/util.cpp
  src/attack_steps.cpp
)

target_include_directories(teamtnt_native PRIVATE include)
target_link_libraries(teamtnt_native PRIVATE PkgConfig::libcurl nlohmann_json_iface)

install(TARGETS teamtnt_native RUNTIME DESTINATION bin)
