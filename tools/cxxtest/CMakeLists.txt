file(GLOB HEADERS "cxxtest/*.h")

target_sources(${SOY_LIB} PRIVATE ${SRCS})
target_include_directories(${SOY_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
