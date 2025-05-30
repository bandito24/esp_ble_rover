file(GLOB_RECURSE srcs "main.c" "src/*.c")

idf_component_register(SRCS "${srcs}"
                       PRIV_REQUIRES bt nvs_flash esp_driver_gpio esp_driver_ledc
                       INCLUDE_DIRS "./include")
