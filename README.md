# Sublime PerlIDE Server
The server for the [Perl-IDE Sublime Project](https://github.com/BenBanerjeeRichards/perl-ide)

Server provides the following language analysis tools

- Autocomplete variables and subroutines
- Find variable and subroutine usages
- Goto symbol declaration
- Rename symbol

## Build

This project uses `cmake 3`. 

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