cmake_minimum_required(VERSION 3.16)
project(Sticky VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# --- Qt6 ---
find_package(Qt6 COMPONENTS Widgets REQUIRED)
message(STATUS "Found Qt6: ${Qt6_VERSION}")

# --- Resources (embedded icon for standalone binaries) ---
qt6_add_resources(STICKY_RESOURCES "assets.qrc")

# --- Target ---
add_executable(sticky main.cpp ${STICKY_RESOURCES})

target_link_libraries(sticky PRIVATE Qt6::Widgets)

# --- Install rules ---
install(TARGETS sticky RUNTIME DESTINATION bin)

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../sticky.desktop"
        DESTINATION share/applications)

# Icons — hicolor icon theme directories
set(ICON_SIZES 16 32 48 64 128 256)
foreach(size ${ICON_SIZES})
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../assets/sticky-${size}.png"
            DESTINATION share/icons/hicolor/${size}x${size}/apps
            RENAME sticky.png)
endforeach()

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../assets/sticky.svg"
        DESTINATION share/icons/hicolor/scalable/apps)

# --- CPack ---
set(CPACK_PACKAGE_NAME "sticky")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simple sticky notes application")
set(CPACK_PACKAGE_VENDOR "cxza3by")
set(CPACK_PACKAGE_CONTACT "cxza3by <cxza3by@gmail.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE")

# Suffix for package naming
if(NOT DEFINED CPACK_PACKAGE_SUFFIX)
    set(CPACK_PACKAGE_SUFFIX "")
endif()

set(CPACK_GENERATOR "TGZ")

# DEB
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_FILE_NAME "sticky-${CPACK_PACKAGE_VERSION}${CPACK_PACKAGE_SUFFIX}-Linux.deb")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-base")

# RPM
set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0")
set(CPACK_RPM_COMPRESSION_TYPE "zstd")
set(CPACK_RPM_FILE_NAME "sticky-${CPACK_PACKAGE_VERSION}${CPACK_PACKAGE_SUFFIX}.rpm")
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase")

include(CPack)
