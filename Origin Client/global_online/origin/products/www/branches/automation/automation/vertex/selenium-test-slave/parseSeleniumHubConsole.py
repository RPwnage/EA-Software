from bs4 import BeautifulSoup
import httplib
import re
import sys

selenium_url_base = 'origintool-002'
#selenium_url_base = 'eac-gavitron.eac.ad.ea.com'
selenium_url_port = 4444
selenium_url_console = '/grid/console'
grid_config_host_prefix = 'host:'
grid_agent_url_port = 8080
grid_agent_url_stop = '/stop'


def main(argv):
    conn = httplib.HTTPConnection(selenium_url_base, selenium_url_port)
    conn.request('GET', selenium_url_console)
    resp = conn.getresponse()
    if 200 == resp.status:
        html_doc = resp.read()
        soup = BeautifulSoup(html_doc, 'html.parser')
        #print(soup.prettify())
        for detail in soup.find_all('div', {'class':'content_detail', 'type':'config'}):
            #print(detail.prettify())
            for version in detail(text=re.compile('version=' + argv[0])):
            #for version in detail(text=re.compile(':eac-qe-one')):
                for host in version.parent.parent(text=re.compile(grid_config_host_prefix)):
                    grid_node_url_base = host[len(grid_config_host_prefix):]
                    print(grid_node_url_base)
                    conn = httplib.HTTPConnection(grid_node_url_base, grid_agent_url_port)
                    conn.request('GET', grid_agent_url_stop)
                    print(grid_node_url_base + ' status code: ' + str(conn.getresponse().status))
    else:
        print('WARNING: ' + selenium_console_url + ' returns ' + str(resp.status))
    
if __name__ == "__main__":
   main(sys.argv[1:])

