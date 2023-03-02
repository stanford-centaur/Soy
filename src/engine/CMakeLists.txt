file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")


target_sources(${SOY_LIB} PRIVATE ${SRCS})
target_include_directories(${SOY_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${SOY_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${SOY_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

set (ENGINE_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
macro(engine_add_unit_test name)
    set(USE_MOCK_COMMON FALSE)
    set(USE_MOCK_ENGINE FALSE)
    soy_add_test(${ENGINE_TESTS_DIR}/Test_${name} engine USE_MOCK_COMMON
        USE_MOCK_ENGINE "unit")
endmacro()

engine_add_unit_test(GurobiWrapper)
engine_add_unit_test(InputQuery)
engine_add_unit_test(MILPEncoder)
engine_add_unit_test(SmtCore)
engine_add_unit_test(SatSolver)
