#!/bin/env python

"""
OVERVIEW
--------
dbmig2 is a database schema version management tool to 
upgrade db schemas from one version to the next.

dbmig2 was created to resolve workflow issues with the 
original dbmig as well as to simplify the system for use 
within and outside dbmig.

dbmig2 is addressing the following:
* Support for master SQL files.
* Support for subcategory files, such as platform.
* Removal of specialized file format.
* Removal of downgrade functionality.
* Removal of fingerprinting check.
* Improved error messaging.
* Support of legacy format for upgrading schemas only.

dbmig relies on a directory structure:
+ trunk
|--+ component
   |--+ mycomponent
         |--+db
            |--+mysql
                |--+001_mycomponent.sql

dbmig3 will refactor this structure as:
+ trunk
|--+ component
   |--+ mycomponent
         |--+db
            |--+mysql
                |--+mycomponent.sql
                |--+mycomponent_ps3.sql
                |--+mycomponent_xbl2.sql
                |--+alter
                    |--+001_mycomponent.sql
                    |--+002_mycomponent.sql
                    |--+002_mycomponent_ps3.sql

UPGRADE
-------
python dbmig2.py upgrade \
    -o host=sdevgosmydb1.online.ea.com,user=jigtest2,passwd=jigtest2,db=jigtest2 \
    --cpath component/mycomponent/

 ARGS           | OPTIONAL | DESCRIPTION
 upgrade        | N        | Command to perform.
 d              | Y        | Database type, defaults to mysql.
 r              | Y        | Root of blaze, will assume it's relative to script.
 c              | N        | Component to update
 V              | Y        | Specific version to upgrade to.
 v              | Y        | Specific content version to upgrade to.
 p              | Y        | Platform to upgrade.
 o              | N        | DB configuration string.
 cpath          | N        | Path to component to upgrade.
 singleplatform | Y        | Flag to indicate that the blazeserver invoking this script is single-platform.

INFO
----
python dbmig2.py info \
    -o host=sdevgosmydb1.online.ea.com,user=jigtest2,passwd=jigtest2,db=jigtest2
 
 ARGS      | OPTIONAL | DESCRIPTION
 info      | N        | Command to perform.
 d         | Y        | Database type, defaults to mysql.
"""
from utils import TablePrinter
from utils import MigException
from utils import parse_opts, parse_component, load_sql, find_content_sql_files, find_master_sql_files, find_sql_files
from optparse import OptionParser
import db
from db.base import Database
from db.utils import DBException
import sys
import os
import logging
import warnings

warnings.filterwarnings("ignore")

def init_version_table(adb, template):
    """ Initialize the version table if it does not exist.
    
    Args:
        adb: Database accessor
        template: sql template to create the table.
    """
    results = None
    try:
        results = adb.execute('select * from blaze_schema_info')
    except DBException, dbe:
        logging.info('Failed to query blaze_schema_info.')
        logging.info('%d: %s' % (dbe.error_no, dbe.error_msg))
        pass
    
    if not results:
        logging.info('DB migration version table not found, creating table.')
        sql = load_sql(template, [])
        for stmt in sql:
            stmt = stmt.strip()
            if stmt:
                logging.info('Executing: %s;' % stmt)
                adb.execute(stmt)
        logging.info('DB migration version table created.')

def set_version(adb, component, version):
    results = adb.execute('select component, version, last_updated, fingerprint, notes from blaze_schema_info where component = \'%s\'' % component)
    if len(results) > 0:
        adb.execute('update blaze_schema_info set version = %d, last_updated = now(), fingerprint = \'%s\', notes = \'%s\' where component = \'%s\'' % (version, '', '', component))
    else:
        adb.execute('insert into blaze_schema_info (component, version, last_updated, fingerprint, notes) values (\'%s\',%d,now(),\'%s\',\'%s\')' % (component, version, '',''))
        
def get_current_version(adb, component):
    version = 0
    rows = adb.execute('select version from blaze_schema_info where component = \'%s\'' % component)
    if rows:
        version = int(rows[0][0])
    return version

def info(adb):
    """ Prints component schema versions.

    Args:
        db: connection to database
    """
    results = adb.execute('select component, version, last_updated from blaze_schema_info')
    print ''
    printer = TablePrinter(['Component','Version', 'Last Updated'])
    for result in results:
        printer.addrow([result[0], str(int(result[1])), result[2].strftime("%m/%d/%Y %I:%M:%S %p")])
    printer.print_table()
    
