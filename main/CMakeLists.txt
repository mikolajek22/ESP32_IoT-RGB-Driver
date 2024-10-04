idf_component_register(SRCS "src/main.c" "src/fs.c"
                    INCLUDE_DIRS "src")
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)                    
# littlefs_create_partition_image(littlefs pages_html)    #upload HTML files to flash.