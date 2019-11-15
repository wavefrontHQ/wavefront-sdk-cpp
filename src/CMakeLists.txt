add_library(wavefront-sdk SHARED
        common/SocketException.cpp
        common/Socket.cpp
        proxy/ProxyConnectionHandler.cpp
        proxy/WavefrontProxyClient.cpp
        direct_ingestion/DirectIngesterService.cpp
        direct_ingestion/WavefrontDirectIngestionClient.cpp
        # cpr
        $<TARGET_OBJECTS:cpr>)

target_link_libraries(wavefront-sdk PUBLIC Boost::iostreams ${CMAKE_THREAD_LIBS_INIT} ${CURL_LIBRARIES})
if (UNIX AND NOT APPLE)
    target_link_libraries(wavefront-sdk PUBLIC rt)
endif ()
set_target_properties(wavefront-sdk PROPERTIES VERSION ${PROJECT_VERSION})
# --[ Main
add_executable(main ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(main PUBLIC wavefront-sdk)
set_target_properties(main PROPERTIES OUTPUT_NAME wavefront-sdk)

include(ExternalProject)
set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

ExternalProject_Add(googletest
GIT_REPOSITORY https://github.com/google/googletest
GIT_TAG           master
CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION}
)

include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)
enable_testing()

add_executable(HistogramTests ${PROJECT_SOURCE_DIR}/src/test/HistogramTest.cpp)
add_dependencies(HistogramTests googletest)
target_link_libraries(HistogramTests gtest gtest_main)
add_test(Test HistogramTests)


install(TARGETS wavefront-sdk EXPORT wavefront-sdk-targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/cpr/include/cpr DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


