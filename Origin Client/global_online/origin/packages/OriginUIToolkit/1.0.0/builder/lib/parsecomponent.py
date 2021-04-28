import json

def parse_component_depends(filelike):
    component_config = json.load(filelike)
    depend_set = set()

    for depend in component_config['dependencies']:
        depend_set.add(depend)

    return depend_set
