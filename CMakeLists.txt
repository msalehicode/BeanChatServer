cmake_minimum_required(VERSION 3.16)

project(BeanChatServer LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Sql Gui)


add_subdirectory(
    ../BeanChatCommon
    BeanChatCommon
)

add_executable(BeanChatServer
  src/main.cpp
  src/network/clientsession.h src/network/clientsession.cpp

  src/server/server.h src/server/server.cpp

  src/models/database.h src/models/database.cpp
  src/models/user.h
  src/models/channel.h
  src/network/udpserver.h src/network/udpserver.cpp
  src/server/uploadsession.h
  src/models/attachment.h
  src/models/message.h

)

# app default configs
target_compile_definitions(BeanChatServer PRIVATE

  DATABASE_FILE_NAME="server.db"
  AVATARS_DIRECTORY_NAME="avatars"

  SERVER_VERSION="0.20.24"
  CLIENT_MINIMUM_VERSION="0.28.39"

  #default port
  DEFAULT_PORT=9987

  #udp ping and packet loss configs
  UDP_SEND_PINGS_INTERVAL=2000 #every 2 seconds
  UDP_CALCULATE_PACKETLOSS_INTERAL=5000 #every 5s calcualte packetloss video and voice
  UDP_MAX_TIMEOUT_TO_CONNECTION_LOST=15000 #when user dont send 15s pingResponse would assume that connection is lost
)

target_link_libraries(BeanChatServer
  Qt6::Core
  Qt6::Network
  Qt6::Sql
  Qt6::Gui

  BeanChatCommon
)

# for deploy, to tell where load libraries not system just ../lib, because on runtime it asks for library Qt6Network.so.6!
set_target_properties(BeanChatServer PROPERTIES
    BUILD_RPATH "\$ORIGIN/../lib"
    INSTALL_RPATH "\$ORIGIN/../lib"
)


include(GNUInstallDirs)
install(TARGETS BeanChatServer
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)



# debug tool to get sigfault infos.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
target_compile_options(BeanChatServer PRIVATE
   -fsanitize=address
   -g)
target_link_options(BeanChatServer PRIVATE
   -fsanitize=address)
endif()






# to deploy
qt_generate_deploy_app_script(
    TARGET BeanChatServer
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

