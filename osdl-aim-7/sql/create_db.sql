# Sql copied from the command line.
# This is not an actual script, mouse this into a mysql window

set password for 'root'@'localhost' = PASSWORD('mysql');
set password for 'root'@'memac' = PASSWORD('mysql');
grant all privileges on *.* to 'mysql'@'localhost' IDENTIFIED by 'mysql'with GRANT OPTION;

grant all privileges on *.* to 'mysql'@'%' IDENTIFIED by 'mysql' with GRANT OPTION;

 create database reaim_db;
 use reaim_db;
 grant select,insert,update,delete,create,drop on reaim_db to 'reaim'@'%'identified by 'reaim';

grant select,insert,update,delete,create,drop on reaim_db to 'reaim'@'%'identified by 'reaim';  


