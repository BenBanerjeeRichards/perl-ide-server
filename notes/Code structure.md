# Post parsing analysis

Currently this is a mess - a file called `VarAnalysis.cpp` contains a function called `doFindVariableDeclarations` which along with finding variable declarations also finds subroutines and very basic `use` statements (this will be expanded).

It also does two passes - one for finding variable declarations and then a second one for variable usages. The idea here is declarations should be found first to allow the usage finder to resolve to the correct declaration. Though in practise variables are declared before they use, so this mechanism is pointless. 

I think the usage resolution (including package name canonicalisation) should be put in a different file to the token traversal. The basic token traversal should only be reliant on the tokens, not any existing analysis. If existing analysis is needing in the future, we'll add that as a second pass.

This means we'll need an intermediate data structure for storing variables - the `Variable` (and derived classes) are find, but of course the usage map can not be found until later. 

The `SymbolNode` structure is mostly good - is just represents scoped objects (lexically scoped variables + use features) (technically some local subroutines should be here, but that is a feature seen rarely in the wild and so omitted for now). 

However, `SymbolNode::variables` only contains declarations. We should modify it to something like

```c++
struct SymbolNode {
  //... (boring stuff like positions, children)
  
  // Important: These following for variabls are disjoint sets - a usage can not be a declaration
  std::vector<std::shared_ptr<Variable>> variableDeclarations;
  std::vector<std::shared_ptr<Variable>> variableUsages;
  
  // as before
  std::vector<std::string> features; 
}
```



Here's an issue - how do we know the variable type without usages. 

OK this is going to be a mess. Instead move out sub/feature stuff out of this file into another file, then do a third pass. After that we'll look into removing the second pass (variable usages) from VarAnalysis. 

OR - still do two passes, but move first pass into separate file. The variable declaration doesn't care about any of this usage stuff. Will need to move lots of structs/classes into their own header files (should help compilation times too). 