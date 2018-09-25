cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 11)

project(wavefront-sdk)
enable_testing()
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake)

option(ENABLE_COMPRESSION "Enable gzip compression" ON)
option(ENABLE_TESTING "Build tests" ON)

# ---[ Dependency:: find pthread
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads)

# ---[ Dependency:: find Boost
find_package(Boost 1.52 REQUIRED COMPONENTS iostreams filesystem system log)
include_directories(${Boost_INCLUDE_DIR})

# ---[ Dependency:: find cpr/curl
find_package(cpr CONFIG REQUIRED PATHS ${PROJECT_SOURCE_DIR}/cmake)

# suppress warnings
if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif ()

# ---[ Includes
set(WAVEFRONT_SDK_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/include)
include_directories(${WAVEFRONT_SDK_INCLUDE_DIR})
include_directories(${CPR_INCLUDE_DIR} ${CURL_INCLUDE_DIRS})
include_directories(BEFORE src) # This is needed for gtest.

# ---[ Subdirectories
add_subdirectory(src)

# ---[ Congif and Install
set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/wavefront-sdk-config.cmake.in wavefront-sdk-config.cmake
        INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
        )
install(EXPORT wavefront-sdk-targets DESTINATION ${CMAKECONFIG_INSTALL_DIR} NAMESPACE wavefront-sdk::)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/wavefront-sdk-config.cmake" DESTINATION ${CMAKECONFIG_INSTALL_DIR})
