#!/bin/bash
# usage: compile.sh [game]

set -e
set -o pipefail

game=$1

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd ${DIR}

# create AST
cd ../../rg/interpreter_node/
node lib/cli --expandGeneratorNodes rg-ast ../examples/${game}.rg > ../../RGcompiler/${game}.json
cd ../../RGcompiler/
python3 -m json.tool ${game}.json > ${game}-ast.json
rm ${game}.json

# generate cpp files
./build/rg2cpp ${game}-ast.json


# format generated files
clang-format -style="{BasedOnStyle: Google, IndentWidth: 4}" -i reasoner.hpp
