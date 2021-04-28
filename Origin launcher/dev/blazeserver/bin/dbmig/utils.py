import os
import ConfigParser
import codecs
import string
import logging
import re

class MigException(Exception):
    MIGDB_INVALID_INPUT = (300, 'Invalid options provided, %(opts)s')
    MIGDB_INVALID_SQL_FILE = (301, 'SQL file does not exist or is invalid. %(opts)s')

    def __init__(self, error_type, msg):
        self.error_no = error_type[0]
        self.error_msg = error_type[1] % msg
        
def parse_opts(opts):
    """ Parse command line settings 
    Args:
        opts: command line parameters
    Returns:
        settings: dictionary of values
    """
    if opts is None:
        raise MigException(MIGDB_INVALID_INPUT, {'opts':opts})
    else:
        settings = {}
        tokens = opts.split(',')
        for token in tokens:
            tokens2 = token.split('=')
            if len(tokens2) != 2:
                raise MigException(MIGDB_INVALID_INPUT, {'opts':token})
            for token2 in tokens2:
                settings[tokens2[0]] = tokens2[1]
    return settings

def parse_component(cpath):
    """ Extract component name from path """
    tokens = cpath.split(os.sep)
    return tokens[-1]
    
def get_base_component_path(type, root, cpath):
    return os.path.join(root,cpath,'db',type);
    
def get_alter_component_path(type, root, cpath):
    path = os.path.join(root, cpath, 'db', type, 'alter')
    if os.path.exists(path) and (len(os.listdir(path)) > 0):
        return path
    else:
        return get_base_component_path(type, root, cpath)

def get_content_component_path(type, root, cpath, contentenv):
    base_path = get_alter_component_path(type, root, cpath)
    path = os.path.join(base_path, contentenv, 'content')
    if os.path.exists(path) and (len(os.listdir(path)) > 0):
        return path
    else:
        base_path = get_base_component_path(type, root, cpath)
        path = os.path.join(base_path, contentenv, 'content')
        if os.path.exists(path) and (len(os.listdir(path)) > 0):
            return path
        else:
            path = os.path.join(base_path, 'content')
            if os.path.exists(path) and (len(os.listdir(path)) > 0):
                return path
            else:
                return None

class SqlFileInfo:
    """ Hold sql file path and its schema version """
    def __init__(self, filepath, schemaversion):
        self.path = filepath
        self.version = schemaversion

def find_master_sql_files(type, root, cpath, component, platform, version):
    """ Search component directory for master sql files
    
    <root>/<cpath>/db/<type>/component.sql
    <root>/<cpath>/db/<type>/component_<platform>.sql
    
    Args:
        type: Type of db
        root: root directory of blaze
        cpath: path to component
        component: name of component
        platform: platform specific version
        version: schema version number master file corresponds to
    
    Returns:
        sqlfiles: list of files to fulfill migration
    """
    files = []
    base_path = get_base_component_path(type, root, cpath)
    master_path = os.path.join(base_path, '%s.sql' % component)
    if os.path.exists(master_path):
        fileInfo = SqlFileInfo(master_path, version)
        files.append(fileInfo)
    if platform:
        master_path = os.path.join(base_path, '%s_%s.sql' % (component, platform))
        if os.path.exists(master_path):
            platFileInfo = SqlFileInfo(master_path, version)
            files.append(platFileInfo)
    return files

def find_content_sql_files(type, root, cpath, component, contentenv, platform, version, end_version):
    """ Search component directory for content sql files
    <root>/<cpath>/db/<type>/<contentenv>/content/00X_component.sql
    <root>/<cpath>/db/<type>/<contentenv>/content/00X_component_<platform>.sql
    
    Args:
        type: Type of db
        root: root directory of blaze
        cpath: path to component
        component: name of component
        contentenv: content environment
        platform: platform specific version
        version: Version number to start with
        end_version: Version to stop on
    
    Returns:
        sqlfiles: list of files to fulfill content migration
    """
    files = []
    content_path = None
    if end_version == 0:
        return (0, files)

    content_path = get_content_component_path(type, root, cpath, contentenv)    
    if content_path != None:
        exists = True
        while exists:
            version += 1
            ver_str = string.zfill(version, 3)
            sql_path = os.path.join(content_path, '%s_%s.sql' % (ver_str, component))
            if os.path.exists(sql_path):
                if version > end_version:
                    break

                fileInfo = SqlFileInfo(sql_path, version)
                files.append(fileInfo)
                if platform:
                    sql_path = os.path.join(content_path, '%s_%s_%s.sql' % (ver_str, component, platform))
                    if os.path.exists(sql_path):
                        platFileInfo = SqlFileInfo(sql_path, version)
                        files.append(platFileInfo)
            else:
                exists = False
    return (version-1, files)

