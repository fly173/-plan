# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles\\UCAVDesigner_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\UCAVDesigner_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\core_tests_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\core_tests_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\ucav_core_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\ucav_core_autogen.dir\\ParseCache.txt"
  "UCAVDesigner_autogen"
  "core_tests_autogen"
  "ucav_core_autogen"
  )
endif()
