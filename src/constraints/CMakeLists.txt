file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")


target_sources(${SOY_LIB} PRIVATE ${SRCS})
target_include_directories(${SOY_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${SOY_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${SOY_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

set (CONSTRAINT_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
macro(constraint_add_unit_test name)
  set(USE_MOCK_COMMON FALSE)
  set(USE_MOCK_ENGINE FALSE)
  soy_add_test(${CONSTRAINT_TESTS_DIR}/Test_${name} constraints USE_MOCK_COMMON
    USE_MOCK_ENGINE "unit")
endmacro()

constraint_add_unit_test(AbsoluteValueConstraint)
constraint_add_unit_test(DisjunctionConstraint)
constraint_add_unit_test(OneHotConstraint)
constraint_add_unit_test(IntegerConstraint)
constraint_add_unit_test(PLConstraint)
