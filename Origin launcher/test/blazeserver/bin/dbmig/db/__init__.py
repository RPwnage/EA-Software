from mysql import MySQLDatabase

def create_database(db_settings, db_type='mysql'):
    """ Create a database connection

    Args:
        db_type: type of database to communicate to.
        db_settings: settings to configure database.

    Returns:
        db: database object.
    """
    if db_type == 'mysql':
        return MySQLDatabase(db_settings)
    else:
        raise DBException(DB_INVALID_TYPE[0], DB_INVALID_TYPE[1] % {'db_type':db_type})
