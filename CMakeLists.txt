project(mipmaps)
#---------------------------------------------------------
#                     Platform options
#---------------------------------------------------------

cmake_minimum_required(VERSION 2.8)
IF(CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-function -std=c++0x")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

add_definitions(-D__STDC_CONSTANT_MACROS)


#----------------------------------------------------------
#                     Configure dependencies
#----------------------------------------------------------

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules")


find_package(OpenGL REQUIRED)
find_package(GLFW REQUIRED)
find_package(GLEW REQUIRED)

set(LIBRARIES ${OPENGL_LIBRARY} ${GLFW_LIBRARY} ${GLEW_LIBRARY})

include_directories(glhack)
include_directories(include)


#----------------------------------------------------------
#                     Base binaries
#----------------------------------------------------------



# libglhack
set(GLHACK_SRC

glhack/tinythread.cpp
glhack/tinymt32.c
glhack/stb_image.c
glhack/shims_and_types.cpp
glhack/persistent_containers.cpp
glhack/math_tools.cpp
glhack/masp.cpp
glhack/managed_structures.cpp
glhack/glhack.cpp
glhack/glbase.cpp
glhack/geometry.cpp
glhack/conversion.c
glhack/allocators.cpp
)

set_source_files_properties(${GLHACK_SRC} PROPERTIES LANGUAGE "CXX")

add_library(glhack ${GLHACK_SRC})


# Test
set(UNITTEST_SRC
glhack_unittests/glhack_tests.cpp
glhack_unittests/masp_tests.cpp
glhack_unittests/unittester.cpp
)

add_executable(unittests ${UNITTEST_SRC})
target_link_libraries(unittests glhack ${LIBRARIES})

# Masp repl
set(MASP_REPL_SRC
   masp_repl/masp_repl.cpp 
)

add_executable(masp_repl glhack ${MASP_REPL_SRC})
target_link_libraries(masp_repl glhack ${LIBRARIES})

#----------------------------------------------------------
#                     Applications
#----------------------------------------------------------

# Hidden lines
set(HID_LINES_SRC
programs/hiddenlines/hiddenlines.cpp
)

add_executable(hiddenlines ${HID_LINES_SRC})
target_link_libraries(hiddenlines glhack ${LIBRARIES})


# add_executable(mipmaps mipmaps.c)
# target_link_libraries(mipmaps ${LIBRARIES})













