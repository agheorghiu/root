############################################################################
# CMakeLists.txt file for building ROOT core/pack package
############################################################################

ROOT_GLOB_SOURCES(sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx)

ROOT_GENERATE_DICTIONARY(G__Pack *.h LINKDEF LinkDef.h)
set(PACK_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/G__Pack.cxx  PARENT_SCOPE)

set(LibCore_SRCS ${LibCore_SRCS} ${sources} PARENT_SCOPE)
set(LibCore_LINKDEF ${LibCore_LINKDEF} ${CMAKE_CURRENT_SOURCE_DIR}/inc/LinkDef.h PARENT_SCOPE)
add_custom_target(PACK_DICTIONARY DEPENDS G__Pack.cxx)
set_target_properties(PACK_DICTIONARY PROPERTIES FOLDER Dictionaries)

ROOT_INSTALL_HEADERS()
