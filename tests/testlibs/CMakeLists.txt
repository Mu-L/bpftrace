file(GLOB testlib_sources CONFIGURE_DEPENDS *.c *.cpp)
set(testlibtargets "")
foreach(testlib_source ${testlib_sources})
  get_filename_component(testlib_name ${testlib_source} NAME_WE)
  add_library(${testlib_name} SHARED ${testlib_source})
  set_target_properties(${testlib_name}
    PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMPILE_FLAGS "-g -O0")
  target_include_directories(${testlib_name} PRIVATE ${CMAKE_SOURCE_DIR}/tests/include)
  # clear the executable bit - ensure bpftrace can trace non-executable
  # shared objects
  add_custom_command(TARGET ${testlib_name}
    POST_BUILD
    COMMAND chmod -x $<TARGET_FILE_NAME:${testlib_name}>)
  list(APPEND testlibtargets ${testlib_name})
endforeach()
add_custom_target(testlibs DEPENDS ${testlibtargets})
