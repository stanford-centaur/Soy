set(MPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mps_example)

set(PARSERS_OUT_DIR ${CMAKE_BINARY_DIR}/input_parsers)

file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")

target_sources(${SOY_LIB} PRIVATE ${SRCS})
target_include_directories(${SOY_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(${SOY_TEST_LIB} PRIVATE ${SRCS})
target_include_directories(${SOY_TEST_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

macro(soy_parser name dir)
    add_executable(${name} "${dir}/main.cpp")
    target_link_libraries(${name} ${SOY_LIB})
    target_include_directories(${name} PRIVATE ${LIBS_INCLUDES})
    set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PARSERS_OUT_DIR})
endmacro()

soy_parser(${MPS_PARSER} ${MPS_DIR})
add_custom_command(TARGET ${MPS_PARSER} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${MPS_PARSER}> ${MPS_PARSER_PATH} )

# set_target_properties(${MPS_PARSER} PROPERTIES RUNTIME_OUTPUT_DIRECTORY  ${PARSERS_OUT_DIR})

