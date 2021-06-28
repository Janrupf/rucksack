######################
# LZMA SDK from 7zip #
######################
# Variable setup
set(LZMA_SDK_DIR "${CMAKE_CURRENT_BINARY_DIR}/lzma-sdk")
set(LZMA_SDK_FILE "${LZMA_SDK_DIR}/lzma.7z")
set(LZMA_SDK_VERSION_FILE "${LZMA_SDK_DIR}/.version")

# LZMA SDK version
set(LZMA_SDK_VERSION "2102" CACHE STRING "The LZMA SDK version to use")

# Check the installed version if possible
if(EXISTS "${LZMA_SDK_VERSION_FILE}")
    file(READ "${LZMA_SDK_VERSION_FILE}" LZMA_SKD_INSTALLED_VERSION)
endif()

# If the installed version doesn't match the selected, nuke the directory
if(NOT "${LZMA_SDK_VERSION}" STREQUAL "${LZMA_SKD_INSTALLED_VERSION}")
    file(REMOVE_RECURSE "${LZMA_SDK_DIR}")
endif()

# If the LZMA SDK dir does not exist, download it
if(NOT EXISTS "${LZMA_SDK_DIR}")
    # Download the SDK file
    file(DOWNLOAD
            "https://www.7-zip.org/a/lzma${LZMA_SDK_VERSION}.7z"
            "${LZMA_SDK_FILE}"
            SHOW_PROGRESS
            STATUS LZMA_SDK_DOWNLOAD_STATUS
            LOG LZMA_SDK_DOWNLOAD_LOG
    )

    # Extract status information from the download
    list(GET LZMA_SDK_DOWNLOAD_STATUS 0 LZMA_SDK_DOWNLOAD_CODE)
    list(GET LZMA_SDK_DOWNLOAD_STATUS 1 LZMA_SDK_DOWNLOAD_MESSAGE)

    # Check if the download succeeded
    if(NOT LZMA_SDK_DOWNLOAD_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download the LZMA SDK: ${LZMA_SDK_DOWNLOAD_MESSAGE}\n${LZMA_SDK_DOWNLOAD_LOG}")
    endif()

    # Extract the SDK
    execute_process(
            COMMAND "${CMAKE_COMMAND}" -E tar xf "${LZMA_SDK_FILE}"
            WORKING_DIRECTORY "${LZMA_SDK_DIR}"
            RESULT_VARIABLE LZMA_SDK_EXTRACT_ERROR
    )

    # Check for extraction errors
    if(LZMA_SDK_EXTRACT_ERROR)
        message(FATAL_ERROR "Failed to extract the LZMA SDK: ${LZMA_SDK_EXTRACT_ERROR}")
    endif()

    # Clean up and set version
    file(REMOVE "${LZMA_SDK_FILE}")
    file(WRITE "${LZMA_SDK_VERSION_FILE}" "${LZMA_SDK_VERSION}")
endif()

set(LZMA_SDK_7z_EXTRACT_SOURCE
        "${LZMA_SDK_DIR}/C/7zAlloc.c"
        "${LZMA_SDK_DIR}/C/7zArcIn.c"
        "${LZMA_SDK_DIR}/C/7zBuf.c"
        "${LZMA_SDK_DIR}/C/7zBuf2.c"
        "${LZMA_SDK_DIR}/C/7zCrc.c"
        "${LZMA_SDK_DIR}/C/7zCrcOpt.c"
        "${LZMA_SDK_DIR}/C/7zDec.c"
        "${LZMA_SDK_DIR}/C/7zFile.c"
        "${LZMA_SDK_DIR}/C/7zStream.c"
        "${LZMA_SDK_DIR}/C/Bcj2.c"
        "${LZMA_SDK_DIR}/C/Bra.c"
        "${LZMA_SDK_DIR}/C/Bra86.c"
        "${LZMA_SDK_DIR}/C/BraIA64.c"
        "${LZMA_SDK_DIR}/C/CpuArch.c"
        "${LZMA_SDK_DIR}/C/Delta.c"
        "${LZMA_SDK_DIR}/C/Lzma2Dec.c"
        "${LZMA_SDK_DIR}/C/Lzma2DecMt.c"
        "${LZMA_SDK_DIR}/C/LzmaDec.c")

set(LZMA_SDK_INCLUDE_DIR
        "${LZMA_SDK_DIR}/C")

add_library(lzma-sdk-7z STATIC ${LZMA_SDK_7z_EXTRACT_SOURCE})
target_include_directories(lzma-sdk-7z PUBLIC ${LZMA_SDK_INCLUDE_DIR})
