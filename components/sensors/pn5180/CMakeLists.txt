idf_component_register(
    SRCS "pn5180.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos log spi_manager
)