cmake_minimum_required(VERSION 3.14)
include(cotire.cmake)

project(PerlParse C CXX)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(lib)

find_package(Threads REQUIRED)
add_executable(PerlParser src/main.cpp src/Tokeniser.cpp src/Tokeniser.h src/TokeniseException.h src/Util.h src/Util.cpp src/IOException.h src/Parser.cpp src/Parser.h src/VarAnalysis.cpp src/VarAnalysis.h src/FilePos.cpp src/FilePos.h src/Autocomplete.cpp src/Autocomplete.h src/Token.cpp src/Token.h src/Node.h src/PerlCommandLine.cpp src/PerlCommandLine.h src/PerlProject.cpp src/PerlProject.h lib/pstreams.h lib/httplib.h lib/json.hpp src/Test.cpp src/Test.h src/AutocompleteItem.h src/AutocompleteItem.cpp lib/httplib.h src/PerlServer.cpp src/PerlServer.h src/stdafx.h)
set_target_properties(PerlParser PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "src/stdafx.h")
cotire(PerlParser)

TARGET_LINK_LIBRARIES ( PerlParser ${CMAKE_THREAD_LIBS_INIT} )

