#!/bin/bash
# usage: compile.sh [game]

set -e
set -o pipefail

game=$1

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd ${DIR}

# create AST
cd ../../rg/interpreter_node/
node lib ../examples/${game}.rg print-ast > ../../RGcompiler/${game}-ast.json


# generate cpp files
cd ${DIR}
cd ..
./build/rg2cpp ${game}-ast.json


# format generated files
clang-format -style="{BasedOnStyle: Google, IndentWidth: 4}" -i reasoner.hpp
