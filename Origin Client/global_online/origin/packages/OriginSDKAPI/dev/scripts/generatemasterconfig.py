import os
import xml.etree.ElementTree as ET
import copy 
def main():
    masterconfig = ET.parse(os.path.join(os.path.dirname(os.path.dirname(__file__)),'masterconfig.xml'))
    masterconfig_versions = ET.parse(os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(__file__)))),'masterconfig_versions.xml'))
    output_filename = os.path.join(os.path.dirname(os.path.dirname(__file__)),'masterconfig_generated.xml')
    for child in masterconfig.getroot():
        if child.tag == 'fragments':
            masterconfig.getroot().remove(child)
    
    for child in masterconfig_versions.getroot():
        if child.tag == "globalproperties":
            masterconfig.find("./globalproperties").text += child.text
        else:
            masterconfig.getroot().append(child)
        
    for child in masterconfig_versions.getroot().findall('./packageroots/packageroot'):
        child.text = "../../{0}".format(child.text) #masterconfig versions is 2 directories up so add two ../ to package roots
        
    for child in masterconfig.getroot().findall('./masterversions/package'):
        if 'PACKAGE_VISUALSTUDIO' in os.environ and child.attrib['name'] == 'VisualStudio':
            child.attrib['version'] = os.environ['PACKAGE_VISUALSTUDIO']
        if 'PACKAGE_EATHREAD' in os.environ and child.attrib['name'] == 'EAThread':
            child.attrib['version'] = os.environ['PACKAGE_EATHREAD']
        if 'TOOLSET_VISUALSTUDIO' in os.environ and child.attrib['name'] == 'VisualStudio':
            if os.environ['TOOLSET_VISUALSTUDIO'] == 'v110':
                child.attrib['version'] = '12.0.31101-proxy'
            if os.environ['TOOLSET_VISUALSTUDIO'] == 'v110':
                child.attrib['version'] = '11.0.50727-4-proxy'
            if os.environ['TOOLSET_VISUALSTUDIO'] == 'v100':
                child.attrib['version'] = '10.0.40219-2-sp1'                
            child.attrib['version']

    masterconfig.write(open(output_filename,'w'))
    

if __name__ == '__main__':
    main()