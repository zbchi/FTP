cmake_minimum_required(VERSION 3.10)
project(FTP)

file(GLOB CLIENT_SOURCES "client/*.cc")
file(GLOB SERVER_SOURCES "server/*.cc")

add_executable(client ${CLIENT_SOURCES})

add_executable(server ${SERVER_SOURCES})
