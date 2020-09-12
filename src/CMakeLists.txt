add_library(fps-lib
    fps.cc
)

set_target_properties(fps-lib
    PROPERTIES
        OUTPUT_NAME fps
)

target_include_directories(fps-lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(fps-lib
    PUBLIC
        ffmpeg-avformat
)
