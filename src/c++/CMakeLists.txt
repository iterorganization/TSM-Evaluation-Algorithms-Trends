set(PIECEWISE_ESTIMATOR_SOURCES
    piecewise_estimator_lib.cpp
    piecewise_estimator_api.cpp
)

add_library(piecewise_estimator_shared SHARED ${PIECEWISE_ESTIMATOR_SOURCES})
add_library(piecewise_estimator_static STATIC ${PIECEWISE_ESTIMATOR_SOURCES})

# Keep the same logical library name for static/shared outputs.
set_target_properties(piecewise_estimator_shared PROPERTIES OUTPUT_NAME piecewise_estimator)
set_target_properties(piecewise_estimator_static PROPERTIES OUTPUT_NAME piecewise_estimator)

target_include_directories(piecewise_estimator_shared
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(piecewise_estimator_static
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(piecewise_estimator_shared
    PUBLIC
        Threads::Threads
)

target_link_libraries(piecewise_estimator_static
    PUBLIC
        Threads::Threads
)

if(EXTRA_INC_DIRS)
    target_include_directories(piecewise_estimator_shared SYSTEM PRIVATE ${EXTRA_INC_DIRS})
    target_include_directories(piecewise_estimator_static SYSTEM PRIVATE ${EXTRA_INC_DIRS})
endif()

if(EXTRA_LIB_DIRS)
    target_link_directories(piecewise_estimator_shared PRIVATE ${EXTRA_LIB_DIRS})
    target_link_directories(piecewise_estimator_static PRIVATE ${EXTRA_LIB_DIRS})
endif()

# Copy public headers into <build>/include (next to <build>/lib)
file(GLOB ESTIMATOR_PUBLIC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include")

add_custom_target(export_public_headers ALL
    DEPENDS ${ESTIMATOR_PUBLIC_HEADERS}
    COMMAND ${CMAKE_COMMAND} -E copy ${ESTIMATOR_PUBLIC_HEADERS} ${CMAKE_BINARY_DIR}/include
    COMMENT "Copying public headers to ${CMAKE_BINARY_DIR}/include"
)
