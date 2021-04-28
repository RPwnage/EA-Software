import os
import shutil

_JS_DIRECTORY = 'scripts'
_CSS_DIRECTORY = 'styles'
_IMAGES_DIRECTORY = 'images'

class AggregateDirExistsError(Exception):
    def __init__(self, aggregate_path):
        self.aggregate_path = aggregate_path

    def __str__(self):
        return 'Aggregate output directory already exists: ' + self.aggregate_path

def _aggregate_source_files(path_list):
    aggregate_source = ''

    for source_path in path_list:
        with open(source_path, 'r') as f:
            aggregate_source += f.read()

    return aggregate_source

def _source_paths(base_dir, extension, components):
    paths = []

    for component_name in components:
        # Look for support files in the component directory
        # This is /{scripts,styles}/{component}/**/*.{js,css}
        support_dir = os.path.join(base_dir, component_name)

        for dirpath, _, filenames in os.walk(support_dir):
            for filename in filenames:
                if filename.endswith(extension):
                    paths.append(os.path.join(dirpath, filename))

        # Look for the base file - /{component}.{js,css}
        basefile_path = os.path.join(base_dir, component_name + extension)

        if os.path.exists(basefile_path):
            paths.append(basefile_path)

    return paths

def aggregate_components(components, components_dir, output_dir, output_base_name):
    agg_base_dir = os.path.join(output_dir, output_base_name)
    
    if os.path.exists(agg_base_dir):
        raise AggregateDirExistsError(agg_base_dir)

    # Make our ouput directories
    agg_js_dir = os.path.join(agg_base_dir, _JS_DIRECTORY) 
    agg_css_dir = os.path.join(agg_base_dir, _CSS_DIRECTORY) 
    agg_images_dir = os.path.join(agg_base_dir, _IMAGES_DIRECTORY) 

    for directory in [agg_base_dir, agg_js_dir, agg_css_dir, agg_images_dir]:
        os.mkdir(directory)

    # Aggregate all the JavaScript source
    components_js_dir = os.path.join(components_dir, _JS_DIRECTORY)
    js_paths = _source_paths(components_js_dir, '.js', components)
    js_source = _aggregate_source_files(js_paths)

    # And the CSS source
    components_css_dir = os.path.join(components_dir, _CSS_DIRECTORY)
    css_paths = _source_paths(components_css_dir, '.css', components)
    css_source = _aggregate_source_files(css_paths)

    # Write them both out
    js_agg_filename = output_base_name + '.js'
    with open(os.path.join(agg_js_dir, js_agg_filename), 'w') as f:
        f.write(js_source)
    
    css_agg_filename = output_base_name + '.css'
    with open(os.path.join(agg_css_dir, css_agg_filename), 'w') as f:
        f.write(css_source) 

    # Copy over any image directories that exist
    components_images_dir = os.path.join(components_dir, _IMAGES_DIRECTORY)

    for component_name in components:
        source_dir = os.path.join(components_dir, _IMAGES_DIRECTORY, component_name)
        dest_dir = os.path.join(agg_images_dir, component_name)

        if os.path.isdir(source_dir):
            shutil.copytree(source_dir, dest_dir) 
