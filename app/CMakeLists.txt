add_executable(fps-bin
    main.cc
)

set_target_properties(fps-bin
    PROPERTIES
        OUTPUT_NAME fps
)

target_link_libraries(fps-bin
    PRIVATE
        fps-lib
        ffmpeg-avformat
)
