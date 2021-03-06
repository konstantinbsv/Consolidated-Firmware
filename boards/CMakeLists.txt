set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.11)

# Export JSON compilation database for third-party programs (e.g. clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Target-platform specific settings
if("${PLATFORM}" STREQUAL "x86")
    # We assume this cmake is running on a x86 platform, so the 
    # default compiler settings should be fine
    project(FORMULA_ELECTRIC_FIRMWARE C CXX)
elseif("${PLATFORM}" STREQUAL "arm")
    set(CMAKE_C_COMPILER_WORKS 1)
    set(TOOLCHAIN_PREFIX arm-none-eabi-)
    set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
    set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
    set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
    set(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump)
    set(SIZE ${TOOLCHAIN_PREFIX}size)

    project(FORMULA_ELECTRIC_FIRMWARE C ASM)
else()
    message(FATAL_ERROR
        "ERROR: Please set the PLATFORM variable to 'x86' or 'arm'.")
endif()
message("Platform: ${PLATFORM}")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Globally Accessible ARM Flags
set(FPU_FLAGS
    -mcpu=cortex-m4 
    -mfloat-abi=hard 
    -mfpu=fpv4-sp-d16
    )

# Globally accessible paths
set(REPO_ROOT_DIR ${CMAKE_SOURCE_DIR}/../)
set(SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/../scripts)
set(PIPENV_PROJECT_DIR ${REPO_ROOT_DIR}) # Use "pipenv run" in this directory to
                                         # ensure the correct PWD gets used in
                                         # the .env file.

# Globally accessible executables/scripts
if(NOT STM32CUBEMX_BIN_PATH)
    # Default STM32CubeMX path
    if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
        set(STM32CUBEMX_BIN_PATH "C:/Program Files (x86)/STMicroelectronics/STM32Cube/STM32CubeMX/STM32CubeMX.exe")
    else()
        set(STM32CUBEMX_BIN_PATH /usr/local/STM32CubeMX/STM32CubeMX)
    endif()
endif()

# Globally accessible targets
if("${PLATFORM}" STREQUAL "x86")
    # This target can be used to build the tests for all the boards
    add_custom_target(all_tests)
elseif("${PLATFORM}" STREQUAL "arm")
    # This target can be used to build the firmware binaries for all the boards
    add_custom_target(all_arm_binaries)
endif()

# Generate STM32CubeMX driver code for BOARD_NAME using the given IOC_FILE in
# the directory where this function is called from.
function(cube_mx_code_generation
            BOARD_NAME
            IOC_FILE
            )
    set(GENERATE_CUBE_CODE_SCRIPT_PY
            ${SCRIPTS_DIR}/utilities/generate_cube_code.py)
    set(FIX_FORMATTING_SCRIPT_PY
            ${REPO_ROOT_DIR}/clang-format/fix_formatting.py)
    set(LOG4J_PROPERTIES "$ENV{HOME}/.stm32cubemx/log4j.properties")

    add_custom_command(
        OUTPUT ${IOC_FILE}.md5
               ${LOG4J_PROPERTIES}
        COMMAND pipenv run python
            ${GENERATE_CUBE_CODE_SCRIPT_PY}
            --board ${BOARD_NAME}
            --log4j_properties_output ${LOG4J_PROPERTIES}
            --ioc ${IOC_FILE}
            --codegen_output_dir ${CMAKE_CURRENT_SOURCE_DIR}
            --cube_bin ${STM32CUBEMX_BIN_PATH}
        # Create a MD5 hash of IOC_FILE for other build targets to depend on, so
        # this custom command is only executed when IOC_FILE.md5 changes.
        COMMAND pipenv run python
            ${SCRIPTS_DIR}/utilities/generate_md5_checksum.py
            ${IOC_FILE}
            ${IOC_FILE}.md5
        # Run clang-format because the STM32CubeMX-generated code is not
        # compliant to our clang-format rules.
        COMMAND pipenv run python
            ${FIX_FORMATTING_SCRIPT_PY}
        DEPENDS ${IOC_FILE}
        WORKING_DIRECTORY ${PIPENV_PROJECT_DIR}
        )
