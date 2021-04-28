#!/usr/bin/env python

import codecs
import httplib
import subprocess
import sys, os
sys.path.append('src')

from grid_node_rest_agent import app

##
# Entry point of Grid Node Agent service.
# Jenkins slave should use this entry point to register node to hub.
# Expects 2 paramters:
#   port: 8080
#   tag:  test run job id
# Example: >c:\Python27\Python.exe agent_main.py 8080 SeleniumGridProbeClient.123
#

# Set the configuration for this node 
def set_node_configuration(node_config):
    job_id = sys.argv[2] # tag for the job
    eax_grid_url = 'http://eax-automation:4446'
    node_config_json = """
        {
          "capabilities":
            [
              {
                "browserName": "chrome",
                "version": "__VERSION__",
                "maxInstances": 1,
                "platform": "WINDOWS",
                "seleniumProtocol": "WebDriver"
              },
              {
                "browserName": "firefox",
                "version": "__VERSION__",
                "maxInstances": 1,
                "platform": "WINDOWS",
                "seleniumProtocol": "WebDriver"
              },
              {
                "browserName": "htmlunit",
                "version": "__VERSION__",
                "maxInstances": 1,
                "platform": "WINDOWS",
                "seleniumProtocol": "WebDriver"
              }
            ],
          "proxy": "org.openqa.grid.selenium.proxy.DefaultRemoteProxy",
          "maxSession": 1,
          "register": true,
          "registerCycle": 5000,
          "hub": "__HUB__"
        }
    """
    
    os.chdir(os.path.dirname(os.path.abspath(__file__)) + '..')
    with codecs.open(node_config, 'w+', encoding='utf-8') as f:
        f.write(node_config_json.replace('__VERSION__', job_id).replace('__HUB__', eax_grid_url))
    
if __name__ == "__main__":
    print('sys.argv: ' + ' '.join(sys.argv))
    
    #os.chdir(os.path.dirname(os.path.abspath(__file__)) + '..')
    node_config = 'nodeConfig.json'
    set_node_configuration(node_config)
    
    # Set up locations of the drivers for execution
    chrome_driver = 'C:\\packages\\seleniumdriver\\chromedriver-2.38-win32.exe'
    firefox_driver = 'C:\\packages\\seleniumdriver\\geckodriver-v0.16.1-win64.exe'
    chrome_driver_param = '-Dwebdriver.chrome.driver=' + chrome_driver
    firefox_driver_param = '-Dwebdriver.gecko.driver=' + firefox_driver
    
    # Set the remainder of the parameters for execution
    selenium_standalone = "C:\\packages\\selenium-server-standalone\\selenium-server-standalone-3.12.0.jar"
    selenium_standalone_param = '-jar ' + selenium_standalone
    role_param = '-role node'
    servlet_param = '-servlet org.openqa.grid.web.servlet.LifecycleServlet'
    config_param = '-nodeConfig ' + node_config
    port_param = '-port 5555'

    print('Registering node to Selenium Hub, version: ' + sys.argv[2])
    command = 'java {0} {1} {2} {3} {4} {5} {6}'.format(
        chrome_driver_param,
        firefox_driver_param,
        selenium_standalone_param,
        role_param,
        servlet_param,
        config_param,
        port_param)
    print('Selenium Node command: ' + command)
    subprocess.Popen(command, shell=True)

    print('starting grid node agent ...')
    app.run()
    print('grid node agent stopped')

    grid_node_url_base = 'localhost'
    grid_node_url_port = 5555
    grid_node_url_shutdown = '/extra/LifecycleServlet?action=shutdown'

    print('shutting down Selenium standalone server ...')
    conn = httplib.HTTPConnection(grid_node_url_base, grid_node_url_port)
    conn.request('GET', grid_node_url_shutdown)
    print('Selenium standalone server returned status code: ' + str(conn.getresponse().status))
