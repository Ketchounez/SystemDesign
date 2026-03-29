cmake_minimum_required(VERSION 3.16)

project(hotel_booking_lab2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(userver COMPONENTS core REQUIRED)

add_library(hotel_booking_core STATIC
    src/domain/service.cpp
)

target_include_directories(hotel_booking_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(hotel_booking_http OBJECT
    src/api/api_utils.cpp
    src/api/middleware/session_auth_middleware.cpp
    src/api/handlers/auth_handlers.cpp
    src/api/handlers/user_handlers.cpp
    src/api/handlers/hotel_handlers.cpp
    src/api/handlers/booking_handlers.cpp
)

target_include_directories(hotel_booking_http
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(hotel_booking_http
    PUBLIC
    hotel_booking_core
    userver::core
)

add_executable(${PROJECT_NAME}
    src/main.cpp
    $<TARGET_OBJECTS:hotel_booking_http>
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    hotel_booking_core
    userver::core
)

add_executable(${PROJECT_NAME}_service_tests
    tests/service_test.cpp
)

target_link_libraries(${PROJECT_NAME}_service_tests
    PRIVATE
    hotel_booking_core
)

enable_testing()
add_test(NAME hotel_booking_lab2_service_tests COMMAND ${PROJECT_NAME}_service_tests)
