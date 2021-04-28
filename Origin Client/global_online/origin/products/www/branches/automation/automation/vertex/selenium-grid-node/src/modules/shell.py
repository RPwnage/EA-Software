import subprocess
import sys
import web
import time

"""
Module for Shell Command operation interface
"""

urls = (
    '/sync', 'Sync',
    '/async', 'Async',
    '/killproc', 'Sync'
)


class Sync:
    ## POST method to execute shell command synchronically on agent local.
    #
    # @param cwd (optional) -- path of working directory. Example: <URL>?cwd=c:\windows\system32
    # @param shell (optional) -- for executing shell commands, False by default. Example: <URL>?shell=True
    # @param POST message body -- complete shell command to be executed. Example: tasklist /NH
    # @result command execution result
    def POST(self):
        try:
            try:
                try:
                    cwd = web.input().cwd
                except:
                    cwd = None
                try:
                    shell = bool(web.input().shell)
                except:
                    shell = False
                proc = subprocess.Popen(web.data(), stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=cwd, shell=shell)
                i = 0
                while proc.poll() is None:
                    i+=1
                    if (i > 50):
                        raise Exception("Process to get running task did not report being completed.")
                    time.sleep(0.2);
                result = proc.communicate()
                return '\n'.join(result)
            except Exception as e:
                return e
        except:
            return web.badrequest()

    ## Get method to terminate a process by pid.
    #
    # @param pid (mandatory) -- pid of process to be terminated. Example: <URL>?pid=1234
    # @result termination message
    def GET(self):
        try:
            pid = int(web.input().pid)
            try:
                if sys.platform.startswith('win32'):
                    cmd = ['taskkill', '/pid', str(pid)]
                elif sys.platform.startswith('darwin'):
                    cmd = ['kill', '-9', str(pid)]
                else:
                    return web.internalerror()
                proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                return '\n'.join(proc.communicate())
            except Exception as e:
                return e
        except:
            return web.badrequest()


class Async:
    ## POST method to execute shell command asynchronically on agent local.
    #
    # @param cwd (optional) -- path of working directory. Example: <URL>?cwd=c:\sandbox
    # @param shell (optional) -- for executing shell commands. Example: <URL>?shell=True
    # @param POST message body -- complete shell command to be executed. Example: notepad
    # @result pid of new spawned process
    def POST(self):
        try:
            try:
                try:
                    cwd = web.input().cwd
                except:
                    cwd = None
                try:
                    shell = bool(web.input().shell)
                except:
                    shell = False
                proc = subprocess.Popen(web.data(), stdout=None, stderr=None, cwd=cwd, shell=shell)
                return proc.pid
            except Exception as e:
                return e
        except:
            return web.badrequest()


shellop = web.application(urls, locals())
