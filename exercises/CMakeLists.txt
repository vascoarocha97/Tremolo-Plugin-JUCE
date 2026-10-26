# Cada subpasta NN-nome com um CMakeLists.txt é um exercício.
# A ordem alfabética coincide com a ordem do currículo.
# CONFIGURE_DEPENDS: uma pasta nova é detetada no build seguinte (o CMake volta a configurar sozinho).

file(GLOB exercise_dirs CONFIGURE_DEPENDS LIST_DIRECTORIES true RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/[0-9][0-9]-*")
list(SORT exercise_dirs)

foreach(dir IN LISTS exercise_dirs)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
        add_subdirectory("${dir}")
    endif()
endforeach()
