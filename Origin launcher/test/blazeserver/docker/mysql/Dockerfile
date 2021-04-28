FROM mysql/mysql-server:5.7

ARG DBBASE
ARG DBUSER
ARG DBPASS
RUN \
        sed -Ei 's/^(bind-address|log)/#&/' /etc/my.cnf && \
        echo "sql-mode=\"\"" >> /etc/my.cnf && \
        echo "bind-address=\"0.0.0.0\"" >> /etc/my.cnf && \
        echo "CREATE DATABASE ${DBBASE};" > /docker-entrypoint-initdb.d/mysql_init.sql && \
        echo "CREATE USER '${DBUSER}'@'%' IDENTIFIED BY '${DBPASS}';" >> /docker-entrypoint-initdb.d/mysql_init.sql && \
        echo "GRANT ALL PRIVILEGES ON ${DBBASE}.* TO '${DBUSER}'@'%'; FLUSH PRIVILEGES;" >> /docker-entrypoint-initdb.d/mysql_init.sql

