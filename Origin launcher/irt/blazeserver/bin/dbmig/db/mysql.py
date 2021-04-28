from base import Database
from utils import *
import MySQLdb

class MySQLDatabase(Database):
    """ MySQL database representation """
    DB_KEYS = ['host','port','user','passwd','db']
    def __init__(self, db_settings):
        Database.__init__(self, db_settings)
        if 'port' not in self.db_settings:
            self.db_settings['port'] = '3306'

    def get_connection(self):
        if not self.connection:
            for key in self.DB_KEYS:
                if key not in self.db_settings:
                    raise DBException(DB_INVALID_CONFIG[0], DB_INVALID_CONFIG[1] % {'key':key})

            try:
                self.connection = MySQLdb.Connect(host=self.db_settings['host'], 
                    port=int(self.db_settings['port']), user=self.db_settings['user'],
                    passwd=self.db_settings['passwd'],db=self.db_settings['db'],charset='utf8')
                self.cursor = self.connection.cursor()
            except MySQLdb.Error, e:
                raise DBException(DB_CONNECT[0], DB_CONNECT[1] % {'dbopts':self.db_settings})
        return self.connection
    
    def get_cursor(self):
        return self.cursor

    def execute(self, sql):
        con = None
        cur = None
        try:
            con = self.get_connection()
            cur = self.get_cursor()
            cur.execute(sql)
            rows = cur.fetchall()
            return rows
        except MySQLdb.Error, e:
            raise DBException(DB_QUERY[0], DB_QUERY[1] % {'error':e.args[1]}, sql)
    
    def commit(self):
        if self.connection:
            self.connection.commit()
    
    def close(self):
        if self.cursor:
            self.cursor.close()
            self.cursor = None
            
        if self.connection:
            self.connection.commit()
            self.connection.close()
            self.connection = None