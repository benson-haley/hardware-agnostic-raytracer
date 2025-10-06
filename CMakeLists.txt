# Specify the minimum version of CMAKE (a modern enough version to allow modules).
cmake_minimum_required(VERSION 3.28)

# Set the compilers.  You're not supposed to do this but I couldn't care less.
set(CMAKE_C_COMPILER "clang-18")
set(CMAKE_CXX_COMPILER "clang++-18")

set(CMAKE_VERBOSE_MAKEFILE ON)

# Specify the name of the project.
project("gi")

# Find AdaptiveCpp.
find_package(AdaptiveCpp CONFIG REQUIRED)

# Obtain the source files from the src directory.
file(GLOB_RECURSE source_files CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Set the ACPP debug level.
# 4 = verbose
# 3 = errors, warnings, & info
# 2 = errors & warnings
# 1 = errors
# 0 = nothing
add_definitions(-DHIPSYCL_DEBUG_LEVEL=4)

# Tell CMAKE that I want an executable.
add_executable("${PROJECT_NAME}" ${source_files})

# My executable should use SYCL.
add_sycl_to_target(TARGET "${PROJECT_NAME}" SOURCES ${source_files})
install(TARGETS "${PROJECT_NAME}" RUNTIME DESTINATION ../install)

# Set the C++ standard to 2023.
target_compile_features("${PROJECT_NAME}" PRIVATE cxx_std_23)
# Optimize as much as possible.
target_compile_options("${PROJECT_NAME}" PRIVATE -O3)
# Force AdaptiveCpp to compile CUDA code.
target_compile_options("${PROJECT_NAME}" PRIVATE
    --acpp-targets=cuda:sm_89
    --acpp-cuda-path=/usr/local/cuda
    --cuda-path=/usr/local/cuda
    --cuda-gpu-arch=sm_89
    -march=native
)
add_definitions(-DWITH_CUDA_BACKEND=ON)
add_definitions(-DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda) # TODO: We used these when building ACPP, do we need this?

set(CUDA_INCLUDE_DIRS "/usr/local/cuda/include")
set(CUDA_LIBRARIES "/usr/local/cuda/lib64/libcudart.so")
include_directories(${CUDA_INCLUDE_DIRS})
target_link_libraries("${PROJECT_NAME}" PRIVATE ${CUDA_LIBRARIES})

#target_compile_options("${PROJECT_NAME}" PRIVATE -fno-exceptions)

# Fetch Eigen.
include(FetchContent)
FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG master
)
FetchContent_MakeAvailable(eigen)
target_link_libraries("${PROJECT_NAME}" PUBLIC Eigen3::Eigen)


########################################################################################################################
#
# cd build
# sudo cmake .. -DAdaptiveCpp_DIR="/home/bah/AdaptiveCpp/build/install/lib/cmake/AdaptiveCpp"
# sudo make
# ./gi
#
########################################################################################################################