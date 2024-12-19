if(TARGET opencv::opencv_world AND NOT TARGET opencv_world)
    add_library(opencv_world INTERFACE IMPORTED)
    set_property(TARGET opencv_world PROPERTY INTERFACE_LINK_LIBRARIES opencv::opencv_world)
endif()
