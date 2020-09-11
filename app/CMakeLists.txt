add_executable(example-bin
    main.cc
)

set_target_properties(example-bin
    PROPERTIES
        OUTPUT_NAME example
)

target_link_libraries(example-bin
    PRIVATE
        example-lib
)
