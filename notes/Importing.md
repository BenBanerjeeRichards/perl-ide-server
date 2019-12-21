## `use` and `require`

`require` is simple - it just revolves the perl file (against `@INC`) and runs the code. If 'A' requires 'B', A can access all of B's package global symbols through fully qualified names (`<Package>::`).

* Everything must be package qualified unless the packages match - 
* This means that if the packages match in both files, no qualification needed (e.g. if both in package `main`).

* `require 'Math/Calc.pm'` is identical to`require Math::Calc` - difference is just perl does path resolution against `@INC` for you.

* With the string syntax, the string can be any perl string - e.g `qq {test.pl}`. 
* You can do `require $file` - as it is run at runtime. **This analysis is not possible at static phase and so is not supported**. 
* You can `require <version>` for runtime equivalent for the `use <version>`. 

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

```perl
use v4.5;
use 4.5;
use 4.5_4;

use Math::Calc 12.34 qw{add sub};
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

