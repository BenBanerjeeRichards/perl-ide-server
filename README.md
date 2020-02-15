# Sublime PerlIDE Server
The server for the [Perl-IDE Sublime Project](https://github.com/BenBanerjeeRichards/perl-ide)

Server provides the following language analysis tools

- Autocomplete variables and subroutines
- Find variable and subroutine usages
- Goto symbol declaration
- Rename symbol

## Build

This project uses cmake 3. You need to use a sufficiently modern C++ compiler for the build to succeed - any compiler supporting `<optional>` should work. To change the compiler used by cmake, set the `CXX` environment variable before invoking cmake. 

```shell script
git clone github.com/benbanerjeerichards/Perl-Ide-Server
cd Perl-Ide-Server
mkdir build && cd build
cmake ../
make
``` 

## Run

```shell script
PerlParser serve
```