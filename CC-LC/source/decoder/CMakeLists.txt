# executable
set( EXE_NAME DecoderApp )

# get source files
file( GLOB SRC_FILES "*.cpp" )

# get include files
file( GLOB INC_FILES "*.h" )

# get additional libs for gcc on Ubuntu systems
if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    if( USE_ADDRESS_SANITIZER )
      set( ADDITIONAL_LIBS asan )
    endif()
  endif()
endif()

# add executable
add_executable( ${EXE_NAME} ${SRC_FILES} ${INC_FILES} )
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if( CMAKE_COMPILER_IS_GNUCC AND BUILD_STATIC )
  set( ADDITIONAL_LIBS ${ADDITIONAL_LIBS} -static -static-libgcc -static-libstdc++ )
  target_compile_definitions( ${EXE_NAME} PUBLIC ENABLE_WPP_STATIC_LINK=1 )
endif()

target_link_libraries( ${EXE_NAME} CommonLib ${ADDITIONAL_LIBS} )

# example: place header files in different folders
source_group( "Natvis Files" FILES ${NATVIS_FILES} )

# set the folder where to place the projects
set_target_properties( ${EXE_NAME}  PROPERTIES FOLDER app LINKER_LANGUAGE CXX )
