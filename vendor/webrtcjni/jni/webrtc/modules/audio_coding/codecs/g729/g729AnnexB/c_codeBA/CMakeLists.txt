project(g729)

add_definitions(
    -DWEBRTC_POSIX
)

set(G729_DIR        ${CMAKE_CURRENT_LIST_DIR})

include_directories(
    ..
    ../../
    ../../
    ../../../../../../../
    ../../../../../../../../

)

file(GLOB G729_FILES ${G729_DIR}/*.c)



add_library(g729 ${WME_LIB_TYPE}
    ${G729_FILES}
)
