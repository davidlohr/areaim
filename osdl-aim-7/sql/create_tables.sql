# DROP TABLE IF EXISTS run_summary;
CREATE TABLE run_summary (
	uid int(20) NOT NULL auto_increment,
	stp_id	int(20) NOT NULL,
	distro_name VARCHAR(20) default NULL,
	kernel_id int(20) NOT NULL,
	kernel_name VARCHAR(50) NOT NULL,
	host_name VARCHAR(20),
	host_group VARCHAR(20) NOT NULL default 'stp',
	lilo VARCHAR(20) NOT NULL default 'default',
	workload VARCHAR(20) NOT NULL default 'new_dbase',
	filesystem VARCHAR(20) NOT NULL default 'ext3',
	profile VARCHAR(20) NOT NULL default 'no',
	sysctl VARCHAR(20) NOT NULL default 'default',
	env VARCHAR(20) NOT NULL default 'none',
	PRIMARY KEY (uid),
	KEY stp_id (stp_id),
	KEY kernel_name (kernel_name),
	KEY host_name (host_name)
) TYPE=MyISAM;

# DROP TABLE IF EXISTS run_data_stp_1_CPU;
CREATE TABLE run_data_stp_1_CPU (
	forks INT(10) NOT NULL,
	jpm FLOAT NOT NULL,
	jpm_child FLOAT NOT NULL,
	jps_child FLOAT NOT NULL,
	parent_time FLOAT NOT NULL,
	child_usec FLOAT NOT NULL,
	child_ssec FLOAT NOT NULL,
	std_dev FLOAT NOT NULL,
	jti FLOAT NOT NULL,
	max_child FLOAT NOT NULL,
	min_child FLOAT NOT NULL,
	pass_num int(5),
	run_exit VARCHAR(20),
	stp_id int(20),
	uid int(20) NOT NULL auto_increment,
	PRIMARY KEY (uid),
	KEY stp_id ( stp_id ),
	KEY forks ( forks ),
	KEY pass_num ( pass_num ),
	KEY run_exit ( run_exit )
) TYPE=MyISAM;

# DROP TABLE IF EXISTS run_data_stp_2_CPU;
CREATE TABLE run_data_stp_2_CPU (
	forks INT(10) NOT NULL,
	jpm FLOAT NOT NULL,
	jpm_child FLOAT NOT NULL,
	jps_child FLOAT NOT NULL,
	parent_time FLOAT NOT NULL,
	child_usec FLOAT NOT NULL,
	child_ssec FLOAT NOT NULL,
	std_dev FLOAT NOT NULL,
	jti FLOAT NOT NULL,
	max_child FLOAT NOT NULL,
	min_child FLOAT NOT NULL,
	pass_num int(5),
	run_exit VARCHAR(20),
	stp_id int(20),
	uid int(20) NOT NULL auto_increment,
	PRIMARY KEY (uid),
	KEY stp_id ( stp_id ),
	KEY forks ( forks ),
	KEY pass_num ( pass_num ),
	KEY run_exit ( run_exit )
) TYPE=MyISAM;

# DROP TABLE IF EXISTS run_data_stp_4_CPU;
CREATE TABLE run_data_stp_4_CPU (
	forks INT(10) NOT NULL,
	jpm FLOAT NOT NULL,
	jpm_child FLOAT NOT NULL,
	jps_child FLOAT NOT NULL,
	parent_time FLOAT NOT NULL,
	child_usec FLOAT NOT NULL,
	child_ssec FLOAT NOT NULL,
	std_dev FLOAT NOT NULL,
	jti FLOAT NOT NULL,
	max_child FLOAT NOT NULL,
	min_child FLOAT NOT NULL,
	pass_num int(5),
	run_exit VARCHAR(20),
	stp_id int(20),
	uid int(20) NOT NULL auto_increment,
	PRIMARY KEY (uid),
	KEY stp_id ( stp_id ),
	KEY forks ( forks ),
	KEY pass_num ( pass_num ),
	KEY run_exit ( run_exit )
) TYPE=MyISAM;

# DROP TABLE IF EXISTS run_data_stp_8_CPU;
CREATE TABLE run_data_stp_8_CPU (
	forks INT(10) NOT NULL,
	jpm FLOAT NOT NULL,
	jpm_child FLOAT NOT NULL,
	jps_child FLOAT NOT NULL,
	parent_time FLOAT NOT NULL,
	child_usec FLOAT NOT NULL,
	child_ssec FLOAT NOT NULL,
	std_dev FLOAT NOT NULL,
	jti FLOAT NOT NULL,
	max_child FLOAT NOT NULL,
	min_child FLOAT NOT NULL,
	pass_num int(5),
	run_exit VARCHAR(20),
	stp_id int(20),
	uid int(20) NOT NULL auto_increment,
	PRIMARY KEY (uid),
	KEY stp_id ( stp_id ),
	KEY forks ( forks ),
	KEY pass_num ( pass_num ),
	KEY run_exit ( run_exit )
) TYPE=MyISAM;
