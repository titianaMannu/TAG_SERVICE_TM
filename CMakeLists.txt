cmake_minimum_required(VERSION 3.20)
project(SOA_Project_TM C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

# Find kernel headers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(KernelHeaders REQUIRED)

# find MODULE_LICENSE("GPL"), MODULE_AUTHOR() etc.
# thanks to "merseyviking" from stack overflow
add_definitions(-D __EXPORTED_HEADERS__ -D__KERNEL__ -DMODULE )

add_executable(SOA_Project_TM
        tag_service/systbl_hack/systbl_hack_main.c
        tag_service/systbl_hack/systbl_hack_service.c
        tag_service/systbl_hack/systbl_hack.h
        tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.c
        tag_service/systbl_hack/memory-mapper/virtual-to-phisical-memory-mapper.h
        tag_service/tag_flags.h tag_service/tag.c
        tag_service/tag_main.c
        user/tag-interface.c user/tag-interface.h
        user/user1.c
        user/user2.c
        user/awake.c
        user/remove.c
        tag_service/device-driver/tag_dev.c
        tag_service/device-driver/tag_dev.h)


# CLion IDE will find symbols from <linux/*>
target_include_directories("SOA_Project_TM" PRIVATE ${KERNELHEADERS_INCLUDE_DIRS})
