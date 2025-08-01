function (target_shaders target shaders)
    set(SHADERS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    file(MAKE_DIRECTORY "${SHADERS_BUILD_DIR}")
    foreach (SHADER_PATH ${SHADERS})
        get_filename_component(SHADER_FILENAME "${SHADER_PATH}" NAME)
        set(SHADER_SPIRV_PATH "${SHADERS_BUILD_DIR}/${SHADER_FILENAME}.spv")
        set(DEPFILE "${SHADER_SPIRV_PATH}.d")
        add_custom_command(
          COMMENT "Building ${SHADER_FILENAME}"
          OUTPUT "${SHADER_SPIRV_PATH}"
          COMMAND ${GLSLC} "${SHADER_PATH}" -o "${SHADER_SPIRV_PATH}" -MD -MF ${DEPFILE} -g
          DEPENDS "${SHADER_PATH}"
          DEPFILE "${DEPFILE}"
        )
        list(APPEND SPIRV_BINARY_FILES ${SHADER_SPIRV_PATH})
    endforeach()

    set(shaders_target_name "${target}_build_shaders")
    add_custom_target(${shaders_target_name}
      DEPENDS ${SPIRV_BINARY_FILES}
    )
    add_dependencies(${target} ${shaders_target_name})
endfunction()

if (NOT DEFINED EXE_GLSLC)
    message(FATAL_ERROR "Missing GLSLC executable" )
endif()