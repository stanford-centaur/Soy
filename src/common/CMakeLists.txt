file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")


target_sources(${SOY_LIB} PRIVATE ${SRCS})
target_include_directories(${SOY_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${SOY_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${SOY_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/tests")

set (COMMON_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
macro(common_add_unit_test name)
    set(USE_MOCK_COMMON FALSE)
    set(USE_MOCK_ENGINE TRUE)
    soy_add_test(${COMMON_TESTS_DIR}/Test_${name} common USE_MOCK_COMMON USE_MOCK_ENGINE "unit")
endmacro()

common_add_unit_test(BoundManager)
common_add_unit_test(Error)
common_add_unit_test(File)
common_add_unit_test(FloatUtils)
common_add_unit_test(LinearExpression)
common_add_unit_test(List)
common_add_unit_test(MString)
common_add_unit_test(MStringf)
common_add_unit_test(Map)
common_add_unit_test(Set)
common_add_unit_test(Vector)
