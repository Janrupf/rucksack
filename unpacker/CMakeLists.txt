#######################################
# Self unpacking and executing binary #
#######################################
set(RUCKSACK_UNPACKER_SOURCE
        src/file.c
        src/main.c
        src/stream.c)

set(RUCKSACK_UNPACKER_INCLUDE_DIR
        "${CMAKE_CURRENT_LIST_DIR}/include")

add_executable(rucksack-unpacker ${RUCKSACK_UNPACKER_SOURCE})
target_include_directories(rucksack-unpacker PUBLIC ${RUCKSACK_UNPACKER_INCLUDE_DIR})
target_link_libraries(rucksack-unpacker PUBLIC rucksack lzma-sdk-7z)
