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
    target_compile_options(project_warnings INTERFACE /W4 /w44062 /permissive- /WX)
else()
    target_compile_options(project_warnings INTERFACE
        -Wall -Wextra -Wpedantic -Werror
        -Wshadow                # a local name hides another one
        -Wconversion            # implicit conversion that can lose data
        -Wsign-conversion       # implicit signed <-> unsigned conversion
        -Wold-style-cast        # C-style (T)x cast instead of static_cast
        -Wnon-virtual-dtor      # class with virtual functions but no virtual destructor
        -Woverloaded-virtual    # a function hides a base class virtual instead of overriding it
        -Wnull-dereference
        -Wdouble-promotion)     # float silently widened to double
endif()

add_library(courier_core STATIC
    src/LogisticsException.cpp
    src/ValidationException.cpp
    src/InvalidStatusChangeException.cpp
    src/PackageNotFoundException.cpp
    src/OperationNotAllowedException.cpp
    src/AssigneeNotFoundException.cpp
    src/StorageException.cpp
    src/Dimensions.cpp
    src/ServiceType.cpp
    src/PackageStatus.cpp
    src/TrackingId.cpp
    src/Validation.cpp
    src/Money.cpp
    src/Package.cpp
    src/ParcelDetails.cpp
    src/StandardPackage.cpp
    src/ExpressPackage.cpp
    src/FragilePackage.cpp
    src/PackageFactory.cpp
    src/ShipmentCommand.cpp
    src/RerouteCommand.cpp
    src/ModifyCommand.cpp
    src/AssignCommand.cpp
    src/CommandHistory.cpp
    src/OperationType.cpp
    src/OperationRecord.cpp
    src/AssigneeKind.cpp
    src/Assignee.cpp
    src/Courier.cpp
    src/Office.cpp
    src/StatusObserver.cpp
    src/LogisticsManager.cpp
    src/DispatchModule.cpp)

target_include_directories(courier_core PUBLIC include)
target_link_libraries(courier_core PRIVATE project_warnings)

add_executable(courier src/main.cpp)

target_link_libraries(courier PRIVATE courier_core project_warnings)

enable_testing()
add_subdirectory(tests)
