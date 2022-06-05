# RGcompiler

Regular Games to C++ compiler


### Requirements

* clone [RG interpreter](https://github.com/radekmie/rg)
* install [nlohman JSON C++ parser](https://github.com/nlohmann/json)
* optional: install [clang-format](https://clang.llvm.org/docs/ClangFormat.html)

### Compilation
```
mkdir build
cd build
cmake ..
make rg2cpp
```

### Usage

* Use interpreter to create AST in json format
```
node lib [game].rg print-ast > [game]-ast.json
```
* Use generated file as input to rg2cpp compiler
```
./build/rg2cpp [game]-ast.json
```
* Run `clang-format` on generated file
```
clang-format -style="{BasedOnStyle: Google, IndentWidth: 4}" -i reasoner.hpp
```
