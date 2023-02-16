
add_library(base STATIC
                LinkImpl.cpp
		Camera.hpp
		Camera.cpp
                Camera.h
                Camera.mm
		Example.hpp
		Example.cpp
                Example.h
                Example.mm)

set_target_properties(base PROPERTIES
	XCODE_GENERATE_SCHEME YES
	XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)

target_link_libraries(base PUBLIC 
     "-framework SDL2"
     "-framework Foundation"
     "-framework Metal"
     "-framework MetalKit"
     "-framework QuartzCore")
