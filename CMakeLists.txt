cmake_minimum_required(VERSION 3.5)

set(KF5_VERSION "5.63.0") # handled by release scripts
project(KBusyIndicatorGadget VERSION ${KF5_VERSION})

include(FeatureSummary)

# drop the Qt requirements w.r.t. the current framework version
set(REQUIRED_QT_VERSION 5.8.0)
find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Widgets)

include_directories(${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_AUTOMOC ON)

qt5_add_resources(ICON_SRCS icons.qrc)
set(SRCS
    kbusyindicatorwidget.cpp
    kbusyindicatorwidgettest.cpp
    ${ICON_SRCS})

add_executable(kbusyindicatorwidgettest ${SRCS})
target_link_libraries(kbusyindicatorwidgettest
    Qt5::Widgets)

feature_summary(WHAT ALL   FATAL_ON_MISSING_REQUIRED_PACKAGES)