def find_sql_files(type, root, cpath, component, platform, version, end_version):
    """ Search component directory for alter sql files
    <root>/<cpath>/db/<type>/alter/00X_component.sql
    <root>/<cpath>/db/<type>/alter/00X_component_<platform>.sql
    
    or legacy structure:
    <root>/<cpath>/db/<type>/00X_component.sql
    <root>/<cpath>/db/<type>/00X_component_<platform>.sql
    
    Args:
        type: Type of db
        root: root directory of blaze
        cpath: path to component
        component: name of component
        platform: platform specific version
        version: Version number to start with
        end_version: Version to stop on
    
    Returns:
        sqlfiles: list of files to fulfill migration
    """
    files = []
    base_path = get_alter_component_path(type, root, cpath)
    exists = True
    while exists:
        version += 1
        ver_str = string.zfill(version, 3)
        sql_path = os.path.join(base_path, '%s_%s.sql' % (ver_str, component))
        if os.path.exists(sql_path):
            if end_version and version > end_version:
                continue
                
            fileInfo = SqlFileInfo(sql_path, version)
            files.append(fileInfo)
            if platform:
                sql_path = os.path.join(base_path, '%s_%s_%s.sql' % (ver_str, component, platform))
                if os.path.exists(sql_path):
                    platFileInfo = SqlFileInfo(sql_path, version)
                    files.append(platFileInfo)
        else:
            exists = False
    return (version-1, files)

def cleanup_procedures(sql):
    """ Clean up SQL files that contain odd delimiters
    Legacy from Tiger's implementation of dbmig.
    
    Args:
        sql: raw SQL data
    Returns:
        cleansq: scrubbed SQL
    """
    sql_delimiter = '\nDELIMITER'
    sql_delimiter_start_token = ' $$'
    sql_delimiter_end_token = ' ;'

    # Remove the trailing comment at the end of the query 
    # as MySQLlib will fail and return error
    sql = re.sub('--.*$', '', sql)

    # Break up the file by DELIMITER blocks
    sql = sql.split(sql_delimiter_start_token)

    for i in xrange(0, len(sql)):
        # Look for the END of CREATE PROCEDURE and clean up the inner 
        # contents so they will not be split by the default delimiter
        if sql[i].endswith("END"):
            sql[i] = "; \n".join(sql[i].split(";\n"));
        # Strip out the DELIMITER keyword
        if sql[i].endswith(sql_delimiter):
            sql[i] = sql[i][:-len(sql_delimiter)]
        if sql[i].startswith(sql_delimiter + sql_delimiter_end_token):
            sql[i] = sql[i][len(sql_delimiter + sql_delimiter_end_token):]

    # Re-assemble the file
    return ";\n".join(sql)

def load_sql(sqlfile, labels):
    """ Load SQL file.
    
    Args:
        sqlfile: path to sql file.
        labels: list of labels to load. May include one or both of ['PLATFORM', 'COMMON']
    
    Returns:
        sql: SQL string loaded from file.
    """
    #sql = ''
    statements = []
    sql = ''
    delim = ';\n'
    logging.info('Loading SQL file %s' % sqlfile)
    if os.path.exists(sqlfile):
        """ We need to support legacy dbmig formats as well as
        raw sql files """
        try:
            config = ConfigParser.ConfigParser()
            config.readfp(codecs.open(sqlfile, "r", "utf-8-sig"))
            if config.has_option('MIGRATION', 'VERSION') and int(config.get('MIGRATION', 'VERSION')) > 1:
                delim = ';;'
            for label in labels:
                if config.has_option('MIGRATION', label):
                    upsql = cleanup_procedures(config.get('MIGRATION',label))
                    sql += upsql
            sql += '\n'
        except (ConfigParser.NoSectionError, ConfigParser.MissingSectionHeaderError), mse:
            rawdata = open(sqlfile,'r')
            sql += cleanup_procedures(''.join(rawdata.readlines()))
            sql += '\n'
            rawdata.close()
    else:
        logging.error('File %s does not exist.' % sqlfile)
    statements.extend(sql.split(delim))
    return statements
    
class TablePrinter:
    def __init__(self, fields):
        self.fields = fields
        self.field_length = []
        self.rows = []
        for f in range(0, len(fields)):
            self.field_length.append(len(fields[f])+2)
        self.total_length = sum(self.field_length)
                
    def addrow(self, row):
        self.rows.append(row)
        for r in range(0, len(row)):
            if self.field_length[r] < len(row[r]):
                self.field_length[r] = len(row[r]) + 2
        self.total_length = sum(self.field_length)
                
    def print_table(self):
        print self._row_to_string(self.fields)
        sep = self._row_sep()
        print sep
        for r in self.rows:
            print self._row_to_string(r)
        print ""
    
    def _row_to_string(self, row):
        rowstring = []
        for r in range(0,len(row)):
            rowstring.append(str(row[r]).center(self.field_length[r]))
        
        return "|".join(rowstring)
        
    def _row_sep(self):
        row = ""
        for i in range(0,self.total_length):
            row += '-'
        return row
