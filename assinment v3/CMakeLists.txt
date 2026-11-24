cmake_minimum_required(VERSION 3.20)
project(GLSolution CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

find_package(OpenGL REQUIRED)

add_library(OpenGL.SharedModule STATIC
    OpenGL.SharedModule/src/glad.c
    OpenGL.SharedModule/src/stb_image.cpp
    OpenGL.SharedModule/src/imgui/imgui.cpp
    OpenGL.SharedModule/src/imgui/imgui_demo.cpp
    OpenGL.SharedModule/src/imgui/imgui_draw.cpp
    OpenGL.SharedModule/src/imgui/imgui_impl_glfw.cpp
    OpenGL.SharedModule/src/imgui/imgui_impl_opengl3.cpp
    OpenGL.SharedModule/src/imgui/imgui_stdlib.cpp
    OpenGL.SharedModule/src/imgui/imgui_tables.cpp
    OpenGL.SharedModule/src/imgui/imgui_widgets.cpp
    OpenGL.SharedModule/src/implementation/Drawer.cpp
    OpenGL.SharedModule/src/implementation/OpenGlShape.cpp
    OpenGL.SharedModule/src/implementation/SharedModule.cpp
)

target_include_directories(OpenGL.SharedModule PUBLIC
    OpenGL.SharedModule/dependencies/include
    OpenGL.SharedModule/dependencies/include/imgui
    OpenGL.SharedModule/dependencies/include/imgui/backends
    OpenGL.SharedModule/dependencies/include/glad
    OpenGL.SharedModule/dependencies/include/GLFW
)

target_link_libraries(OpenGL.SharedModule PUBLIC
    OpenGL::GL
)

file(GLOB_RECURSE GLPROJECT_SOURCES
    "GLProject/*.cpp"
    "GLProject/*.c"
)

list(FILTER GLPROJECT_SOURCES EXCLUDE REGEX "(PrimitivesMain|ModelsMain|MultilightMain)\\.cpp$")

add_executable(GLProject
    ${GLPROJECT_SOURCES}
)

target_include_directories(GLProject PRIVATE
    GLProject
    OpenGL.SharedModule/dependencies/include
    OpenGL.SharedModule/dependencies/include/GLFW
    OpenGL.SharedModule/dependencies/include/imgui
    OpenGL.SharedModule/dependencies/include/imgui/backends
    OpenGL.SharedModule/dependencies/include/glad
)

if(MSVC)
    target_link_directories(GLProject PRIVATE
        OpenGL.SharedModule/dependencies/libs
    )
    target_link_libraries(GLProject PRIVATE
        OpenGL.SharedModule
        glfw3
        assimp-vc143-mt
        OpenGL::GL
    )
elseif(MINGW)
    find_package(glfw3 QUIET)
    find_package(assimp QUIET)

    if(TARGET glfw)
        set(GLFW_LINK_TARGET glfw)
    else()
        set(GLFW_LINK_TARGET glfw3)
    endif()

    if(TARGET assimp::assimp)
        set(ASSIMP_LINK_TARGET assimp::assimp)
    else()
        set(ASSIMP_LINK_TARGET assimp)
    endif()

    target_link_libraries(GLProject PRIVATE
        OpenGL.SharedModule
        ${GLFW_LINK_TARGET}
        ${ASSIMP_LINK_TARGET}
        OpenGL::GL
        gdi32
    )
else()
    target_link_libraries(GLProject PRIVATE
        OpenGL.SharedModule
        glfw
        assimp
        OpenGL::GL
    )
endif()

add_custom_command(
    TARGET GLProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/GLProject
            $<TARGET_FILE_DIR:GLProject>
    COMMENT "Syncing GLProject shaders, textures, and models to build output..."
)

if(MSVC AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/OpenGL.SharedModule/dependencies/dlls/assimp-vc143-mt.dll")
    add_custom_command(
        TARGET GLProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/OpenGL.SharedModule/dependencies/dlls/assimp-vc143-mt.dll"
                $<TARGET_FILE_DIR:GLProject>
        COMMENT "Copying assimp-vc143-mt.dll to build output..."
    )
endif()
