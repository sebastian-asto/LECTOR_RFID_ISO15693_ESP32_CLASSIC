idf_component_register(
    SRCS 
    "main.c" 
    "app/sources/TPS61022_+5V.c"
    "app/rf/lector_rf.c"
    INCLUDE_DIRS "."
    REQUIRES spi_manager pn5180 i2c_manager
)