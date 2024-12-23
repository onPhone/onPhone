cmake_minimum_required(VERSION 3.6)
project(OnPhone)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure output directories
set(OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY})

# Set output directories for all build types
foreach(CONFIG_TYPE IN ITEMS DEBUG RELEASE MINSIZEREL RELWITHDEBINFO)
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE} ${OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE} ${OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE} ${OUTPUT_DIRECTORY})
endforeach()

# Configure SC2 API
set(BUILD_API_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(BUILD_API_TESTS OFF CACHE INTERNAL "" FORCE)
add_subdirectory("cpp-sc2")

# Visual Studio configuration
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT OnPhone)

# Organize SC2 API targets
foreach(TARGET sc2api sc2lib sc2protocol sc2utils)
    set_target_properties(${TARGET} PROPERTIES FOLDER sc2api)
endforeach()

# Organize contrib targets
foreach(TARGET civetweb-c-library libprotobuf libprotobuf-lite libprotoc protoc)
    set_target_properties(${TARGET} PROPERTIES FOLDER sc2api/contrib)
endforeach()

# Platform-specific targets
if(NOT APPLE)
    set_target_properties(sc2renderer PROPERTIES FOLDER sc2api/utilities)
    foreach(TARGET ipv6-parse SDL2main SDL2-static)
        set_target_properties(${TARGET} PROPERTIES FOLDER sc2api/contrib)
    endforeach()
endif()

# Project source files
file(GLOB SOURCES_ONPHONE "${PROJECT_SOURCE_DIR}/src/*.cpp" "${PROJECT_SOURCE_DIR}/src/*.c")
file(GLOB HEADERS_ONPHONE "${PROJECT_SOURCE_DIR}/includes/*.h")

# Include directories
include_directories(SYSTEM
    ${PROJECT_SOURCE_DIR}/includes
    ${PROJECT_SOURCE_DIR}/cpp-sc2/include
    ${PROJECT_SOURCE_DIR}/cpp-sc2/contrib/protobuf/src
    ${PROJECT_BINARY_DIR}/cpp-sc2/generated
)

# Create executable
add_executable(OnPhone ${SOURCES_ONPHONE} ${HEADERS_ONPHONE})
target_link_libraries(OnPhone sc2api sc2lib sc2utils)
