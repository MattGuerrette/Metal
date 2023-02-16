
add_library(base STATIC
		Camera.h
		Camera.mm
		Example.h
		Example.mm)

set_target_properties(base PROPERTIES
	XCODE_GENERATE_SCHEME YES
	XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)

target_link_libraries(base PUBLIC 
     "-framework SDL2"
     "-framework Metal"
     "-framework MetalKit")
