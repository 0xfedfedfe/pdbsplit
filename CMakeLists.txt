cmake_minimum_required(VERSION 3.22)
project(pdbsplit)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external/raw_pdb)
include_directories(${CMAKE_SOURCE_DIR}/external/COFFI) 

add_subdirectory(pdbsplit)
