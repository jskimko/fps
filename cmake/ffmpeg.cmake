# ffmpeg.cmake
# Create a target for an ffmpeg module using ffmpeg_module(name).

function(ffmpeg_module mod)
    if(TARGET ffmpeg-${mod})
        return()
    endif()

    if(${mod} STREQUAL "avcodec")
        ffmpeg_avcodec_init()
    elseif(${mod} STREQUAL "avformat")
        ffmpeg_avformat_init()
    elseif(${mod} STREQUAL "avutil")
        ffmpeg_avutil_init()
    elseif(${mod} STREQUAL "swresample")
        ffmpeg_swresample_init()
    else()
        message(FATAL_ERROR "ffmpeg_module: invalid module '${mod}'")
    endif()
endfunction()

# Helpers.

function(ffmpeg_create_target mod)
    string(TOUPPER ${mod} MOD)
    set(MOD_INCLUDE_DIR FFMPEG_${MOD}_INCLUDE_DIR)
    set(MOD_LIBRARY     FFMPEG_${MOD}_LIBRARY)

    find_path(${MOD_INCLUDE_DIR} lib${mod}/${mod}.h)
    find_library(${MOD_LIBRARY} ${mod})

    if(NOT ${MOD_INCLUDE_DIR} OR NOT ${MOD_LIBRARY})
        message(FATAL_ERROR "ffmpeg_create_target: failed to find module '${mod}'")
    endif()

    set(target ffmpeg-${mod})
    add_library(${target} STATIC IMPORTED GLOBAL)
    set_target_properties(${target}
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${${MOD_INCLUDE_DIR}}"
            IMPORTED_LOCATION             "${${MOD_LIBRARY}}"
    )

endfunction()

function(ffmpeg_avcodec_init)
    ffmpeg_create_target(avcodec)
    ffmpeg_module(swresample)
    target_link_libraries(ffmpeg-avcodec
        INTERFACE
            ffmpeg-swresample
            ffmpeg-avutil
    )
endfunction()

function(ffmpeg_avformat_init)
    ffmpeg_create_target(avformat)
    ffmpeg_module(avcodec)
    ffmpeg_module(avutil)
    target_link_libraries(ffmpeg-avformat 
        INTERFACE
           ffmpeg-avcodec 
    )
endfunction()

function(ffmpeg_avutil_init)
    ffmpeg_create_target(avutil)
    find_package(Threads)
    target_link_libraries(ffmpeg-avutil
        INTERFACE
            Threads::Threads
    )
endfunction()

function(ffmpeg_swresample_init)
    ffmpeg_create_target(swresample)
endfunction()
