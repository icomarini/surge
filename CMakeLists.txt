cmake_minimum_required(VERSION 3.27)
project(surge VERSION 0.01)


# vulkan ======================================================================
find_package(Vulkan REQUIRED)


# glfw ========================================================================
find_package(glfw3 3.3 REQUIRED)


# stb =========================================================================
if (NOT DEFINED LIB_STB_INCLUDE)
    message(FATAL_ERROR "Missing path to stb include folder" )
endif()
message("Path to stb include folder: '${LIB_STB_INCLUDE}'" )

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${LIB_STB_INCLUDE})


# tinyonbjloader =============================================================
if (NOT DEFINED LIB_TINYOBJLOADER_INCLUDE)
    message(FATAL_ERROR "Missing path to tinyonbjloader include folder" )
endif()
message("Path to tinyonbjloader include folder: '${LIB_TINYOBJLOADER_INCLUDE}'" )

add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE ${LIB_TINYOBJLOADER_INCLUDE})


# ktx =========================================================================
if (NOT DEFINED LIB_KTX_INCLUDE)
    message(FATAL_ERROR "Missing path to Ktx include folder" )
endif()
message("Path to Ktx include folder: '${LIB_KTX_INCLUDE}'" )
if (NOT DEFINED LIB_KTX_LIB)
    message(FATAL_ERROR "Missing path to Ktx library folder" )
endif()
message("Path to Ktx library folder: '${LIB_KTX_LIB}'" )

add_library(ktx STATIC IMPORTED)
set_property(TARGET ktx PROPERTY IMPORTED_LOCATION ${LIB_KTX_LIB}/libktx.a)
target_include_directories(ktx INTERFACE ${LIB_KTX_INCLUDE})
# find_library(ktx ktx "${LIB_KTX_LIB}")
# target_link_libraries(ktx INTERFACE IMPORTED_LOCATION ktx)


# fastgltf ====================================================================
if (NOT DEFINED LIB_FASTGLTF_INCLUDE)
    message(FATAL_ERROR "Missing path to fastgltf include folder" )
endif()
message("Path to fastgltf include folder: '${LIB_FASTGLTF_INCLUDE}'" )
if (NOT DEFINED LIB_FASTGLTF_LIB)
    message(FATAL_ERROR "Missing path to fastgltf library folder" )
endif()
message("Path to fastgltf library folder: '${LIB_FASTGLTF_LIB}'" )

add_library(fastgltf STATIC IMPORTED)
set_property(TARGET fastgltf PROPERTY IMPORTED_LOCATION ${LIB_FASTGLTF_LIB}/libfastgltf.a)
target_include_directories(fastgltf INTERFACE ${LIB_FASTGLTF_INCLUDE})


# imgui =======================================================================
if (NOT DEFINED LIB_IMGUI_SOURCE)
    message(FATAL_ERROR "Missing path to ImGUI source folder" )
endif()
message("Path to ImGUI source folder: '${LIB_IMGUI_SOURCE}'" )

add_library(imgui STATIC
    ${LIB_IMGUI_SOURCE}/imgui.cpp
    ${LIB_IMGUI_SOURCE}/imgui_demo.cpp
    ${LIB_IMGUI_SOURCE}/imgui_draw.cpp
    ${LIB_IMGUI_SOURCE}/imgui_tables.cpp
    ${LIB_IMGUI_SOURCE}/imgui_widgets.cpp
    # ${LIB_IMGUI_SOURCE}/backends/imgui_impl_vulkan.cpp
    # ${LIB_IMGUI_SOURCE}/backends/imgui_impl_glfw.cpp
    )
target_include_directories(imgui PUBLIC ${LIB_IMGUI_SOURCE})
# target_include_directories(imgui PUBLIC ${LIB_IMGUI_SOURCE}/backends)


# miniaudio ===================================================================
if (NOT DEFINED LIB_MINIAUDIO_SOURCE)
    message(FATAL_ERROR "Missing path to miniaudio source folder" )
endif()
message("Path to miniaudio source folder: '${LIB_MINIAUDIO_SOURCE}'" )

set(miniaudio_dir /home/ico/projects/extern/miniaudio)
add_library(miniaudio STATIC
    ${LIB_MINIAUDIO_SOURCE}/miniaudio.c
    )
target_include_directories(miniaudio PUBLIC ${LIB_MINIAUDIO_SOURCE})


# clay ========================================================================
# add_library(clay INTERFACE)
# target_include_directories(clay INTERFACE ${LIB_CLAY_INCLUDE})


# surge =======================================================================
add_executable (surge.bin main.cpp)
set_property(TARGET surge.bin PROPERTY CXX_STANDARD 23)

target_include_directories(surge.bin PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(surge.bin LINK_PUBLIC
    vulkan
    glfw
    stb
    tinyobjloader
    imgui
    ktx
    fastgltf
    miniaudio 
    # clay
    )