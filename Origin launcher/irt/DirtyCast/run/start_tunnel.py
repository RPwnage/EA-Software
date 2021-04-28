#!/usr/bin/python2.7

"""
start_tunnel.py: manages the starting & reloading of stunnel
"""

import argparse
import ConfigParser
import datetime
import os
import socket
import subprocess
import sys
import time
import threading

DEFAULT_CERT_DIRECTORY = '/home/gos-ops/cert/gos2015'
ROTATE_SIZE = 1000000
ROTATE_RATE = 3600

def get_path():
    """returns the path local to this script"""
    return os.path.dirname(os.path.realpath(__file__))

def generate_config(args, config):
    """generate the service level sections of the configuration based on args"""

    for index, value in enumerate(args.ports.split(',')):
        section_name = 'voipserver' + str(index)

        config.add_section(section_name)
        config.set(section_name, 'accept', args.addr + ':' + value)
        config.set(section_name, 'connect', '127.0.0.1:' + value)

    return config

def write_global_options(configfile, args):
    """writes the global configuration that doesn't belong to a section"""

    configfile.write('foreground = ' + args.foreground + '\n')
    configfile.write('debug = ' + args.debug + '\n')
    configfile.write('pid = ' + args.pid + '\n')
    configfile.write('output = ' + args.logfile + '\n')
    configfile.write('TIMEOUTidle = ' + args.idle_timeout + '\n')

    if args.syslog == True:
        configfile.write('syslog = yes\n')
    else:
        configfile.write('syslog = no\n')

    configfile.write('cert = ' + args.cert + '\n')
    configfile.write('key = ' + args.key + '\n')
    configfile.write('CAfile = certs/cacert.pem\n')
    configfile.write('verify = 3\n')

def read_global_options(configfile, args):
    """reads global configuration from configuration file"""

    args.foreground = configfile.readline().split(' = ')[1].rstrip('\n')
    args.debug = configfile.readline().split(' = ')[1].rstrip('\n')
    args.pid = configfile.readline().split(' = ')[1].rstrip('\n')
    args.logfile = configfile.readline().split(' = ')[1].rstrip('\n')
    args.idle_timeout = configfile.readline().split(' = ')[1].rstrip('\n')

    # normally syslog is treated as True or None but we can use fully boolean as
    # we only check for True
    args.syslog = configfile.readline().split(' = ')[1].rstrip('\n') == 'yes'

    args.cert = configfile.readline().split(' = ')[1].rstrip('\n')
    args.key = configfile.readline().split(' = ')[1].rstrip('\n')

def write_config(config, args):
    """writes all the configuration data to the file"""
    with open(get_path() + '/stunnel.cfg', 'w+') as configfile:
        write_global_options(configfile, args)
        config.write(configfile)

def get_addr():
    """gets external address by attempting to connect"""

    # create a datagram socket to use to figure out our address
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # connect to find what the public ip is
    s.connect(('8.8.8.8', 80))
    return s.getsockname()[0]

def rotate_log(*args, **kwargs):
    """rotates the stunnel log when it hits the size threshold"""
    event = args[0]
    logfile = args[1]
    errlog = args[2]

    # while we are still running check if it is time to rotate the log (size hits 100m)
    while event.is_set() == False:
        # if so then call tunnel_cmd to rotate
        if (os.path.isfile(logfile) == True) and (os.path.getsize(logfile) > ROTATE_SIZE):
            subprocess.call([get_path() + '/tunnel_cmd.sh', 'rotate'])

        # if so then truncate the file instead of rotating
        # this is to keep it simple and prevent the file from getting too large
        if (os.path.isfile(errlog) == True) and (os.path.getsize(errlog) > ROTATE_SIZE):
            open(errlog, 'w').close()

        time.sleep(ROTATE_RATE)

def is_process_active(path):
    """checks if stunnel is already running"""

    # if the file doesn't exist then nothing left to do
    if os.path.isfile(path) == False:
        return False

    # the file exists so lets see if the process is active
    # kill doesn't actually kill it will just poke the process
    with open(path) as pidfile:
        pid = int(pidfile.readline())

        try:
            os.kill(pid, 0)
            return True
        except OSError:
            return False

def main(*args):
    # add the command line arguments we need to parse
    parser = argparse.ArgumentParser()
    parser.add_argument('--ports', help='list of ports we tunnel traffic for (comma delimited)', required=True)
    parser.add_argument('--addr', help='address we bind for tunneling', default=get_addr())
    parser.add_argument('--path', help='path to the server certs', default=DEFAULT_CERT_DIRECTORY)
    parser.add_argument('--cert', help='filename for cert', default='cert.pem')
    parser.add_argument('--key', help='filename for key', default='key.pem')
    parser.add_argument('--logfile', help='filename for log', default=get_path() + '/stunnel.log')
    parser.add_argument('--errlog', help='filename for stderr log', default=get_path() + '/err.log')
    parser.add_argument('--foreground', help='run in the foreground', default='yes')
    parser.add_argument('--debug', help='debug level', default='3')
    parser.add_argument('--idle_timeout', help='time in seconds before we close an idle connection', default='300')
    parser.add_argument('--pid', help='filename for the pid file', default=get_path() + '/stunnel.pid')
    parser.add_argument('--gen_only', help='generate the configuration and quit', action='store_const', const=True)
    parser.add_argument('--syslog', help='should logs be written to syslog', action='store_const', const=True)
    args = parser.parse_args(args[1:])

    # append path to cert/key data
    args.cert = args.path + '/' + args.cert
    args.key = args.path + '/' + args.key

    # if the logfile doesn't exist then touch it
    # since stunnel creates without read access to all
    if os.path.isfile(args.logfile) == False:
        with open(args.logfile, 'a'):
            os.utime(args.logfile, None)

    # if we are not generating only terminate stunnel
    # this makes sure we have nothing running before restarting and
    # allows us to regenerate the configuration fully
    # this _must_ happen before the next check
    if args.gen_only != True:
        # we use the shell script so we can reuse code
        subprocess.call([get_path() + '/tunnel_cmd.sh', 'stop'])

    # global configuration is not reconfigurable so lets read to reuse
    if os.path.isfile(get_path() + '/stunnel.cfg') == True:
        with open(get_path() + '/stunnel.cfg', 'r') as configfile:
            read_global_options(configfile, args)

    config = ConfigParser.RawConfigParser()
    config = generate_config(args, config)
    write_config(config, args)

    # if the program is not running then we start it, otherwise we are done 
    if is_process_active(args.pid) == False:
        result = 0

        # create thread for rotating
        event = threading.Event()
        thread = threading.Thread(target=rotate_log, name="rotate log", args=[event, args.logfile, args.errlog])

        # start the thread and call stunnel
        try:
            thread.start()

            # open a logfile that can be used to redirect stderr logging to (create if doesn't exist)
            with open(args.errlog, 'a+') as errlog:
                # start the stunnel process this will block until the child process exits
                result = subprocess.call(['/opt/ea/stunnel/bin/stunnel', get_path() + '/stunnel.cfg'], stderr=errlog)
        except KeyboardInterrupt:
            pass

        # signal the log rotation thread to exit
        event.set()
        return result
    else:
        print 'reloading configuration'
        return 0

if __name__ == '__main__':
    sys.exit(main(*sys.argv))
