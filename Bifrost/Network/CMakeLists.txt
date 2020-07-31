################################################################################
#                                                                              #
#                      BIFROST XXXXXXXXXXXXXX PROJECT                          #
#                                                                              #
################################################################################
###  CMakeList.txt :                                                         ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BF_Networking VERSION 0.0.1 DESCRIPTION "" LANGUAGES C CXX)

add_executable(
  BFNetworkTest
  "network_main.cpp"
  "NetworkContext.cpp"
  "NetworkPlatform.cpp"
  "Socket.cpp"
)

target_include_directories(
  BFNetworkTest

  PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
