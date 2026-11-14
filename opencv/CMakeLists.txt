# 3.5 is the oldest compatibility level accepted by current CMake releases.
cmake_minimum_required (VERSION 3.5)
project (InterfaceCurvatureTracking CXX)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (OpenCV REQUIRED)
find_package (GSL REQUIRED)
find_package (Boost COMPONENTS filesystem REQUIRED)

include_directories (${GSL_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

# The input data are not part of this repository, so there is no default:
# pass -DMSC_DATA_DIR=/path/to/data, or set MSC_DATA before configuring.
set (MSC_DATA_DIR "$ENV{MSC_DATA}" CACHE PATH "Folder that holds the input data (see README.md, Data availability)")
if (NOT MSC_DATA_DIR)
  message (FATAL_ERROR "MSC_DATA_DIR is not set. Pass -DMSC_DATA_DIR=/path/to/data or set MSC_DATA; "
                       "the data are not part of this repository (see README.md, Data availability).")
endif ()
if (NOT IS_DIRECTORY "${MSC_DATA_DIR}/marker-templates")
  message (FATAL_ERROR "${MSC_DATA_DIR} has no marker-templates/ folder; is it the input data folder?")
endif ()

# Absolute paths baked into the binaries, so they run from any working directory:
# stage 1 reads its marker templates from TEMPLATE_DIR, and stage 2 appends its
# per-run regression row in OUTPUT_DIR (the build folder), never in the data folder.
add_definitions (-DTEMPLATE_DIR="${MSC_DATA_DIR}/marker-templates" -DOUTPUT_DIR="${CMAKE_BINARY_DIR}")

# One executable per pipeline stage; each source file has its own main().
add_executable (background-template-prototype src/01_background_template_prototype.cpp)
target_link_libraries (background-template-prototype ${OpenCV_LIBS})

add_executable (template-matching src/02_template_matching.cpp)
target_link_libraries (template-matching ${OpenCV_LIBS} ${GSL_LIBRARIES} ${Boost_LIBRARIES})

add_executable (interface-statistics src/03_interface_statistics.cpp)
target_link_libraries (interface-statistics ${OpenCV_LIBS} ${GSL_LIBRARIES} ${Boost_LIBRARIES})

add_executable (final-pipeline src/04_final_pipeline.cpp)
target_link_libraries (final-pipeline ${OpenCV_LIBS} ${GSL_LIBRARIES} ${Boost_LIBRARIES})
