cmake_minimum_required(VERSION 3.16)
project(sailpush-tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Required modules
find_package(Qt5 REQUIRED COMPONENTS Core Test Network)

# Optional modules — tests that need them are skipped if unavailable
find_package(Qt5 QUIET COMPONENTS WebSockets DBus Multimedia)

enable_testing()
include_directories(${CMAKE_SOURCE_DIR}/../src)

# --- Pure Qt tests (no platform deps) ---

add_executable(tst_message
    tst_message.cpp
)
target_link_libraries(tst_message PRIVATE Qt5::Core Qt5::Test)
add_test(NAME Message COMMAND tst_message)

add_executable(tst_messagestore
    tst_messagestore.cpp
    ${CMAKE_SOURCE_DIR}/../src/messagestore.cpp
)
target_link_libraries(tst_messagestore PRIVATE Qt5::Core Qt5::Test)
add_test(NAME MessageStore COMMAND tst_messagestore)

add_executable(tst_sailpushclient
    tst_sailpushclient.cpp
    ${CMAKE_SOURCE_DIR}/../src/sailpushclient.cpp
)
target_link_libraries(tst_sailpushclient PRIVATE Qt5::Core Qt5::Network Qt5::Test)
add_test(NAME SailPushClient COMMAND tst_sailpushclient)

# --- WebSocket-dependent tests ---

if(TARGET Qt5::WebSockets)
    add_executable(tst_websocketmanager
        tst_websocketmanager.cpp
        ${CMAKE_SOURCE_DIR}/../src/websocketmanager.cpp
    )
    target_link_libraries(tst_websocketmanager PRIVATE Qt5::Core Qt5::WebSockets Qt5::Test)
    add_test(NAME WebSocketManager COMMAND tst_websocketmanager)
else()
    message(STATUS "Qt5WebSockets not found — skipping WebSocketManager tests")
endif()

# --- D-Bus dependent tests (use QSKIP when services unavailable) ---

if(TARGET Qt5::DBus)
    add_executable(tst_networkmonitor
        tst_networkmonitor.cpp
        ${CMAKE_SOURCE_DIR}/../src/networkmonitor.cpp
    )
    target_link_libraries(tst_networkmonitor PRIVATE Qt5::Core Qt5::DBus Qt5::Test)
    add_test(NAME NetworkMonitor COMMAND tst_networkmonitor)

    add_executable(tst_cpukeepalive
        tst_cpukeepalive.cpp
        ${CMAKE_SOURCE_DIR}/../src/cpukeepalive.cpp
    )
    target_link_libraries(tst_cpukeepalive PRIVATE Qt5::Core Qt5::DBus Qt5::Test)
    add_test(NAME CpuKeepalive COMMAND tst_cpukeepalive)

    add_executable(tst_notificationmanager
        tst_notificationmanager.cpp
        ${CMAKE_SOURCE_DIR}/../src/notificationmanager.cpp
        ${CMAKE_SOURCE_DIR}/../src/sailpushclient.cpp
    )
    target_link_libraries(tst_notificationmanager PRIVATE Qt5::Core Qt5::DBus Qt5::Network Qt5::Test)
    add_test(NAME NotificationManager COMMAND tst_notificationmanager)

    if(TARGET Qt5::WebSockets)
        add_executable(tst_dbusinterface
            tst_dbusinterface.cpp
            ${CMAKE_SOURCE_DIR}/../src/dbusinterface.cpp
            ${CMAKE_SOURCE_DIR}/../src/messagestore.cpp
            ${CMAKE_SOURCE_DIR}/../src/websocketmanager.cpp
        )
        target_link_libraries(tst_dbusinterface PRIVATE Qt5::Core Qt5::DBus Qt5::WebSockets Qt5::Test)
        add_test(NAME DbusInterface COMMAND tst_dbusinterface)
    else()
        message(STATUS "Qt5WebSockets not found — skipping DbusInterface tests")
    endif()
else()
    message(STATUS "Qt5DBus not found — skipping D-Bus dependent tests")
endif()
