project (StatisBig CXX)
cmake_minimum_required (VERSION 2.8)

find_package (OpenCV REQUIRED)
find_package (GSL REQUIRED)
find_package (Boost COMPONENTS filesystem REQUIRED)

add_compile_options (-std=c++11)
include_directories (${GSL_INCLUDE_DIRS})

aux_source_directory (src SRC_LIST)
add_executable (${PROJECT_NAME} ${SRC_LIST})
target_link_libraries (${PROJECT_NAME} ${OpenCV_LIBS} ${GSL_LIBRARIES} ${Boost_LIBRARIES})