endfunction()

# Generate C code to store the Git hash in read-only memory
# Code will be generated in the directory that this function was called from
function(git_hash_code_generation GIT_HASH_HEADER)
    add_custom_command(
        OUTPUT "${GIT_HASH_HEADER}"
        COMMAND
            echo "#define GIT_HASH $(git rev-parse head)" > ${GIT_HASH_HEADER}
        )
endfunction()

# Create the Arm binary executable that can be flashed to a board
#   BOARD_NAME - The name of the board. This will be used to name the executable
#   ARM_BINARY_SRCS - Source files for the Arm binary
#   ARM_BINARY_INCLUDE_DIRS - Include directories for the Arm binary
#   ARM_BINARY_THIRD_PARTY_SRCS - Third-party source files for the Arm binary
#                                 that should have warnings suppressed
#   ARM_BINARY_THIRD_PARTY_INCLUDE_DIRS - Third-party include directories for
#                                         the Arm binary that should have
#                                         warnings suppressed
#   ARM_LINKER_SCRIPT - The Arm linker script for the board
#   STM32_DEVICE_TYPE - The device name required for <stm32f3xx.h> to work
function(create_arm_binary
            BOARD_NAME
            ARM_BINARY_SRCS
            ARM_BINARY_INCLUDE_DIRS
            ARM_BINARY_THIRD_PARTY_SRCS
            ARM_BINARY_THIRD_PARTY_INCLUDE_DIRS
            ARM_LINKER_SCRIPT
            STM32_DEVICE_TYPE
            )
    add_executable(${BOARD_NAME}.elf
        ${ARM_BINARY_SRCS}
        ${ARM_BINARY_THIRD_PARTY_SRCS}
        ${SHARED_ARM_BINARY_X86_COMPATIBLE_SRCS}
        ${SHARED_ARM_BINARY_X86_INCOMPATIBLE_SRCS}
        ${SHARED_ARM_BINARY_X86_INCOMPATIBLE_THIRD_PARTY_SRCS}
        ${BOARD_NAME}.ioc.md5
        ${GIT_HASH_AUTOGENERATED_SRC} 
        )
    add_dependencies(all_arm_binaries ${BOARD_NAME}.elf)
    target_include_directories(${BOARD_NAME}.elf
        PRIVATE
            ${ARM_BINARY_INCLUDE_DIRS}
            ${SHARED_ARM_BINARY_INCLUDE_DIRS}
        )
    # Suppress header file warnings for third-party IO code that are not meant
    # to be modified by marking them as system includes
    target_include_directories(${BOARD_NAME}.elf SYSTEM
        PRIVATE
            ${ARM_BINARY_THIRD_PARTY_INCLUDE_DIRS}
            ${SHARED_ARM_BINARY_X86_INCOMPATIBLE_THIRD_PARTY_INCLUDE_DIRS}
        )
    # Suppress source file warnings for third-party IO code that are not meant to
    # be modified
    set_source_files_properties(
            ${ARM_BINARY_THIRD_PARTY_SRCS}
            ${SHARED_ARM_BINARY_X86_INCOMPATIBLE_THIRD_PARTY_SRCS}
        PROPERTIES COMPILE_FLAGS "-w")

    target_compile_definitions(${BOARD_NAME}.elf
        PRIVATE
            -D__weak=__attribute__\(\(weak\)\) 
            -D__packed=__attribute__\(\(__packed__\)\) 
            -DUSE_HAL_DRIVER 
            -D${STM32_DEVICE_TYPE}
            -DARM_MATH_CM4 
            -DARM_MATH_MATRIX_CHECK 
            -DARM_MATH_ROUNDING
        )
    target_compile_options(${BOARD_NAME}.elf
        PUBLIC
            ${FPU_FLAGS}
            -mthumb
            -mthumb-interwork
            -ffunction-sections
            -fdata-sections
            -g3
            -fno-common
            -fmessage-length=0
            -Wall
            -Werror
            -Wextra
            -pedantic
            -specs=nosys.specs
            -specs=nano.specs
            -Wdouble-promotion
            -Wshadow
            -Wundef
            -fstack-usage
            -Wconversion
        )
    target_link_options(${BOARD_NAME}.elf
        PUBLIC
            ${FPU_FLAGS}
            -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${BOARD_NAME}.map
            -Wl,-gc-sections,--print-memory-usage
            -Wl,-T ${ARM_LINKER_SCRIPT}
            --specs=nano.specs
        )

    set(HEX_FILE "${BOARD_NAME}.hex")
    set(BIN_FILE "${BOARD_NAME}.bin")
    set(ASM_FILE "${BOARD_NAME}.asm")
    
    add_custom_command(TARGET ${BOARD_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
            -Oihex $<TARGET_FILE:${BOARD_NAME}.elf> $<TARGET_FILE_DIR:${BOARD_NAME}.elf>/${HEX_FILE}
    COMMAND ${CMAKE_OBJCOPY}
            -Obinary $<TARGET_FILE:${BOARD_NAME}.elf> $<TARGET_FILE_DIR:${BOARD_NAME}.elf>/${BIN_FILE}
    COMMAND ${CMAKE_OBJDUMP}
            -DS $<TARGET_FILE:${BOARD_NAME}.elf> > $<TARGET_FILE_DIR:${BOARD_NAME}.elf>/${ASM_FILE}
    COMMENT "
Building ${HEX_FILE}
Building ${BIN_FILE}
Building ${ASM_FILE}")
endfunction()

# Generate C code for sending/receiving CAN messages for BOARD_NAME
function(can_code_generation
            BOARD_NAME
            APP_CAN_TX_SRC_OUTPUT
            APP_CAN_TX_HEADER_OUTPUT
            IO_CAN_TX_SRC_OUTPUT
            IO_CAN_TX_HEADER_OUTPUT
            APP_CAN_RX_SRC_OUTPUT
            APP_CAN_RX_HEADER_OUTPUT
            IO_CAN_RX_SRC_OUTPUT
            IO_CAN_RX_HEADER_OUTPUT
            APP_CANMSG_SRC_OUTPUT
            APP_CANMSG_HEADER_OUTPUT
            DBC_FILE
            )
    add_custom_command(
        OUTPUT ${APP_CAN_TX_SRC_OUTPUT}
               ${APP_CAN_TX_HEADER_OUTPUT}
               ${IO_CAN_TX_SRC_OUTPUT}
               ${IO_CAN_TX_HEADER_OUTPUT}
               ${APP_CAN_RX_SRC_OUTPUT}
               ${APP_CAN_RX_HEADER_OUTPUT}
               ${IO_CAN_RX_SRC_OUTPUT}
               ${IO_CAN_RX_HEADER_OUTPUT}
               ${APP_CANMSG_SRC_OUTPUT}
               ${APP_CANMSG_HEADER_OUTPUT}
        COMMAND pipenv run python
                ${SCRIPTS_DIR}/codegen/CAN/generate_c_code_from_dbc.py
                --board                    ${BOARD_NAME}
                --app_can_tx_source_output ${APP_CAN_TX_SRC_OUTPUT}
                --app_can_tx_header_output ${APP_CAN_TX_HEADER_OUTPUT}
                --io_can_tx_source_output  ${IO_CAN_TX_SRC_OUTPUT}
                --io_can_tx_header_output  ${IO_CAN_TX_HEADER_OUTPUT}
                --app_can_rx_source_output ${APP_CAN_RX_SRC_OUTPUT}
                --app_can_rx_header_output ${APP_CAN_RX_HEADER_OUTPUT}
                --io_can_rx_source_output  ${IO_CAN_RX_SRC_OUTPUT}
                --io_can_rx_header_output  ${IO_CAN_RX_HEADER_OUTPUT}
                --cantools_source_output   ${APP_CANMSG_SRC_OUTPUT}
                --cantools_header_output   ${APP_CANMSG_HEADER_OUTPUT}
                --dbc                      ${DBC_FILE}
        DEPENDS ${DBC_FILE}
        WORKING_DIRECTORY ${PIPENV_PROJECT_DIR}
        )
endfunction()

# Generate ErrorId enum using the DBC
function(error_ids_generation
        ERROR_ID_HEADER_OUTPUT
        DBC_FILE
        )
    add_custom_command(
            OUTPUT ${ERROR_ID_HEADER_OUTPUT}
            COMMAND pipenv run python
                ${SCRIPTS_DIR}/codegen/ErrorId/generate_error_ids.py
                --dbc         ${DBC_FILE}
                --output_path ${ERROR_ID_HEADER_OUTPUT}
            DEPENDS ${DBC_FILE}
            WORKING_DIRECTORY ${PIPENV_PROJECT_DIR}
    )
endfunction()

function(compile_googletest_executable
        BOARD_NAME
        TEST_SRCS
        TEST_INCLUDE_DIRS
        ARM_BINARY_X86_COMPATIBLE_SRCS
        ARM_BINARY_INCLUDE_DIRS
        )
    set(TEST_EXECUTABLE_NAME "${BOARD_NAME}_test")
    add_executable(${TEST_EXECUTABLE_NAME}
            ${TEST_SRCS}
            ${ARM_BINARY_X86_COMPATIBLE_SRCS}
            ${SHARED_ARM_BINARY_X86_COMPATIBLE_SRCS}
            )
    add_dependencies(all_tests ${TEST_EXECUTABLE_NAME})
    target_include_directories(${TEST_EXECUTABLE_NAME}
        PRIVATE
            ${TEST_INCLUDE_DIRS}
            ${ARM_BINARY_INCLUDE_DIRS}
            ${SHARED_ARM_BINARY_INCLUDE_DIRS}
            ${SHARED_GOOGLETEST_TEST_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/shared/Test_Utils
            )
    target_compile_options(${TEST_EXECUTABLE_NAME}
        PUBLIC
            -Wall
            -Werror
            -g3
            )
    target_link_libraries(${TEST_EXECUTABLE_NAME} gtest_main)
    add_test(NAME ${TEST_EXECUTABLE_NAME}
             COMMAND ${TEST_EXECUTABLE_NAME})
endfunction()

function(download_and_unpack_google_test GOOGLETEST_DOWNLOAD_SCRIPT)
    # Download and unpack googletest at configure time
    configure_file(${GOOGLETEST_DOWNLOAD_SCRIPT} googletest-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
    if(result)
        message(FATAL_ERROR "CMake step for googletest failed: ${result}")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
    if(result)
        message(FATAL_ERROR "Build step for googletest failed: ${result}")
    endif()

    # Prevent overriding the parent project's compiler/linker
    # settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Add googletest directly to our build. This defines
    # the gtest and gtest_main targets.
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
            ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
            EXCLUDE_FROM_ALL)
endfunction()

# Creates the Arm binary or tests for a board, depending on what CPU architecture
# this is being compiled for.
#
# @note The App and Io source files must be sorted in two platform-specific 
#       lists:
#
#         1. ARM_BINARY_X86_COMPATIBLE_SRCS - These can be compiled on x86
#         2. ARM_BINARY_X86_INCOMPATIBLE_SRCS - These can't be compiled on x86
#
#       Every App file can be compiled on x86. However, the same is not true
#       for Io files. Some Io files rely on STM headers (e.g. stm32f3xx.h) and
#       thus can't be compiled on x86. The rest of the Io files, however, can
#       be compiled on x86.
#
#       These relationships can be summarized as follows:
#
#       ARM_BINARY_X86_COMPATIBLE_SRCS = App source files + x86-compatible Io files
#       ARM_BINARY_X86_INCOMPATIBLE_SRCS = Io sources files - x86-compatible Io files
#
#       On the other hand, the App and Io include directories are always
#       x86-compatible because they are paths and not actual files. We just have
#       to place App and Io include directories in ARM_BINARY_INCLUDE_DIRS.
#
#   BOARD_NAME - The name of the board, will be used to name the binary or tests
#   IOC_FILE - The IOC file for the board
#   ARM_BINARY_X86_COMPATIBLE_SRCS - Source files for the Arm binary that can be
#                                    compiled on x86
#   ARM_BINARY_X86_INCOMPATIBLE_SRCS - Source files for the Arm binary that
#                                      can't be compiled on x86
#   ARM_BINARY_INCLUDE_DIRS - The include directories for the Arm binary
#   ARM_BINARY_THIRD_PARTY_SRCS - Third-party source files for the Arm binary
#                                 that should have warnings suppressed
#   ARM_BINARY_THIRD_PARTY_INCLUDE_DIRS - Third-party include directories for
#                                         the Arm binary that should have
#                                         warnings suppressed
#   ARM_LINKER_SCRIPT - The Arm linker script for the board
#   STM32_DEVICE_TYPE - The device name required for <stm32f3xx.h> to work
#   GOOGLETEST_TEST_SRCS - Source files for the test programs
#   GOOGLETEST_TEST_INCLUDE_DIRS - Include directories for the test programs
function(create_arm_binary_or_tests_for_board
            BOARD_NAME
            IOC_FILE
            ARM_BINARY_X86_COMPATIBLE_SRCS
            ARM_BINARY_X86_INCOMPATIBLE_SRCS
            ARM_BINARY_INCLUDE_DIRS
            ARM_BINARY_THIRD_PARTY_SRCS
            ARM_BINARY_THIRD_PARTY_INCLUDE_DIRS
            ARM_LINKER_SCRIPT
            STM32_DEVICE_TYPE
            GOOGLETEST_TEST_SRCS
            GOOGLETEST_TEST_INCLUDE_DIRS
            )
    # Construct the paths for the generated CAN code
    set(APP_CAN_TX_SRC_FILE_NAME     "App_CanTx.c")
    set(APP_CAN_TX_HEADER_FILE_NAME  "App_CanTx.h")
    set(IO_CAN_TX_SRC_FILE_NAME      "Io_CanTx.c")
    set(IO_CAN_TX_HEADER_FILE_NAME   "Io_CanTx.h")
    set(APP_CAN_RX_SRC_FILE_NAME     "App_CanRx.c")
    set(APP_CAN_RX_HEADER_FILE_NAME  "App_CanRx.h")
    set(IO_CAN_RX_SRC_FILE_NAME      "Io_CanRx.c")
    set(IO_CAN_RX_HEADER_FILE_NAME   "Io_CanRx.h")
    set(APP_CANMSGS_SRC_FILE_NAME    "App_CanMsgs.c")
    set(APP_CANMSGS_HEADER_FILE_NAME "App_CanMsgs.h")

    set(BOARD_SPECIFIC_AUTOGENERATED_APP_SRC_DIR
            "${CMAKE_CURRENT_BINARY_DIR}/Src/App")
    set(BOARD_SPECIFIC_AUTOGENERATED_APP_INCLUDE_DIR
            "${CMAKE_CURRENT_BINARY_DIR}/Inc/App")
    set(BOARD_SPECIFIC_AUTOGENERATED_IO_SRC_DIR
            "${CMAKE_CURRENT_BINARY_DIR}/Src/Io")
    set(BOARD_SPECIFIC_AUTOGENERATED_IO_INCLUDE_DIR
            "${CMAKE_CURRENT_BINARY_DIR}/Inc/Io")

    set(APP_CAN_TX_SRC_FILE
            "${BOARD_SPECIFIC_AUTOGENERATED_APP_SRC_DIR}/${APP_CAN_TX_SRC_FILE_NAME}")
    set(APP_CAN_TX_HEADER_FILE
            "${BOARD_SPECIFIC_AUTOGENERATED_APP_INCLUDE_DIR}/${APP_CAN_TX_HEADER_FILE_NAME}")
    set(IO_CAN_TX_SRC_FILE
            "${BOARD_SPECIFIC_AUTOGENERATED_IO_SRC_DIR}/${IO_CAN_TX_SRC_FILE_NAME}")
    set(IO_CAN_TX_HEADER_FILE
            "${BOARD_SPECIFIC_AUTOGENERATED_IO_INCLUDE_DIR}/${IO_CAN_TX_HEADER_FILE_NAME}")
    set(APP_CAN_RX_SRC_FILE
            "${BOARD_SPECIFIC_AUTOGENERATED_APP_SRC_DIR}/${APP_CAN_RX_SRC_FILE_NAME}")
    set(APP_CAN_RX_HEADER_FILE
            "${BOARD_SPECIFIC_AUTOGENERATED_APP_INCLUDE_DIR}/${APP_CAN_RX_HEADER_FILE_NAME}")
    set(IO_CAN_RX_SRC_FILE
            "${BOARD_SPECIFIC_AUTOGENERATED_IO_SRC_DIR}/${IO_CAN_RX_SRC_FILE_NAME}")
    set(IO_CAN_RX_HEADER_FILE
            "${BOARD_SPECIFIC_AUTOGENERATED_IO_INCLUDE_DIR}/${IO_CAN_RX_HEADER_FILE_NAME}")
    set(APP_CAN_MSGS_SRC_FILE
            "${BOARD_SPECIFIC_AUTOGENERATED_APP_SRC_DIR}/${APP_CANMSGS_SRC_FILE_NAME}")
    set(APP_CAN_MSGS_INCLUDE_FILE
            "${BOARD_SPECIFIC_AUTOGENERATED_APP_INCLUDE_DIR}/${APP_CANMSGS_HEADER_FILE_NAME}")
    set(DBC_FILE
            "${SCRIPTS_DIR}/codegen/CAN/App_CanMsgs.dbc")

    set(AUTOGENERATED_CAN_APP_SRCS
            "${APP_CAN_TX_SRC_FILE}"
            "${APP_CAN_RX_SRC_FILE}"
            "${APP_CAN_MSGS_SRC_FILE}"
            )
    set(AUTOGENERATED_CAN_IO_SRCS
            "${IO_CAN_TX_SRC_FILE}"
            "${IO_CAN_RX_SRC_FILE}"
            )
    set(AUTOGENERATED_CAN_APP_INCLUDE_DIRS
            "${BOARD_SPECIFIC_AUTOGENERATED_APP_INCLUDE_DIR}")
    set(AUTOGENERATED_CAN_IO_INCLUDE_DIRS
            "${BOARD_SPECIFIC_AUTOGENERATED_IO_INCLUDE_DIR}")

    list(APPEND ARM_BINARY_X86_COMPATIBLE_SRCS
            "${AUTOGENERATED_CAN_APP_SRCS}")
    list(APPEND ARM_BINARY_INCLUDE_DIRS
            "${AUTOGENERATED_CAN_APP_INCLUDE_DIRS}")
    list(APPEND ARM_BINARY_X86_INCOMPATIBLE_SRCS
            "${AUTOGENERATED_CAN_IO_SRCS}")
    list(APPEND ARM_BINARY_INCLUDE_DIRS
            "${AUTOGENERATED_CAN_IO_INCLUDE_DIRS}")

    can_code_generation(
        "${BOARD_NAME}"
        "${APP_CAN_TX_SRC_FILE}"
        "${APP_CAN_TX_HEADER_FILE}"
        "${IO_CAN_TX_SRC_FILE}"
        "${IO_CAN_TX_HEADER_FILE}"
        "${APP_CAN_RX_SRC_FILE}"
        "${APP_CAN_RX_HEADER_FILE}"
        "${IO_CAN_RX_SRC_FILE}"
        "${IO_CAN_RX_HEADER_FILE}"
        "${APP_CAN_MSGS_SRC_FILE}"
        "${APP_CAN_MSGS_INCLUDE_FILE}"
        "${DBC_FILE}"
        )

    set(ERROR_ID_HEADER_FILE_NAME "App_ErrorId.h")
    set(ERROR_ID_HEADER_FILE
            "${BOARD_SPECIFIC_AUTOGENERATED_APP_INCLUDE_DIR}/${ERROR_ID_HEADER_FILE_NAME}")
    error_ids_generation(
            "${ERROR_ID_HEADER_FILE}"
            "${DBC_FILE}")
    list(APPEND ARM_BINARY_X86_COMPATIBLE_SRCS
            "${ERROR_ID_HEADER_FILE}")

    set(GIT_HASH_HEADER "${BOARD_SPECIFIC_AUTOGENERATED_APP_INCLUDE_DIR}/git_hash.h")
    git_hash_code_generation(${GIT_HASH_HEADER})

    if ("${PLATFORM}" STREQUAL "x86")
        compile_googletest_executable(
                "${BOARD_NAME}"
                "${GOOGLETEST_TEST_SRCS}"
                "${GOOGLETEST_TEST_INCLUDE_DIRS}"
                "${ARM_BINARY_X86_COMPATIBLE_SRCS}"
                "${ARM_BINARY_INCLUDE_DIRS}"
        )
    elseif("${PLATFORM}" STREQUAL "arm")
        cube_mx_code_generation(
            "${BOARD_NAME}"
            "${IOC_FILE}"
            )
        list(APPEND ARM_BINARY_SRCS
                ${ARM_BINARY_X86_COMPATIBLE_SRCS}
                ${ARM_BINARY_X86_INCOMPATIBLE_SRCS}
                )
        list(APPEND ARM_BINARY_INCLUDE_DIRS
                ${BOARD_SPECIFIC_AUTOGENERATED_APP_INCLUDE_DIR}
                ${BOARD_SPECIFIC_AUTOGENERATED_IO_INCLUDE_DIR}
                )
        create_arm_binary(
            "${BOARD_NAME}"
            "${ARM_BINARY_SRCS}"
            "${ARM_BINARY_INCLUDE_DIRS}"
            "${ARM_BINARY_THIRD_PARTY_SRCS}"
            "${ARM_BINARY_THIRD_PARTY_INCLUDE_DIRS}"
            "${ARM_LINKER_SCRIPT}"
            "${STM32_DEVICE_TYPE}"
            )
    endif()
endfunction()

if ("${PLATFORM}" STREQUAL "x86")
    download_and_unpack_google_test(${CMAKE_SOURCE_DIR}/CMakeLists.txt.in)
    # For ctest to return a non-zero code when Valgrind fails, we must use
    # --error-exitcode. On the other hand, --leak-check=full ensures that any
    # memory leak issue will cause ctest to return a non-zero code.
    #
    # This must be set before include(CTest) is called, or else
    # MEMORYCHECK_COMMAND_OPTIONS won't be overwritten correctly!
    set(MEMORYCHECK_COMMAND_OPTIONS "--error-exitcode=1 --leak-check=full")
    include (CTest)
    enable_testing()
endif()

add_subdirectory(shared __shared__)
add_subdirectory(DCM)
add_subdirectory(PDM)
add_subdirectory(FSM)
add_subdirectory(BMS)
add_subdirectory(DIM)
