import json
import argparse


ID_LABEL = "identifier"
KIND_LABEL = "kind"
TARGET_KINDS = ["TypeReference", "TypeDeclaration"]


def should_modify_type_name(name):
    return name[0].islower()


def get_modified_type_name(name):
    return name.title()


def get_type_name_modifications(ast_json):
    type_name_modifications = dict()
    for type_declaration in ast_json["types"]:
        type_name = type_declaration[ID_LABEL]
        if should_modify_type_name(type_name):
            type_name_modifications[type_name] = get_modified_type_name(type_name)
    return type_name_modifications


def apply_type_name_modifications(ast_json, type_name_modifications):
    if isinstance(ast_json, dict):
        if ID_LABEL in ast_json and KIND_LABEL in ast_json:
            if ast_json[ID_LABEL] in type_name_modifications and ast_json[KIND_LABEL] in TARGET_KINDS:
                ast_json[ID_LABEL] = type_name_modifications[ast_json[ID_LABEL]]
        for item in ast_json.values():
            apply_type_name_modifications(item, type_name_modifications)
    elif isinstance(ast_json, list):
        for item in ast_json:
            apply_type_name_modifications(item, type_name_modifications)


parser = argparse.ArgumentParser(description="Adjust RG AST to compiler")
parser.add_argument("input", nargs=1, help="input JSON file with AST")
parser.add_argument("output", nargs=1, help="output JSON file with AST")

args = parser.parse_args()
input_file = args.input[0]
output_file = args.output[0]

with open(input_file, "r") as json_file:
    ast_json = json.load(json_file)

type_name_modifications = get_type_name_modifications(ast_json)
apply_type_name_modifications(ast_json, type_name_modifications)

with open(output_file, "w") as json_file:
    json.dump(ast_json, json_file, indent=4)
