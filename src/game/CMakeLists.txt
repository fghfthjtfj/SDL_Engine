add_executable(Game)

target_sources(Game PRIVATE
    src/main.cpp
    src/Game.cpp
)

target_include_directories(Game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Game PRIVATE Engine)

set_target_properties(Game PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src/game"
)

# DLL рядом с exe при каждой сборке (для отладки — оставляем)
add_custom_command(TARGET Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/SDL3/bin/SDL3.dll"
        $<TARGET_FILE_DIR:Game>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/SDL3_image/lib/x64/SDL3_image.dll"
        $<TARGET_FILE_DIR:Game>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/SDL3_shadercross/bin/SDL3_shadercross.dll"
        $<TARGET_FILE_DIR:Game>
)

# ============================================================
# INSTALL — запускается только cmake --install
# ============================================================

# Сам exe
install(TARGETS Game
    RUNTIME DESTINATION .       # Game.exe ляжет в корень dist/
)

# DLL-ки
install(FILES
    "${CMAKE_SOURCE_DIR}/external/SDL3/bin/SDL3.dll"
    "${CMAKE_SOURCE_DIR}/external/SDL3_image/lib/x64/SDL3_image.dll"
    DESTINATION .
)

# Ресурсы игры
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/models/"
    DESTINATION models
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/textures/"
    DESTINATION textures
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sounds/"
    DESTINATION sounds
)

# Шейдеры движка
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/engine/shaders/"
    DESTINATION ../engine/shaders
)