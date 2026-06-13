
target_include_directories(app PRIVATE .)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/id.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/id_shell.c)
