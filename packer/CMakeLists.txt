###############################################################
# Helper for packing a 7z archive to a self unpacking binary  #
###############################################################
set(RUCKSACK_PACKER_SOURCE
        src/main.c)

set(RUCKSACK_PACKER_INCLUDE_DIR
        "${CMAKE_CURRENT_LIST_DIR}/include")

add_executable(rucksack-packer ${RUCKSACK_PACKER_SOURCE})
target_include_directories(rucksack-packer PUBLIC ${RUCKSACK_PACKER_INCLUDE_DIR})
target_link_libraries(rucksack-packer PUBLIC rucksack)
