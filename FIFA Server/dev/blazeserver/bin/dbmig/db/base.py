import logging
class Database(object):
    """ Encapsulates database activity """
    
    def __init__(self, db_settings):
        """ Initialize a connection

        Args:
            db_settings: dictionary of db credentials.
        """
        self.connection = None
        self.cursor = None
        self.db_settings = db_settings

    def get_connection(self):
        """ Retrieve a connection from the db

        Returns:
            con: database specific connection.
        """
        pass
    
    def get_cursor(self, con):
        pass

    def execute(self, sql):
        """ Execute a query from the db.

        Args:
            sql: SQL statement to execute.

        Returns:
            results: Results of execution
        """
        return None
    
    def commit(self):
        pass
    
    def close(self):
        pass

    def print_settings(self):
        logging.info(self.db_settings)
