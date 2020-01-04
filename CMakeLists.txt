cmake_minimum_required(VERSION 3.14)

project(PerlParse C CXX)

# Use ccache if installed - significantly improves clean compilation times
find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(lib)

find_package(Threads REQUIRED)
add_executable(PerlParser src/main.cpp src/Tokeniser.cpp src/Tokeniser.h src/TokeniseException.h src/Util.h src/Util.cpp src/IOException.h src/Parser.cpp src/Parser.h src/VarAnalysis.cpp src/VarAnalysis.h src/FilePos.cpp src/FilePos.h src/Token.cpp src/Token.h src/Node.h src/PerlCommandLine.cpp src/PerlCommandLine.h src/PerlProject.cpp src/PerlProject.h lib/pstreams.h lib/httplib.h src/Test.cpp src/Test.h src/AutocompleteItem.h src/AutocompleteItem.cpp lib/httplib.h src/PerlServer.cpp src/PerlServer.h src/FileAnalysis.cpp src/FileAnalysis.h src/Variable.h src/Variable.cpp src/Subroutine.cpp src/Subroutine.h src/Symbols.cpp src/Symbols.h src/Package.h src/Package.cpp src/SymbolLoader.cpp src/SymbolLoader.h src/Cache.cpp src/Cache.h lib/md5.h lib/md5.c)

TARGET_LINK_LIBRARIES ( PerlParser ${CMAKE_THREAD_LIBS_INIT} )

