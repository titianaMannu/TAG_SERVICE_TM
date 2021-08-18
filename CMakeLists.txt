cmake_minimum_required(VERSION 3.20)
project(SOA_Project_TM C)

set(CMAKE_C_STANDARD 11)


# Find kernel headers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(KernelHeaders REQUIRED)

# find MODULE_LICENSE("GPL"), MODULE_AUTHOR() etc.
# thanks to "merseyviking" from stack overflow
add_definitions(-D __EXPORTED_HEADERS__ -D__KERNEL__ -DMODULE )

add_executable(SOA_Project_TM
        #main.c
        systbl_hack/systbl_hack_main.c
        systbl_hack/systbl_hack_service.c
        systbl_hack/systbl_hack.h
        systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c
        systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.h tag_service/tag_flags.h tag_service/tag_service.c
        )


# CLion IDE will find symbols from <linux/*>
target_include_directories("SOA_Project_TM" PRIVATE ${KERNELHEADERS_INCLUDE_DIRS})
