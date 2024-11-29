idf_component_register(SRCS "src/main.c" "src/fs.c" "src/http_server.c" "src/rgbController.cpp" "src/startup.c" "src/http_handlers.c"
                    INCLUDE_DIRS "src")
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)                    
# littlefs_create_partition_image(littlefs pages_html)    #upload HTML files to flash.