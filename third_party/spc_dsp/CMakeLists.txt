# Isolated LGPL DSP hardware dependency. It contains no SPC/65C816 CPU sources.
add_library(unirally_dsp SHARED adapter.cpp)
target_compile_definitions(unirally_dsp PRIVATE UNIRALLY_DSP_BUILD)
target_include_directories(unirally_dsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(unirally_dsp PROPERTIES
    CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES
    VERSION 1.0.0 SOVERSION 1)
# Preserve upstream warnings and integer behavior. Apply the same sanitizers
# as the native callers, but not the project's stricter conversion diagnostics.
if(LAB_SANITIZERS AND NOT MSVC)
    target_compile_options(unirally_dsp PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(unirally_dsp PRIVATE -fsanitize=address,undefined)
endif()
