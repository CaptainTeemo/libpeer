project(lib)

file(GLOB SRCS "*.c" "librtp/*.c" "utils/*.c")

file(GLOB HEADERS "peer.h" "peer_connection.h" "peer_signaling.h" "mongoose.h" "utils/darray.h")

add_library(peer SHARED
        ${SRCS}
)

target_link_libraries(peer ${DEP_LIBS})
target_include_directories(peer PRIVATE librtp/include)

set_target_properties(peer PROPERTIES PUBLIC_HEADER "${HEADERS}")

add_definitions(-DMG_TLS=MG_TLS_MBED)

install(TARGETS peer
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include/
)