def upgrade(adb, options):
    """ Upgrades a component's schema
    Args:
        adb: Database accessor
        options: Arguments for upgrade
    """
    init_version_table(adb, os.path.join(
        options.root,os.path.normcase('bin/dbmig/template/001_schemaversion.sql')))
    cur_version = get_current_version(adb, options.component)
    logging.info('Current version: %d' % cur_version)
    if cur_version == 0:
        logging.info('Tables for %s do not exist, creating schema...' % options.component)
        (latest_ver, altfiles) = find_sql_files(options.db, options.root, options.cpath, options.component, options.platform, cur_version, options.version)
        if options.version and options.version < latest_ver:
            latest_ver = options.version
            sqlfiles = altfiles
        else:
            sqlfiles = find_master_sql_files(options.db, options.root, options.cpath, options.component, options.platform, latest_ver)
            if not sqlfiles:
                logging.info('Master SQL files for %s were not found.' % options.component)
                sqlfiles = altfiles
    else:
        (latest_ver, sqlfiles) = find_sql_files(options.db, options.root, options.cpath,options.component, options.platform, cur_version, options.version)
        logging.info('Latest version: %d' % latest_ver)
        if options.version and options.version < latest_ver:
            latest_ver = options.version

    labels = []
    if (options.singlePlatform) or (options.platform == 'common'):
        labels.append('COMMON')
    if (options.platform != 'common'):
        labels.append('PLATFORM')

    if cur_version == latest_ver:
        logging.info('%s is up-to-date at version %s' % (options.component, cur_version))
    elif len(sqlfiles) > 0:
        logging.info('Upgrading %s from version %s to version %s' % (options.component, cur_version, latest_ver))
        adb.execute('SET FOREIGN_KEY_CHECKS = 0')

        """ Do each sql file then update its version in db, in lock step """
        for sqlfileinfo in sqlfiles:
            logging.info('Reading SQL statements from: %s;' % sqlfileinfo.path)
            sql = load_sql(sqlfileinfo.path, labels)
            for stmt in sql:
                stmt = stmt.strip()
                if stmt:
                    logging.info('Executing: %s;' % stmt)
                    adb.execute(stmt)
            logging.info('Writing component version (%s)' % sqlfileinfo.version)
            set_version(adb, options.component, sqlfileinfo.version)
            adb.commit()

        logging.info('Upgrade to version %s successful' % latest_ver)
    else:
        logging.error('Could not find any SQL files for %s' % (options.component))

    """ This section will perform the upgrade for content """    
    cur_content_version = get_current_version(adb, options.component + "_content")
    (latest_ver, sqlfiles) = find_content_sql_files(options.db, options.root, options.cpath, options.component, options.contentenv, options.platform, cur_content_version, options.contentVersion)
    
    logging.info('Current content version: %d' % cur_content_version)
    logging.info('Latest content version: %d' % latest_ver)

    """ We'll only need to care about 'content' version assuming there are files under <db>/<flavor>/content/* """
    if cur_content_version == latest_ver:
        logging.info('%s_content is up-to-date at version %s' % (options.component, cur_content_version))
    elif latest_ver > cur_content_version:
        if cur_content_version == latest_ver:
            logging.info('%s_content is up-to-date at version %s' % (options.component, cur_content_version))
        elif cur_content_version < latest_ver and len(sqlfiles) > 0:
            logging.info('Upgrading %s_content from version %s to version %s' % (options.component, cur_content_version, latest_ver))

            """ Do each sql file then update its version in db, in lock step """
            for sqlfileinfo in sqlfiles:
                logging.info('Reading SQL statements from: %s;' % sqlfileinfo.path)
                """ Content should always respect foreign key constraints, unless a version specifically disables it """
                adb.execute('SET FOREIGN_KEY_CHECKS = 1')
                sql = load_sql(sqlfileinfo.path, labels)
                for stmt in sql:
                    stmt = stmt.strip()
                    if stmt:
                        logging.info('Executing: %s;' % stmt)
                        adb.execute(stmt)
                logging.info('Writing component content version (%s)' % sqlfileinfo.version)
                set_version(adb, options.component + "_content", sqlfileinfo.version)
                adb.commit()

            logging.info('Upgrade to content version %s successful' % latest_ver)

    adb.close()

def basic_usage():
    """ Basic help text """
    print 'Usage dbmig.py [action] [options]'
    print 'info\t show current version information.'
    print 'upgrade\t upgrade schema version.'

