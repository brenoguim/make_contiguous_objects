add_subdirectory(unit)

add_custom_target(tests DEPENDS
    test.unit
)
