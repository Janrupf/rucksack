####################################
# Core library for helper purposes #
####################################
set(RUCKSACK_SOURCE
        src/rucksack.c)

set(RUCKSACK_INCLUDE_DIR
        "${CMAKE_CURRENT_LIST_DIR}/include")

add_library(rucksack STATIC ${RUCKSACK_SOURCE})
target_include_directories(rucksack PUBLIC ${RUCKSACK_INCLUDE_DIR})