def main():
    """ Process user input """    
    if len(sys.argv) <= 2:
        basic_usage()
    
    action = sys.argv[1]
    if action == 'info':
        parser = OptionParser()
        parser.add_option('-d', '--database', dest='db', default='mysql',
            help='Database type; mysql')
        parser.add_option('-o', '--dboptions', dest='dbopts', 
            help='Database login credentials')
    
        (options, args) = parser.parse_args()
        try:
            if not options.dbopts:
                raise MigException(MigException.MIGDB_INVALID_INPUT, {'opts':'--dboptions'})
            db_settings = parse_opts(options.dbopts)
            adb = db.create_database(db_settings, options.db)
            info(adb)
        except DBException, dbe:
            logging.error('%d: %s' % (dbe.error_no, dbe.error_msg))
            sys.exit(dbe.error_no)
        except MigException, mig:
            logging.error('%d: %s' % (mig.error_no, mig.error_msg))
            sys.exit(mig.error_no)
    elif action == 'drop':
        parser = OptionParser()
        parser.add_option('-d', '--database', dest='db', default='mysql',
            help='Database type; mysql')
        parser.add_option('-o', '--dboptions', dest='dbopts', 
            help='Database login credentials')
    
        (options, args) = parser.parse_args()
        try:
            if not options.dbopts:
                raise MigException(MigException.MIGDB_INVALID_INPUT, {'opts':'--dboptions'})
            db_settings = parse_opts(options.dbopts)
            adb = db.create_database(db_settings, options.db)
            tables = adb.execute('select TABLE_NAME from information_schema.tables where TABLE_SCHEMA = \'%s\'' % adb.db_settings['db'])
            print adb.db_settings['db']
            adb.execute('SET FOREIGN_KEY_CHECKS = 0')
            for t in tables:
                adb.execute('DROP TABLE %s' % t[0])
            
        except DBException, dbe:
            logging.error('%d: %s' % (dbe.error_no, dbe.error_msg))
            sys.exit(dbe.error_no)
        except MigException, mig:
            logging.error('%d: %s' % (mig.error_no, mig.error_msg))
            sys.exit(mig.error_no)
    elif action == 'upgrade':
        parser = OptionParser()
        parser.add_option('-d', '--database', dest='db', default='mysql',
            help='Database type; mysql')
        parser.add_option('-o', '--dboptions', dest='dbopts', 
            help='Database login credentials')
        parser.add_option('-r', '--root', dest='root',
            help='Root path to blaze')
        parser.add_option('-V', '--SVERSION', dest='version', type='int',
            help='Schema version to upgrade to.')
        parser.add_option('-v', '--CVERSION', dest='contentVersion', type='int',
            help='Content schema version to upgrade to.')
        parser.add_option('-p', '--platform', dest='platform',
            help='Platform to upgrade.')
        parser.add_option('-c', '--component', dest='component',
            help='Component to upgrade.')
        parser.add_option('--contentenv', dest='contentenv',
            help='Component content environment', default='')
        parser.add_option('--cpath', dest='cpath',
            help='Path to component')
        parser.add_option('--singleplatform', dest='singlePlatform', action='store_true',
            help='Flag to indicate that the blazeserver invoking this script is single-platform.')
        
        (options, args) = parser.parse_args()
        
        try:
            if (options.singlePlatform) and (options.platform == 'common'):
                raise MigException(MigException.MIGDB_INVALID_INPUT, {'opts':"platform 'common' is mutually exclusive with the --singleplatform option"})

            """ Get the base root of Blaze to perform the upgrade """
            if not options.root:
                options.root = os.path.abspath(
                    os.path.join(os.path.realpath(__file__)+'/../../../'))
                logging.info('Detected root path as %s' % options.root)
            else:
                if not os.path.exists(os.path.abspath(options.root)):
                   raise MigException(MigException.MIGDB_INVALID_INPUT, {'opts':'--root %s does not exist' % options.root})
            
            if not options.cpath:
                raise MigException(MigException.MIGDB_INVALID_INPUT, {'opts':'--cpath not set'})
                
            """ Figure out component name from path """   
            if not options.component:
                options.component = parse_component(options.cpath)
                if not options.component:
                    raise MigException(MigException.MIGDB_INVALID_INPUT, {'opts':'Could not derive component name from --cpath %s' % options.cpath})
                else:
                    logging.info('Component detected %s' % options.component)
                
            if not options.dbopts:
                raise MigException(MigException.MIGDB_INVALID_INPUT, {'opts':'--dboptions'})
            db_settings = parse_opts(options.dbopts)
            adb = db.create_database(db_settings, options.db)
            upgrade(adb, options)
        except DBException, dbe:
            logging.error('%d: %s' % (dbe.error_no, dbe.error_msg))
            if dbe.sql:
                logging.error('Error occurred in the following statments: %s' % dbe.sql)
            sys.exit(dbe.error_no)
        except MigException, mig:
            logging.error('%d: %s' % (mig.error_no, mig.error_msg))
            sys.exit(mig.error_no)
    else:
        basic_usage()
    
if __name__ == '__main__':
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
    ch.setFormatter(formatter)
    logger.addHandler(ch)
    main()
