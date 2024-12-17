cmake_minimum_required(VERSION 3.10)
project(DeribitOEMS VERSION 0.0.1 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system thread program_options)
find_package(OpenSSL REQUIRED)

# Add the executable
add_executable(deribitOEMSBinary src/main2.cpp src/utils.cpp)

# Include directories
target_include_directories(deribitOEMSBinary
    PUBLIC src
    PUBLIC Header_Files
)
target_link_libraries(deribitOEMSBinary
    ${Boost_LIBRARIES}  # Boost libraries
    OpenSSL::SSL        # OpenSSL SSL library
    OpenSSL::Crypto     # OpenSSL Crypto library
    pthread             # Pthreads (required for Boost)
)

