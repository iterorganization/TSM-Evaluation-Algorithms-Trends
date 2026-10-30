if(NOT BUILD_TESTS)
    return()
endif()

set(VERIFICATION_DIR "${CMAKE_SOURCE_DIR}/tests")
set(MODEL_TESTS_DIR "${CMAKE_SOURCE_DIR}/tests")

if(EXISTS "${VERIFICATION_DIR}/test_1_one_step_cooldown.cpp")
    set(TEST_SOURCES_DIR "${VERIFICATION_DIR}")
elseif(EXISTS "${MODEL_TESTS_DIR}/test_1_one_step_cooldown.cpp")
    set(TEST_SOURCES_DIR "${MODEL_TESTS_DIR}")
else()
    message(FATAL_ERROR "Could not find regression test sources under ../verification or ../models/MCTF/tests")
endif()

if(NOT EXISTS "${TEST_SOURCES_DIR}/reference")
    message(FATAL_ERROR "Missing reference directory next to tests: ${TEST_SOURCES_DIR}/reference")
endif()

set(PIECEWISE_TEST_ASSET_DIR "${TEST_SOURCES_DIR}" CACHE PATH
    "Directory containing sensors_config.json/configuration_MCTF.json and data/ for runtime tests")

# Runtime working directory expected by test executables.
set(TEST_RUNTIME_DIR "${PIECEWISE_TEST_ASSET_DIR}")

set(PIECEWISE_TEST_NAMES
    test_1_one_step_cooldown
    # test_2_one_step_cooldown_from_file
    # test_3_cooldown_history_from_file
    # test_4_cooldown_history_with_piecewise_algorithm
    # test_5_cooldown_history_from_100k_with_piecewise_algorithm
    # test_6_cooldown_history_subset_sensors_from_file
)

foreach(TEST_NAME IN LISTS PIECEWISE_TEST_NAMES)
    set(TEST_SOURCE "${TEST_SOURCES_DIR}/${TEST_NAME}.cpp")
    if(NOT EXISTS "${TEST_SOURCE}")
        message(FATAL_ERROR "Missing test source: ${TEST_SOURCE}")
    endif()

    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE evaluation_algorithms_trends_static)

    add_test(
        NAME ${TEST_NAME}
        COMMAND ${CMAKE_COMMAND}
            -DTEST_EXE=$<TARGET_FILE:${TEST_NAME}>
            -DTEST_NAME=${TEST_NAME}
            -DWORK_DIR=${TEST_RUNTIME_DIR}
            -DREF_FILE=${TEST_SOURCES_DIR}/reference/${TEST_NAME}.ref
            -DOUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/outputs
            -P ${CMAKE_CURRENT_LIST_DIR}/run_and_compare_output.cmake
    )
endforeach()
