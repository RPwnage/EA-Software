#!/usr/bin/env python

import boto3
import os
import pyscreenshot as ImageGrab
import sys
import tempfile
import web
import _winreg

"""
Module for upload Origin data operation interface
"""
AWS_BUCKET = "client-automation-artifacts"
TEST_RESULTS_FOLDER = "test_case_result"

urls = (
    '/clientstatus', 'OriginClientStatus',
    '/fullscreen', 'FullScreen',
    '/enablesilentuninstall', 'EnableSilentUninstall',
    '/upload', 'Upload',
    '/windowsguid', 'WindowsGUID'
)

class OriginClientStatus:
    ## GET method to get Origin Client status.
    #
    # @result 'True' if Origin Client is ready for testing
    def GET(self):
        if os.environ['ProgramFiles(x86)']:
            OriginQtWebDriverDLL = os.path.join(os.environ['ProgramFiles(x86)'], 'Origin', 'plugins', 'OriginQtWebDriver', 'OriginQtWebDriver.dll')
        else:
            OriginQtWebDriverDLL = os.path.join(os.environ['ProgramFiles'], 'Origin', 'plugins', 'OriginQtWebDriver', 'OriginQtWebDriver.dll')
        return os.path.isfile(OriginQtWebDriverDLL)


class FullScreen:
    ## GET method to upload full screen image.
    #
    # @param id (mandatory) -- test result id. Example: <URL>?id=123456
    # @param name (mandatory) -- image name. Example: <URL>?name=pass.png
    # @result 'ok' if succeed
    def GET(self):
        try:
            tId = web.input().id
            name = web.input().name
            try:
                session = boto3.Session()
                client = session.client('s3')
                hostname = os.environ['COMPUTERNAME']

                # get full screen
                temp = tempfile.NamedTemporaryFile(delete=False)
                im = ImageGrab.grab()
                im.save(temp, 'png')

                client.upload_file(temp.name, AWS_BUCKET, TEST_RESULTS_FOLDER + '/' + tId + '/' + hostname + '_' + name)

                # remove temp file
                temp.close()
                os.unlink(temp.name)
                
                return 'ok'
            except Exception as e:
                print(e)
                return web.internalerror()
        except:
            return web.badrequest()


class Upload:
    ## GET method to upload test data.
    #
    # @param id (mandatory) -- test result id. Example: <URL>?id=123456
    # @result 'ok' if succeed
    def GET(self):
        try:
            tId = web.input().id
            try:
                session = boto3.Session()
                client = session.client('s3')
                hostname = os.environ['COMPUTERNAME']

                # Upload EACore
                if os.environ['ProgramFiles(x86)']:
                    eacore = os.path.join(os.environ['ProgramFiles(x86)'], 'Origin', 'EACore.ini')
                else:
                    eacore = os.path.join(os.environ['ProgramFiles'], 'Origin', 'EACore.ini')
                client.upload_file(eacore, AWS_BUCKET, TEST_RESULTS_FOLDER + '/' + tId + '/' + hostname + '_' + 'EACore.ini')

                # Upload All files in C:\ProgramData\Origin\Logs except those with IGOProxy in their name
                logsDir = os.path.join(os.environ['ProgramData'], 'origin', 'Logs')
                names = (file for file in os.listdir(logsDir) if os.path.isfile(os.path.join(logsDir, file)))
                for name in names:
                    if 'IGOProxy' not in name:
                        path = os.path.join(logsDir, name)
                        client.upload_file(path, AWS_BUCKET, TEST_RESULTS_FOLDER + '/' + tId + '/' + hostname + '_' + name)

                return 'ok'
            except Exception as e:
                print(e)
                return web.internalerror()
        except:
            return web.badrequest()


class EnableSilentUninstall:
    ## GET method to enable silent uninstall for the entitlement.
    #
    # @param windowsProductCode (mandatory) -- GUID or Globally Unique Identifier. Example: <URL>?windowsProductCode=1A590C87-0B62-4CF7-9B3C-85789E577C03
    # @result 'True' if enable successful
    def GET(self):
        try:
            windowsProductCode = web.input().windowsProductCode
            try:
                try:
                    game_registry_key = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{' + windowsProductCode + '}', 0, _winreg.KEY_ALL_ACCESS)
                except WindowsError, e:
                    game_registry_key = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{' + windowsProductCode + '}', 0, _winreg.KEY_ALL_ACCESS)
                
                uninstall_value = _winreg.QueryValueEx(game_registry_key, 'UninstallString')
                uninstall_game_path = uninstall_value[0]
                
                if "quiet" not in uninstall_game_path:
                    _winreg.SetValueEx(game_registry_key, 'UninstallString', 0, _winreg.REG_SZ, uninstall_game_path + ' -quiet')
                else:
                    return False
                
                _winreg.CloseKey(game_registry_key)

                return True
            except WindowsError, e:
                print(e)
                return web.internalerror()
        except:
            return web.badrequest()

class WindowsGUID:
    ## GET method to get all the registry keys/GUIDs as an array
    #
    # @result The array of registry keys/GUIDs
    def GET(self):
        try:
            hKey = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE,
                                   "Software\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall")
            i = 0
            arr = []
            while True:
                try:
                    key = _winreg.EnumKey(hKey, i)
                    arr.append(key)
                    i += 1
                except WindowsError, e:
                    print(e)
                    break
            return arr
        except:
            return web.badrequest()


utilop = web.application(urls, locals())