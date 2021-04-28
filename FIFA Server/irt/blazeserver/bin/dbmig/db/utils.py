DB_INVALID_TYPE = (100, 'Database type %(db_type)s is invalid.  Please select a type such as like mysql')
DB_INVALID_CONFIG = (101, 'Database could not be configured.  Missing or invalid parameter %(key)s')
DB_CONNECT = (102, 'Could not connect to with the supplied credentials: %(dbopts)s')
DB_QUERY = (103, 'Could not execute query successfully: %(error)s')

class DBException(Exception):
    def __init__(self, error_no, error_msg, sql=None):
        self.error_no = error_no
        self.error_msg = error_msg
        self.sql = ''