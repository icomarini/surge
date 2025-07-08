cmake_minimum_required(VERSION 3.27)
project(ico VERSION 0.01)

# --- extern stuff ------------------------------------------------------------

# vulkan
find_package(Vulkan REQUIRED)

# glfw
find_package(glfw3 3.3 REQUIRED)

# stb
set(std_dir //home/ico/projects/extern/stb)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${std_dir})

# tinyonbjloader
set(tinyobjloader_dir /home/ico/projects/extern/tinyobjloader)
add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE ${tinyobjloader_dir})

# ktx
set(ktx_dir /home/ico/projects/extern/KTX-Software/install)
add_library(ktx STATIC IMPORTED)
set_property(TARGET ktx PROPERTY IMPORTED_LOCATION ${ktx_dir}/lib/libktx.a)
target_include_directories(ktx INTERFACE ${ktx_dir}/include)

# fastgltf
set(fastgltf_dir /home/ico/projects/extern/fastgltf/install)
add_library(fastgltf STATIC IMPORTED)
set_property(TARGET fastgltf PROPERTY IMPORTED_LOCATION ${fastgltf_dir}/lib/libfastgltf.a)
target_include_directories(fastgltf INTERFACE ${fastgltf_dir}/include)

# imgui
set(imgui_dir /home/ico/projects/extern/imgui)
add_library(imgui STATIC
    ${imgui_dir}/imgui.cpp
    ${imgui_dir}/imgui_demo.cpp
    ${imgui_dir}/imgui_draw.cpp
    ${imgui_dir}/imgui_tables.cpp
    ${imgui_dir}/imgui_widgets.cpp
    ${imgui_dir}/backends/imgui_impl_vulkan.cpp
    ${imgui_dir}/backends/imgui_impl_glfw.cpp
    )
target_include_directories(imgui PUBLIC ${imgui_dir})
target_include_directories(imgui PUBLIC ${imgui_dir}/backends)


# miniaudio
set(miniaudio_dir /home/ico/projects/extern/miniaudio)
add_library(miniaudio STATIC
    ${miniaudio_dir}/miniaudio.c
    )
target_include_directories(miniaudio PUBLIC ${miniaudio_dir})

# clay
# set(clay_dir /home/ico/projects/extern/clay)
# add_library(clay INTERFACE)
# target_include_directories(clay INTERFACE ${clay_dir})

# --- app stuff ---------------------------------------------------------------
add_executable (surge.bin main.cpp)
target_include_directories(surge.bin PUBLIC /home/ico/projects/surge/include)
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