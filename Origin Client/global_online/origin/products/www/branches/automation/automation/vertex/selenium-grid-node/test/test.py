#!/usr/bin/env python

import httplib
import json
import os
import subprocess
import time

"""
Unit test
"""
if __name__ == "__main__":
    agent = subprocess.Popen(['python', 'agent_main_local.py'])
    time.sleep(5)

    try:
        conn = httplib.HTTPConnection('localhost', 8080)

        # sysinfo/environ
        conn.request('GET', '/sysinfo/environ')
        environ = json.loads(conn.getresponse().read())['data']
        for key in os.environ.keys():
            if os.environ[key] != environ[key]:
                print('FAILED: ' + key)

        # directory/create
        conn.request('GET', '/directory/create?path=temp')
        if 'ok' != conn.getresponse().read():
            print('FAILED')

        # file/create
        conn.request('GET', '/file/create?path=temp/temp.txt&size=1000')
        if 'ok' != conn.getresponse().read():
            print('FAILED')

        # file/size
        conn.request('GET', '/file/size?path=temp/temp.txt')
        if 1000 != int(conn.getresponse().read()):
            print('FAILED')

        # file/copy
        conn.request('GET', '/file/copy?source=temp/temp.txt&dest=temp/tobedeleted.txt')
        if 'ok' != conn.getresponse().read():
            print('FAILED')

        # file/write
        conn.request('POST', '/file/write?path=temp/temp.txt', 'a', {})
        if 'ok' != conn.getresponse().read():
            print('FAILED')

        # file/move
        conn.request('GET', '/file/move?source=temp/temp.txt&dest=temp/tobedeleted.txt')
        if 'ok' != conn.getresponse().read():
            print('FAILED')

        # file/read
        conn.request('GET', '/file/read?path=temp/tobedeleted.txt')
        if 'a' != conn.getresponse().read():
            print('FAILED')

        # shell/async
        conn.request('POST', '/shell/sync?shell=True', 'echo 123', {})
        if '123\r\n' != conn.getresponse().read():
            print('FAILED')

        # shell/async
        conn.request('POST', '/shell/async', 'notepad', {})
        pid = conn.getresponse().read()
        time.sleep(1)

        # keyevent/type
        conn.request('POST', '/keyevent/type?delay=10', 'typing ...', {})
        if 'ok' != conn.getresponse().read():
            print('FAILED')

        # capture/fullscreen
        conn.request('GET', '/capture/fullscreen')
        with open('temp/alsotobedeleted.png', 'wb') as f:
            f.write(conn.getresponse().read())
        with open('temp/alsotobedeleted.png', 'rb') as f:
            if 'PNG' not in f.read(4):
                print("FAILED")

        # keyevent/multikey
        conn.request('GET', '/keyevent/multikey?combo=17+90')
        if 'ok' != conn.getresponse().read():
            print('FAILED')
        time.sleep(1)

        # shell/killproc
        conn.request('GET', '/shell/killproc?pid=' + pid)
        if ('SUCCESS: Sent termination signal to the process with PID ' + pid + '.\r\n') != conn.getresponse().read():
            print('FAILED')

        # directory/list
        conn.request('GET', '/directory/list?path=temp')
        if 0 != cmp(json.loads(conn.getresponse().read()), ['alsotobedeleted.png', 'tobedeleted.txt']):
            print('FAILED')

        # directory/search
        conn.request('POST', '/directory/search?path=temp', '["delete", "png"]', {})
        if '["alsotobedeleted.png"]' != conn.getresponse().read():
            print('FAILED')

        # file/delete
        conn.request('GET', '/file/delete?path=temp/alsotobedeleted.png')
        if 'ok' != conn.getresponse().read():
            print('FAILED')

        # file/exist
        conn.request('GET', '/file/exist?path=temp/alsotobedeleted.png')
        if 'False' != conn.getresponse().read():
            print('FAILED')

        # directory/delete
        conn.request('GET', '/directory/delete?path=temp')
        if 'ok' != conn.getresponse().read():
            print('FAILED')

        # directory/exist
        conn.request('GET', '/directory/exist?path=temp')
        if 'False' != conn.getresponse().read():
            print('FAILED')

        # util/clientstatus
        conn.request('GET', '/util/clientstatus')
        if 'True' != conn.getresponse().read():
            print('FAILED')

        # util/fullscreen
        conn.request('GET', '/util/fullscreen?id=0_node_agent_unit_test&name=pass.png')
        if 'ok' != conn.getresponse().read():
            print('FAILED')

        # util/upload
        conn.request('GET', '/util/upload?id=0_node_agent_unit_test')
        if 'ok' != conn.getresponse().read():
            print('FAILED')

        # util/enablesilentuninstall
        conn.request('GET', '/util/enablesilentuninstall?windowsProductCode=1A590C87-0B62-4CF7-9B3C-85789E577C03')
        if 'True' != conn.getresponse().read():
            print('FAILED')

    except Exception as e:
        print(e)
    finally:
        conn.request('GET', '/stop')
        print('stop agent: ' + conn.getresponse().read())

    agent.communicate()