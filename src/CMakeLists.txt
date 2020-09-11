add_library(example-lib
    example.cc
)

set_target_properties(example-lib
    PROPERTIES
        OUTPUT_NAME example
)

target_include_directories(example-lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)
