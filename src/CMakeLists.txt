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

install(TARGETS wavefront-sdk EXPORT wavefront-sdk-targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/cpr/include/cpr DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


