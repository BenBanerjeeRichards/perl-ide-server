# Perl parser

Perl tokeniser and parser, for use in perl refactoring toools

```c++
    Tokeniser tokeniser("my $x = 20 unless $#ARGS < 5");
    auto tokens = tokeniser.tokenise();
    auto parseTree = parse(tokens, partiallyParsed);
    printParseTree(parseTree);
```
