# RG compiler

Regular Games to C++ compiler


### Requirements

* install [nlohman JSON C++ parser](https://github.com/nlohmann/json)
* install [boost](http://www.boost.org) libraries:
    * [program options](http://www.boost.org/libs/program_options/) (*libboost-program-options-dev*)
    * [container](http://www.boost.org/libs/container/) (*libboost-container-dev*)
* optional: install [clang-format](https://clang.llvm.org/docs/ClangFormat.html)

### Compilation
```
./scripts/rebuild.py
```

### Usage

* Translate game description to C++
```
./scripts/compile.py game
```
* Random simulations
```
./scripts/sims.py game limit
```
* Perft
```
./scripts/perft.py game depth

```

Use `-h` flag for detailed description about usage of specific script.

All generated files and compiled binaries are placed in `build-test` directory.
