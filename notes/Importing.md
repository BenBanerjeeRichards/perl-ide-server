## `use` and `require`

### require

`require` is simple - it just revolves the perl file (against `@INC`) and runs the code. If 'A' requires 'B', A can access all of B's package global symbols through fully qualified names (`<Package>::`).

* Everything must be package qualified unless the packages match - 
* This means that if the packages match in both files, no qualification needed (e.g. if both in package `main`).

* `require 'Math/Calc.pm'` is identical to`require Math::Calc` - difference is just perl does path resolution against `@INC` for you. **Module syntax in both require and use assume .pm extension**

* With the string syntax, the string can be any perl string - e.g `qq {test.pl}`. 
* You can do `require $file` - as it is run at runtime. **This analysis is not possible at static phase and so is not supported**. 
* You can `require <version>` for runtime equivalent for the `use <version>`. 
* Require is done at run time, so it does not take effect until it has been run. **We can't support this fully!** - we will simply by simply scanning entire file and treating the same as use. 

### use

`use` does everything `require` does*, but then calls `B->import()`. This function could do anything, but typically places some specific symbols from `B::` into the current namespace of `A`.  (\*  = you can only use package names, not file strings. **use is processed statically**). 

* Use only supports package syntax
* Use supports additions string at the end for exporting to current namespace

```perl
use Math::Calc;
use Math::Calc 'add sub';
use Math::Calc qq{add &sub $variable};
```

* Some are 'pragmatic' and have specific meaning - they don't refer to a file. e.g. `use feature say`. See below.
* Version can be included to require perl version
* `use <Module>` looks up module via file path, but package name doesn't have to match/can contain multiple packages that can each be references
* Doesn't matter where the use statement is put - position, lexical scope,... As it's handled statically by perl.

```perl
use v4.5;
use 4.5;
use 4.5_4;

use Math::Calc 12.34 qw{add sub};
```



### Handling imports

Now we have the imports parsed and resolved, need to figure out how we will walk through them to parse everything.

This is a recursive process: 

```pseudocode
FILE_SYMBOLS : List[FileSymbol] = []

function loadSymbols(path) {
  // Important to prevent infinite recursion in cyclic imports (which are allowed in Perl)
	if (newPath in FILE_SYMBOLS) return;

	// Get file symbols, specific only to this file
	FileSymbols fileSymbols = getFileSymbols(path)
	FILE_SYMBOLS[path] = fileSymbols
	
	// Consider imports
	for (import in fileSymbols.imports) {
		newPath = resolveImport(import)
		loadSymbols(importPath)
	}
}
```

Now let's ignore exported methods for now.  We only care about the following in the imported files:

* Sub routines (all package level by default)
* Package variables (our, explicit $Package::X = ...)

Also note: importing is transitive! So we then collect all package global stuff from every element in `FILE_SYMBOLS` into a symbol table (this includes the ones corresponding to file), and then add the lexical stuff from the file's symbol table. 

There's a clear optimisation here that can be done - no need to process local variables (and definitely not store them in memory), but that's for another day. Also in practise we'll only parse the perl libraries once and cache them in a file (well they'll be rebuild when the module is updated, but that is something else for the future).



#### Data structure for holding symbols from multiple files

As already stated, we need to store package level sub routines and variables from other files, as well as all local symbols from the current file. 

```c++
// as before
struct Global {
  FilePos location;
  std::string package;
  std::string name;
  std::string sigil;
}

struct Symbols {
  // Each global with it's usages. usages cross over files
  map<Global, vector<FilePos>> globals;
  
}
```



## Pragmatic modules

```
attributes
autodie
autodie::exception
autodie::exception::system
autodie::hints
autodie::skip
autouse
base
bigint
bignum
bigrat
blib
bytes
charnames
constant
deprecate
diagnostics
encoding
encoding::warnings
experimental
feature
fields
filetest
if
integer
less
lib
locale
mro
ok
open
ops
overload
overloading
parent
re
sigtrapsort
strict
subs
threads
threads::shared
utf8
vars
version
vmsish
warnings
warnings::register
```

