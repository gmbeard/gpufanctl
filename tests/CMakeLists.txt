include(MakeTest)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_library(testing OBJECT testing.cpp)

# NOTE:
#  Use -DGPUFANCTL_ENABLE_TEST_CATEGORIES="val1;val2" to control the
#  ENABLE_IF flags

make_test(NAME slope_tests SOURCES slope_tests.cpp LINK_LIBRARIES GpuFanCtl::gpufanctl)
make_test(NAME validation_tests SOURCES validation_tests.cpp LINK_LIBRARIES GpuFanCtl::gpufanctl)
make_test(NAME curve_parsing_tests SOURCES curve_parsing_tests.cpp LINK_LIBRARIES GpuFanCtl::gpufanctl)
make_test(NAME cmdline_tests SOURCES cmdline_tests.cpp LINK_LIBRARIES GpuFanCtl::gpufanctl)

add_subdirectory(execution)
