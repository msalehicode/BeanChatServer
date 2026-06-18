cmake_minimum_required(VERSION 3.16)

project(BeanChatServer LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core Network Sql)

add_executable(BeanChatServer
  src/main.cpp
  src/network/packet.h src/network/packet.cpp
  src/network/packets.h
  src/network/packethelpers.h
  src/network/clientsession.h src/network/clientsession.cpp

  src/server/server.h src/server/server.cpp

  src/models/database.h src/models/database.cpp
  src/models/user.h
  src/models/channel.h
  src/network/voicepackets.h src/network/voicepackets.cpp
  src/network/udpserver.h src/network/udpserver.cpp

)
target_link_libraries(BeanChatServer Qt6::Core Qt6::Network Qt6::Sql)

include(GNUInstallDirs)
install(TARGETS BeanChatServer
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
