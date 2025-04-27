idf_component_register(SRCS "src/main.c" 
                            "src/fs.c"
                            "src/http_server.c" 
                            "src/rgbController.cpp" 
                            "src/startup.c" 
                            "src/http_handlers.c" 
                            "src/sntp_sync.c" 
                            "src/logs.c" 
                            "src/diagnostic.c" 
                            "src/i2c_bus.c" 
                            "src/SSD1306.cpp"  
                            "src/ssd1306_graphic/gfx.cpp"  
                            "src/ssd1306_graphic/oled_controller.cpp"              
                            INCLUDE_DIRS "src")
include_directories(${CMAKE_CURRENT_LIST_DIR}/include
                    ${CMAKE_CURRENT_LIST_DIR}/include/ssd1306_graphic)                    
# littlefs_create_partition_image(littlefs pages_html)    #upload HTML files to flash.