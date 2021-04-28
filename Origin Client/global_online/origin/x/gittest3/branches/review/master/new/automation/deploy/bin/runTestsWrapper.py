import subprocess
import os
import argparse
import time
import psutil # c:\origin_squish is using psutil version 1.0.1
from pprint import pprint


def cleanup(parent_process):
    try:
        parent = psutil.Process(parent_process.pid)
        if parent.is_running():
            for child in parent.get_children(recursive=True):
                if child.is_running():
                    try:
                        print 'terminating child process {0} {1}'.format(child.pid, child.name)
                        child.kill()
                    except psutil.NoSuchProcess as e:
                        print 'NoSuchProcess: {0}'.format(str(e))
                    except Exception as e:
                        print 'Exception caught at killing child process {0}: {1}'.format(child.pid, str(e))
        # parent may be gone after all child process are terminated
        if parent.is_running():
            try:
                print 'terminating parent process {0} {1}'.format(parent.pid, parent.name)
                parent.kill()
            except psutil.NoSuchProcess as e:
                print 'NoSuchProcess: {0}'.format(str(e))
            except Exception as e:
                print 'Exception caught at killing parent process {0}: {1}'.format(parent.pid, str(e))
    except Exception as e:
         print 'Exception caught at getting child process of {0}: {1}'.format(parent_process.pid, str(e))



def main():
    arg_parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    arg_parser.add_argument("squish_runtests_py", help="The path to runTests.py (including filename)")
    arg_parser.add_argument("squish_runtests_py_args", help="runTests.py argument")
    arg_parser.add_argument("python_path", help="The path to python root (excluding executable filename)")
    arg_parser.add_argument("node_path", help="The path to node.exe (including executable filename)")
    arg_parser.add_argument("build_path", help="The path to minspa/mockup build root")
    arg_parser.add_argument("--minispa", action="store_true", help="Run minispa")
    arg_parser.add_argument("--mockup", action="store_true", help="Run mockup")

    arguments = vars(arg_parser.parse_args())
    print 'arguments:'
    pprint(arguments)

    minispa_process = None
    mockup_process = None

    if arguments['minispa'] :
        # Run minispa server
        try:
            os.chdir(os.path.join(arguments['build_path'], 'app'))
            server_path = os.path.join('node_modules', 'http-server', 'bin', 'http-server')
            minispa_process = subprocess.Popen([arguments['node_path'], server_path, '-p', '1500'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            print 'minispa parent process: {0}'.format(minispa_process.pid)
        except Exception as e:
            print 'Exception caught at running minispa: {0}'.format(str(e))

    if arguments['mockup'] :
        # Run mockup server
        try:
            os.chdir(os.path.join(arguments['build_path'], 'mockup'))
            server_path = os.path.join('src', 'start.js')
            mockup_process = subprocess.Popen([arguments['node_path'], server_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            print 'mockup parent process: {0}'.format(mockup_process.pid)
        except Exception as e:
            print 'Exception caught at running mockup: {0}'.format(str(e))


    # give server a little bit time to start
    time.sleep(3)


    # execute and block
    try:
        os.chdir(arguments['python_path'])
        command = 'python.exe \"{0}\" {1}'.format(arguments['squish_runtests_py'], arguments['squish_runtests_py_args'])
        print 'executing: {0}'.format(command)
        os.system(command)
    except Exception as e:
        print 'Exception caught: {0}'.format(str(e))


    # clean up
    if minispa_process:
        # terminate minispa server
        cleanup(minispa_process)

    if mockup_process:
         # terminate mockup server
        cleanup(mockup_process)



if __name__ == '__main__':
    main()
