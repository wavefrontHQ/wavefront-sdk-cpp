add_library(wavefront-sdk
        common/SocketException.cpp
        include/common/SocketException.h
        common/Socket.cpp
        include/common/Socket.h
        proxy/ProxyConnectionHandler.cpp
        include/proxy/ProxyConnectionHandler.h
        include/proxy/WavefrontProxyClient.h
        include/common/HistogramGranularity.h
        proxy/WavefrontProxyClient.cpp
        include/common/Serializer.h
        include/direct_ingestion/DirectIngesterService.h
        direct_ingestion/DirectIngesterService.cpp
        direct_ingestion/WavefrontDirectIngestionClient.cpp
        include/direct_ingestion/WavefrontDirectIngestionClient.h
        include/common/WavefrontSender.h
        include/common/Constants.h
        include/common/Utils.h
        # cpr
        $<TARGET_OBJECTS:cpr>)

target_link_libraries(wavefront-sdk PRIVATE ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(wavefront-sdk PRIVATE ${CURL_LIBRARIES})
if (UNIX AND NOT APPLE)
    target_link_libraries(wavefront-sdk PRIVATE rt)
endif ()
set_target_properties(wavefront-sdk PROPERTIES VERSION ${PROJECT_VERSION})
# --[ Main
add_executable(main ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(main PUBLIC wavefront-sdk Boost::iostreams)

set_target_properties(main PROPERTIES OUTPUT_NAME wavefront-sdk)

configure_file(../cmake/wavefront-sdk-lib.pc.in ../wavefront-sdk-lib.pc @ONLY)
install(TARGETS wavefront-sdk EXPORT wavefront-sdk-targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(FILES ${CMAKE_BINARY_DIR}/wavefront-sdk-lib.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)


