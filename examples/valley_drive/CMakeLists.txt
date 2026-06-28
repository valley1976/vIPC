set(VALLEY_DIR "${CMAKE_INSTALL_DIR}")

ADD_EXECUTABLE(valley_drive "main.cpp")
target_include_directories(valley_drive PRIVATE ../../)
target_link_libraries(valley_drive PRIVATE vIPC)