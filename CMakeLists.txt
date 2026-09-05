cmake_minimum_required(VERSION 3.21)
project(OpenBioUnlockDesktop VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Network Widgets Bluetooth)
find_package(OpenSSL 3.0 REQUIRED COMPONENTS Crypto)
include(FetchContent)
FetchContent_Declare(qrcodegen URL https://github.com/nayuki/QR-Code-generator/archive/refs/tags/v1.8.0.zip DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(qrcodegen)
set(OPENBIOUNLOCK_SOURCES
    src/main.cpp
    src/core/server.cpp src/core/server.hpp
    src/core/crypto.cpp src/core/crypto.hpp
    src/core/credential.cpp src/core/credential.hpp
    src/ui/main_window.cpp src/ui/main_window.hpp
    src/ui/pairing_dialog.cpp src/ui/pairing_dialog.hpp
    src/core/ble_discovery.cpp src/core/ble_discovery.hpp)
add_executable(openbiounlock-desktop ${OPENBIOUNLOCK_SOURCES})
target_include_directories(openbiounlock-desktop PRIVATE src)
target_link_libraries(openbiounlock-desktop PRIVATE Qt6::Core Qt6::Network Qt6::Widgets Qt6::Bluetooth OpenSSL::Crypto qrcodegen)
target_compile_definitions(openbiounlock-desktop PRIVATE QT_NO_KEYWORDS)
if(WIN32)
    target_link_libraries(openbiounlock-desktop PRIVATE Crypt32)
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSECRET REQUIRED libsecret-1)
    target_compile_definitions(openbiounlock-desktop PRIVATE OPENBIOUNLOCK_HAS_LIBSECRET)
    target_include_directories(openbiounlock-desktop PRIVATE ${LIBSECRET_INCLUDE_DIRS})
    target_link_directories(openbiounlock-desktop PRIVATE ${LIBSECRET_LIBRARY_DIRS})
    target_link_libraries(openbiounlock-desktop PRIVATE ${LIBSECRET_LIBRARIES})
endif()
if(MSVC)
    target_compile_options(openbiounlock-desktop PRIVATE /W4 /permissive-)
else()
    target_compile_options(openbiounlock-desktop PRIVATE -Wall -Wextra -Wpedantic)
endif()

find_program(CARGO_EXECUTABLE cargo)
if(CARGO_EXECUTABLE)
    add_custom_target(openbiounlock-daemon
        COMMAND ${CARGO_EXECUTABLE} build --release --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/daemon/Cargo.toml
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(WIN32)
    add_library(openbiounlock-credential-provider SHARED windows-cp/CredentialProvider.cpp windows-cp/dllmain.cpp windows-cp/CredentialProvider.h)
    target_include_directories(openbiounlock-credential-provider PRIVATE windows-cp)
    target_compile_features(openbiounlock-credential-provider PRIVATE cxx_std_17)
    install(TARGETS openbiounlock-credential-provider RUNTIME DESTINATION bin)
elseif(UNIX AND NOT APPLE)
    find_program(MAKE_EXECUTABLE make)
    if(MAKE_EXECUTABLE)
        add_custom_target(openbiounlock-pam
            COMMAND ${MAKE_EXECUTABLE} -C ${CMAKE_CURRENT_SOURCE_DIR}/linux-pam
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS openbiounlock-desktop RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set(CPACK_PACKAGE_NAME "OpenBioUnlock")
set(CPACK_PACKAGE_VENDOR "OpenBioUnlock")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenBioUnlock")
if(WIN32)
    set(CPACK_GENERATOR "WIX;ZIP")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
endif()
include(CPack)