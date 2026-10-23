cmake_minimum_required(VERSION 3.25)

project(CourierSystem
    VERSION 0.1.0
    DESCRIPTION "Courier shipment management system"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(project_warnings INTERFACE)
if(MSVC)
    target_compile_options(project_warnings INTERFACE /W4 /permissive- /WX)
else()
    target_compile_options(project_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(courier src/main.cpp)

target_link_libraries(courier PRIVATE project_warnings)
