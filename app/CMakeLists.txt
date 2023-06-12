project(desktop LANGUAGES CXX)

include(common/common.cmake)
include(../3thrd/3thrd.cmake)
include(desktop/desktop.cmake)

add_executable(watermark-desktop desktop/main.cpp
        ${SINGLEAPP_GUI_SRC} ${DESKTOP_SRC}
        )
target_link_libraries(watermark-desktop Qt5::Widgets Qt5::Network ${GIO_LIBRARIES})
target_include_directories(watermark-desktop PUBLIC ${Qt5Core_INCLUDE_DIRS} ${Qt5Widget_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
