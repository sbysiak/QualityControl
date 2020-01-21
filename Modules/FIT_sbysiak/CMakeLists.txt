# ---- Library ----

add_library(QcFIT_sbysiak)

target_sources(QcFIT_sbysiak PRIVATE)

target_include_directories(
  QcFIT_sbysiak
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(QcFIT_sbysiak PUBLIC QualityControl)

install(TARGETS QcFIT_sbysiak
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_root_dictionary(QcFIT_sbysiak
                    HEADERS
                    LINKDEF include/FIT_sbysiak/LinkDef.h
                    BASENAME QcFIT_sbysiak)

# ---- Test(s) ----

set(TEST_SRCS test/testQcFIT_sbysiak.cxx)

foreach(test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
                        PRIVATE QcFIT_sbysiak Boost::unit_test_framework)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_property(TARGET ${test_name}
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 20)
endforeach()

