if(NOT DEFINED TEST_EXE OR NOT DEFINED TEST_NAME OR NOT DEFINED WORK_DIR OR NOT DEFINED REF_FILE OR NOT DEFINED OUT_DIR)
    message(FATAL_ERROR "Missing required arguments for run_and_compare_output.cmake")
endif()

file(MAKE_DIRECTORY "${OUT_DIR}")
set(ACTUAL_FILE "${OUT_DIR}/${TEST_NAME}.out")
set(ERROR_FILE "${OUT_DIR}/${TEST_NAME}.err")

execute_process(
    COMMAND "${TEST_EXE}"
    WORKING_DIRECTORY "${WORK_DIR}"
    RESULT_VARIABLE TEST_EXIT_CODE
    OUTPUT_FILE "${ACTUAL_FILE}"
    ERROR_FILE "${ERROR_FILE}"
)

if(NOT TEST_EXIT_CODE EQUAL 0)
    file(READ "${ERROR_FILE}" TEST_STDERR)
    message(FATAL_ERROR "${TEST_NAME} failed with exit code ${TEST_EXIT_CODE}. stderr:\n${TEST_STDERR}")
endif()

if(NOT EXISTS "${REF_FILE}")
    message(FATAL_ERROR "Reference file not found: ${REF_FILE}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${REF_FILE}" "${ACTUAL_FILE}"
    RESULT_VARIABLE CMP_EXIT_CODE
)

if(NOT CMP_EXIT_CODE EQUAL 0)
    file(READ "${REF_FILE}" REF_CONTENT)
    file(READ "${ACTUAL_FILE}" ACTUAL_CONTENT)
    message(FATAL_ERROR
        "Output mismatch for ${TEST_NAME}.\n"
        "Reference (${REF_FILE}):\n${REF_CONTENT}\n"
        "Actual (${ACTUAL_FILE}):\n${ACTUAL_CONTENT}\n"
    )
endif()
