#!/usr/bin/env php
<?php
/**
 * Initialize the MySQL database for use with Excel/loc imports. this will add a new database called xlocalization
 *
 * Requires MySQL to be intalled on the VM
 * sudo apt-get update
 * sudo apt-get insall mysql-5.5 mysql-5.5-server mysql-5.5-client php5-mysql
 *
 * @author Richard Hoar
 */
$pdo = new PDO('mysql:host=localhost;port=3306', 'root', 'admin');

//Allow access to the VM host
$pdo->query("GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' IDENTIFIED BY 'admin' WITH GRANT OPTION");

//Create the database
$pdo->query("create database xlocalization");

echo "\nDatabase Intiialized. Perform your first import using import.php!\n";
