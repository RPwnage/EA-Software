import os

from parsecomponent import parse_component_depends

class ComponentNotFoundError(Exception):
    def __init__(self, includer_name, component_name):
        self.includer_name = includer_name
        self.component_name = component_name

    def __str__(self):
        return 'Component "' + self.component_name + '" not found while processing "' + self.includer_name + '"'

def _make_list_unique(input_list):
    """Remove duplicate items from the passed list"""
    seen_items = set()
    output_list = []

    for item in input_list:
        if item not in seen_items:
            seen_items.add(item)
            output_list.append(item)

    return output_list


def resolve_depends(includer_name, depend_set, component_dir):
    component_list = []

    # Go over the depends in lexographic order
    # This means our output is stable across runs and makes unit testing easier 
    for component_name in sorted(depend_set):
        # Load the component information
        component_file = os.path.join(component_dir, component_name + '.json')

        try:
            with open(component_file, 'r') as f:
                component_depends = parse_component_depends(f)
        except IOError:
            raise ComponentNotFoundError(includer_name, component_name)
        
        # Recursively resolve this component's dependencies
        recursive_components = resolve_depends(component_name, component_depends, component_dir)

        # Make sure the dependency components are at the head of the list
        duped_component_list = recursive_components + component_list + [component_name]
        component_list = _make_list_unique(duped_component_list)

    return component_list
