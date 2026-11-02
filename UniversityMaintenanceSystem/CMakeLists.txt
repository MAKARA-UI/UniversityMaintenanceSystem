cmake_minimum_required(VERSION 3.22)
project(UniversityMaintenanceSystem VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Sql)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

set(CMAKE_AUTORCC ON)

add_executable(university_maintenance_system
    src/main.cpp
    src/domain/FaultReport.h
    src/domain/FaultRules.h
    src/domain/FaultRules.cpp
    src/data/FaultRepository.h
    src/data/FaultRepository.cpp
    src/data/AccountRepository.h
    src/data/AccountRepository.cpp
    src/data/WorkOrderRepository.h
    src/data/WorkOrderRepository.cpp
    src/data/LocationRepository.h
    src/data/LocationRepository.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/UiHelpers.h
    src/ui/dashboards/AdministratorDashboard.cpp
    src/ui/dashboards/FacilitiesManagerDashboard.cpp
    src/ui/dashboards/FaultReportsDashboard.cpp
    src/ui/dashboards/ReporterDashboard.cpp
    src/ui/dashboards/TechnicianDashboard.cpp
    src/ui/dashboards/WorkOrdersDashboard.cpp
    src/ui/LoginDialog.h
    src/ui/LoginDialog.cpp
    resources/resources.qrc
)

target_include_directories(university_maintenance_system
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(university_maintenance_system
    PRIVATE Qt6::Widgets Qt6::Sql OpenSSL::Crypto
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(university_maintenance_system PRIVATE -Wall -Wextra -Wpedantic)
endif()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/database")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/database/schema.sql"
    "${CMAKE_CURRENT_BINARY_DIR}/database/schema.sql"
    COPYONLY
)
