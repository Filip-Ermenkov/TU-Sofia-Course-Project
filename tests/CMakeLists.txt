include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.16.1
    GIT_SHALLOW TRUE
    SYSTEM)
FetchContent_MakeAvailable(Catch2)

add_executable(courier_tests
    LogisticsExceptionTests.cpp
    ValidationExceptionTests.cpp
    InvalidStatusChangeExceptionTests.cpp
    PackageNotFoundExceptionTests.cpp
    OperationNotAllowedExceptionTests.cpp
    AssigneeNotFoundExceptionTests.cpp
    DimensionsTests.cpp
    ServiceTypeTests.cpp
    PackageStatusTests.cpp
    TrackingIdTests.cpp
    MoneyTests.cpp
    PackageTests.cpp
    ParcelDetailsTests.cpp
    StandardPackageTests.cpp
    ExpressPackageTests.cpp
    FragilePackageTests.cpp
    PackageFactoryTests.cpp
    RerouteCommandTests.cpp
    ModifyCommandTests.cpp
    CommandHistoryTests.cpp
    OperationRecordTests.cpp
    AssigneeKindTests.cpp
    LogisticsManagerTests.cpp
    DispatchModuleTests.cpp)

target_link_libraries(courier_tests PRIVATE courier_core Catch2::Catch2WithMain project_warnings)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(courier_tests)
