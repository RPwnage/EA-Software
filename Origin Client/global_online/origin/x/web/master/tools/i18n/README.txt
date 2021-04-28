This package will populae a local mysql database with all of the translations from QA
Main. You can then hand this Excel workbook file off for localization.

Download the Mysql to Excel connector Here
http://dev.mysql.com/downloads/file.php?id=412152

Connection settings
server: local.cms.origin.com
port: 3306
u: root
p: admin

Initialize the database (first run):

run ./init.php

Import translations into the database:

run ./import.php

Acessing from windows

By default mysql is bound to local use only. change this in /etc/mysql/my.cnf

Comment the line like so,
# bind-address 127.0.0.1

and then restart MySQL
sudo restart mysql

