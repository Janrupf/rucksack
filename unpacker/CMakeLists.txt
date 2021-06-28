#######################################
# Self unpacking and executing binary #
#######################################
set(RUCKSACK_UNPACKER_SOURCE
        src/main.c
        src/stream.c)

if(WIN32)
    set(RUCKSACK_UNPACKER_PLATFORM_SOURCE
            src/file_win32.c)
else()
    set(RUCKSACK_UNPACKER_PLATFORM_SOURCE
            src/file_unix.c)
endif()

set(RUCKSACK_UNPACKER_INCLUDE_DIR
        "${CMAKE_CURRENT_LIST_DIR}/include")

add_executable(rucksack-unpacker ${RUCKSACK_UNPACKER_SOURCE} ${RUCKSACK_UNPACKER_PLATFORM_SOURCE})
target_include_directories(rucksack-unpacker PUBLIC ${RUCKSACK_UNPACKER_INCLUDE_DIR})
target_link_libraries(rucksack-unpacker PUBLIC rucksack lzma-sdk-7z)
