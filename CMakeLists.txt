cmake_minimum_required(VERSION 3.14)
project(PerlParser)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(lib)

add_executable(PerlParser src/main.cpp src/Tokeniser.cpp src/Tokeniser.h src/TokeniseException.h src/Util.h src/Util.cpp src/IOException.h src/Parser.cpp src/Parser.h src/VarAnalysis.cpp src/VarAnalysis.h src/FilePos.cpp src/FilePos.h src/Autocomplete.cpp src/Autocomplete.h src/Token.cpp src/Token.h src/Node.h src/PerlCommandLine.cpp src/PerlCommandLine.h src/PerlProject.cpp src/PerlProject.h lib/pstreams.h src/Test.cpp src/Test.h)
