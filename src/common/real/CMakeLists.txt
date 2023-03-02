file(GLOB SRCS "*.cpp")
file(GLOB HEADERS "*.h")

target_sources(${SOY_LIB} PRIVATE ${SRCS})
target_include_directories(${SOY_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")


if (${BUILD_PYTHON})
    target_include_directories(${SOY_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
