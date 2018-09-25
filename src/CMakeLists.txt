add_library(wavefront-sdk
        common/SocketException.cpp
        include/common/SocketException.h
        common/Socket.cpp
        include/common/Socket.h
        proxy/ProxyConnectionHandler.cpp
        include/proxy/ProxyConnectionHandler.h
        include/proxy/ProxyClient.h
        include/common/HistogramGranularity.h
        proxy/ProxyClient.cpp
        include/common/Serializer.h
        include/direct_ingestion/DirectIngesterService.h
        direct_ingestion/DirectIngesterService.cpp
        direct_ingestion/DirectIngestionClient.cpp
        include/direct_ingestion/DirectIngestionClient.h
        include/common/WavefrontSender.h

        # cpr
        $<TARGET_OBJECTS:cpr>)

target_link_libraries(wavefront-sdk PRIVATE ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(wavefront-sdk PRIVATE ${CURL_LIBRARIES})
if (UNIX AND NOT APPLE)
    target_link_libraries(wavefront-sdk PRIVATE rt)
endif ()
# --[ Main
add_executable(main ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(main PUBLIC wavefront-sdk Boost::iostreams)

set_target_properties(main PROPERTIES OUTPUT_NAME wavefront-sdk)

install(TARGETS wavefront-sdk EXPORT wavefront-sdk-targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


