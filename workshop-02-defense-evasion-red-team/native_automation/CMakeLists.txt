cmake_minimum_required(VERSION 3.16)
project(evasion_native_automation CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libcurl REQUIRED IMPORTED_TARGET libcurl)

add_executable(evasion_native
  src/main.cpp
  src/config.cpp
  src/http_client.cpp
  src/util.cpp
  src/evasion_steps.cpp
)

target_include_directories(evasion_native PRIVATE include)
target_link_libraries(evasion_native PRIVATE PkgConfig::libcurl)

install(TARGETS evasion_native RUNTIME DESTINATION bin)
