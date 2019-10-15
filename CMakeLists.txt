cmake_minimum_required(VERSION 3.14)
project(PerlParser)

set(CMAKE_CXX_STANDARD 17)

add_executable(PerlParser main.cpp Tokeniser.cpp Tokeniser.h TokeniseException.h Util.h Util.cpp IOException.h Parser.cpp Parser.h VarAnalysis.cpp VarAnalysis.h FilePos.cpp FilePos.h Autocomplete.cpp Autocomplete.h Token.cpp Token.h Node.h)
add_executable(PerlParserTokeniserTest TestMain.cpp Util.h Util.cpp Tokeniser.h Tokeniser.cpp IOException.h)
