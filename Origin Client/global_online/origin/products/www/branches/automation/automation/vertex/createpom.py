'''
Created on Dec 12, 2016
Adjusts the origin_client pom to contain the necessary information to start the
tests. This will adjust the variables injected into the build, which will be
pushed to the subjobs.

@author: glivingstone
'''
import os
from xml.dom import minidom

branch = os.environ['VERTEX_BRANCH']
CURRENT_DIR = os.path.join(os.environ['WORKSPACE'], 'origin/support/QETools/qtwebdriver/poc/' + branch + '/master')
ENVIRONMENT_VARS = ('TEST_ENVIRONMENT', 'CMSSTAGE', 'SPA_VERSION', 'SANDBOX_ADDRESS', 'JOB_ID')

# Write the Environment variables that are needed to the test.
def setEnvironmentVariables(base_doc, properties):
    for variable in ENVIRONMENT_VARS:
        if variable in os.environ:
            element = properties.getElementsByTagName('Env.' + variable)[0]
            properties.removeChild(element)
            new_property = base_doc.createElement('Env.' + variable)
            new_property_text = base_doc.createTextNode(os.environ[variable])
            new_property.appendChild(new_property_text)
            properties.appendChild(new_property)

# Write the testNGSuiteXML to the file. Will determine which profile under Suites to run.
def setTestNG(base_doc, properties):
    element = properties.getElementsByTagName('testNGSuiteXML')[0]
    properties.removeChild(element)
    new_property = base_doc.createElement('testNGSuiteXML')
    new_property_text = base_doc.createTextNode(os.environ['TEST_SUITE'] + '.xml')
    new_property.appendChild(new_property_text)
    properties.appendChild(new_property)
        
def main():
    # Open the default file and read as an xml
    default_filename = os.path.join(CURRENT_DIR, 'nbactions-origin_client_default.xml')
    default_file = open(default_filename, 'r')
    file_doc = minidom.parse(default_file)
    
    # Get the properties, which we are going to edit based on the job.
    properties = file_doc.getElementsByTagName('actions')[0].getElementsByTagName('action')[0].getElementsByTagName('properties')[0]
    default_file.close()
    
    # Set environment properties
    setEnvironmentVariables(file_doc, properties)
    setTestNG(file_doc, properties)
    
    # Write to the new file which will be used to trigger the test
    new_filename = os.path.join(CURRENT_DIR, 'nbactions-origin_client.xml')  
    new_file = open(new_filename, 'w')
    new_file.write(file_doc.toxml())
    new_file.close()

if __name__ == '__main__':
    main()