#
# Wraps psutil to provide utilities specific to blaze processes
#
# Example:
#    for process in blazeprocess.enumerate( lambda p: p.label == serviceName ):
#        print process.program, process.name, process.type, process.id
#
# Output:
#    monitor configMaster0 configMaster 0
#    monitor coreMaster1 configMaster 1
#    monitor coreSlave2 configMaster 2
#
#
# Example:
#    for process in blazeprocess.enumerate( lambda p: p.label == serviceName, matchall=True ):
#        print process.program, process.name, process.type, process.id
#
# Output:
#    monitor configMaster0 configMaster 0
#    blazeserver configMaster0 configMaster 0
#    monitor coreMaster1 configMaster 1
#    blazeserver coreMaster1 configMaster 1
#    monitor coreSlave2 configMaster 2
#    blazeserver coreSlave2 configMaster 2

#### Imports

import argparse
import psutil

__ALL__ = [ 'InvalidBlazeProcess', 'BlazeProcess', 'Popen', 'enumerate' ]

#### Classes

class InvalidBlazeProcess(Exception):
    """Raised by BlazeProcess when a non blaze instance provided"""
    pass

class BlazeProcess(object):
    """Blaze process wrapper for psutil.Process object"""
    def __init__(self, process):
        object.__init__(self)

        if isinstance( process, psutil.Process ):
            self._subproc = process
        elif isinstance( process, int ):
            self._subproc = psutil.Process(pid)
        else:
            raise InvalidBlazeProcess("Cannot wrap non process object")
        
        # check to make sure the command is a blaze process
        if not any( self._subproc.name.startswith(valid) for valid in [ 'monitor', 'blazeserver' ] ):
            raise InvalidBlazeProcess("Process with pid {0} is not a blaze instance".format(self._subproc.pid))

        # Monitor instances will have leading arguments that we 
        # don't want to deal with in the parser.  Find the start of the 
        # blazeserver arguments and parse from there
        cmdline = self._subproc.cmdline

        index = 0
        parsable = None
        for arg in cmdline:
            if arg.endswith('blazeserver'):
                parsable = cmdline[index:]
                break

        if not parsable:
            raise InvalidBlazeProcess("Command line for process {0} does not connect blaze arguments".format(self._subproc.pid))

        _parse_blaze_cmdline(parsable, self)

    def __cmp__(self, other):
        if not isinstance( other, BlazeProcess ):
            return False

        return (self.label == other.label) and (self.id == other.id)

    def __eq__(self, other):
        if not isinstance( other, BlazeProcess ):
            return False

        return (self.label == other.label) and (self.id == other.id)

    def __ne__(self, other):
        return not self.__eq__(other)


    def __dir__(self):
        return list(set(dir(BlazeProcess) + dir(psutil.Process)))

    def __getattr__(self, name):
        return getattr(self._subproc, name)

    @property
    def program(self):
        """Property used to reveal the underlying process's name, eg. blazeserver, since
        the BlazeProcess object will return the instance name, eg. configMaster, for
        the 'name' attribute."""

        name = self._subproc.name
        return name


class _AppendDefine(argparse.Action):
    """Specialized argparse Action for parsing blaze preprocessor 
    commandline options, such as: -DPLATFORM=ps3"""

    def __call__(self, parser, namespace, values, option_string=None):
        if not hasattr(namespace, self.dest):
            setattr(namespace, self.dest, dict())

        items = getattr(namespace, self.dest)

        if not items:
            items = dict()
            setattr(namespace, self.dest, items)

        temp = values.split("=",1)
        if len(temp) == 2:
            items[temp[0]] = temp[1]
        else:
            items[temp[0]] = None

#### Utility functions

def enumerate(function=None, matchall=False):
    """Provides a list of running blaze processes.  Only monitor 
    processes will be returned if they exists.  Otherwise the actual 
    blaze processes are returned.
    
    This is should be the main access point for getting blaze processes

    :param function: a function used to filter processes. should return True for desired processes. 
    :type function: callable
         default: None

    :param matchall: if true, enumerate will return all blaze processes (monitors and blazeservers)
    :type matchall: boolean
        default: False
    """

    procList = list()

    for proc in psutil.process_iter():
        try:
            blzProc = BlazeProcess(proc)
        except (psutil.AccessDenied, psutil.NoSuchProcess, InvalidBlazeProcess) as e:
            continue

        if function == None or function( blzProc ):
            procList.append(blzProc)

    procList.sort( key = lambda proc: proc.id )
   
    if matchall:
        return procList
    else:
        monitors = filter( lambda proc: proc.program == 'monitor', procList )

        if len(monitors) > 0:
            return monitors
        else:
            return procList

def _parse_blaze_cmdline(*args, **kwargs):
    """Convience wrapper which uses argparse.parse_known_args to parse blaze command line arguments"""

    blazeparser = argparse.ArgumentParser()
    blazeparser.add_argument("-l", dest="label")
    blazeparser.add_argument("-p", dest="pidfile")
    blazeparser.add_argument("--id", type=int)
    blazeparser.add_argument("--type")
    blazeparser.add_argument("--name") 
    blazeparser.add_argument("--dbdestructive", action="store_true") 
    blazeparser.add_argument("--logdir")
    blazeparser.add_argument("--logname")
    blazeparser.add_argument("--argfile") 
    blazeparser.add_argument("--bootFileOverride") 
    blazeparser.add_argument("--outOfService", action="store_true") 
    blazeparser.add_argument("--disable-config-string-escapes", dest="disableConfigStringEscapes", action="store_true") 
    blazeparser.add_argument("--monitor-pid", dest="monitorPid", type=int, default=-1) 
    blazeparser.add_argument("-D", dest="defines", action=_AppendDefine) 
    blazeparser.add_argument("bootfile") 

    return blazeparser.parse_known_args(*args, **kwargs)