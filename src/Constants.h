//
// Created by Ben Banerjee-Richards on 04/01/2020.
//

#ifndef PERLPARSE_CONSTANTS_H
#define PERLPARSE_CONSTANTS_H

#include <string>
#include <vector>

namespace constant {
    const std::vector<std::string> SPECIAL_SCALARS = std::vector<std::string>{"$_", "$ARG", "$.", "$NR", "$/", "$RS",
                                                                              "$,",
                                                                              "$OFS", "$\\",
                                                                              "$ORS", "$\"", "$LIST_SEPARATOR", "$;",
                                                                              "$SUBSCRIPT_SEPARATOR", "$^L",
                                                                              "$FORMAT_FORMFEED", "$:",
                                                                              "$FORMAT_LINE_BREAK_CHARACTERS", "$^A",
                                                                              "$ACCUMULATOR", "$#",
                                                                              "$OFMT", "$?", "$CHILD_ERROR", "$!",
                                                                              "$OS_ERROR", "$@",
                                                                              "$EVAL_ERROR", "$$", "$PROCESS_ID", "$<",
                                                                              "$REAL_USER_ID",
                                                                              "$>", "$EFFECTIVE_USER_ID", "$(",
                                                                              "$REAL_GROUP_ID", "$)",
                                                                              "$EFFECTIVE_GROUP_ID", "$0",
                                                                              "$PROGRAM_NAME",
                                                                              "$[", "$]",
                                                                              "$PERL_VERSION", "$^D", "$DEBUGGING",
                                                                              "$^E",
                                                                              "$EXTENDED_OS_ERROR", "$^F",
                                                                              "$SYSTEM_FD_MAX",
                                                                              "$^H", "$^I",
                                                                              "$INPLACE_EDIT", "$^M", "$^O", "$OSNAME",
                                                                              "$^P",
                                                                              "$PERLDB",
                                                                              "$^T", "$BASETIME", "$^W", "$WARNING",
                                                                              "$^X",
                                                                              "$EXECUTABLE_NAME", "$ARGV", "$INC",
                                                                              "$ENV",
                                                                              "$SIG",
                                                                              "$&", "$MATCH", "$`", "$PREMATCH", "$'",
                                                                              "$POSTMATCH", "$+",
                                                                              "$LAST_PAREN_MATCH", "$|",
                                                                              "$OUTPUT_AUTOFLUSH",
                                                                              "$%",
                                                                              "$FORMAT_PAGE_NUMBER", "$=",
                                                                              "$FORMAT_LINES_PER_PAGE", "$-",
                                                                              "$FORMAT_LINES_LEFT", "$~",
                                                                              "$FORMAT_NAME",
                                                                              "$^",
                                                                              "$FORMAT_TOP_NAME"
    };

    const std::vector<std::string> SPECIAL_ARRAYS = std::vector<std::string>{"@ARGV", "@INC", "@F", "@ENV", "@ENV",
                                                                             "@SIG",
                                                                             "@_"};

    const std::vector<std::string> SPECIAL_HASHES = std::vector<std::string>{"%INC", "%ENV", "%SIG"};
    const std::vector<std::string> PRAGMATIC_MODULES = std::vector<std::string>{"attributes", "autodie",
                                                                                "autodie::exception",
                                                                                "autodie::exception::system",
                                                                                "autodie::hints",
                                                                                "autodie::skip", "autouse", "base",
                                                                                "bigint",
                                                                                "bignum", "bigrat", "blib", "bytes",
                                                                                "charnames",
                                                                                "constant", "deprecate", "diagnostics",
                                                                                "encoding", "encoding::warnings",
                                                                                "experimental",
                                                                                "feature", "fields", "filetest", "if",
                                                                                "integer",
                                                                                "less", "lib", "locale", "mro", "ok",
                                                                                "open",
                                                                                "ops", "overload", "overloading",
                                                                                "parent",
                                                                                "re",
                                                                                "sigtrapsort", "strict", "subs",
                                                                                "threads",
                                                                                "threads::shared", "utf8", "vars",
                                                                                "version",
                                                                                "vmsish", "warnings",
                                                                                "warnings::register"};
}

#endif //PERLPARSE_CONSTANTS_H
