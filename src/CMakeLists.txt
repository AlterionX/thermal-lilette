SET(pwd ${CMAKE_CURRENT_LIST_DIR})

SET(src "")

AUX_SOURCE_DIRECTORY(${pwd} src)
add_executable(thermal-lilette ${src})
message(STATUS "thermal-lilette added ${src}")

target_link_libraries(thermal-lilette ${stdgl_libraries})
FIND_PACKAGE(JPEG REQUIRED)
TARGET_LINK_LIBRARIES(thermal-lilette ${JPEG_LIBRARIES})
TARGET_LINK_LIBRARIES(thermal-lilette pmdreader)
