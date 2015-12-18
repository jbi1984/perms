This small command-line program will allow you to create a database of a directory including all files and sub-directory's so that if something goes wrong you can restore all the permissions, user ids and group ids.
The database is just a text file so is quite compressible.

Command line usage is:
Usage: perms OPTION STARTDIRECTORY

Create a database of permissions, uids and guids for files stating at STARTDIRECTORY

-c, --create    create a database

-r, --repair    repair file permissions using the database in STARTDIRECTORY if given if not in the current directory

-v, --verify	verify file permissions using the database in STARTDIRECTORY if given if not in the current directory

-f, --fix	when used with the --verify switch will attempt to repair the permissions, users and groups

-q, --quiet     only report errors

-n, --nolinks   don't recurse into folders that are links

-s, --system    recurse into the system folders /sys,/proc,/dev, /tmp, /lost+found (Setting permissions on these files doesn't make a lot of sense as they are (mostly) recreated at boot time)

-u, --users     same as above but for the /home folder if /home is on a seperate partition you may need to use the -l switch as well

-e, --exclude   exclude a directory from the database, one --exclude for every folder FULL path names only

-l, --leave     go into other file systems, default is to stay on the same device

-h, -?, --help  print this help

STARTDIRECTORY is optional if not included the current directory is used

Examples:

List the permssions for all files in directory:'/etc but not for /etc/webmin'

perms -e /etc/webmin /etc

Create a database in / including the /home folder

sudo perms --create --users /

Repair permissions for user 'someuser', obviously you must have created the database first!

cd /home/someuser && sudo perms --repair

Verify and fix  permissions for the current directory

perms --verify --fix

Repair permissions for the current directory

perms --repair

You may need to run perms as root to be able to read/repair certain folders.
Stat errors for a file usually indicate a broken link, a deleted file or a circular link reference.
ie /usr/bin/X11/X11 which is a symlink to usr/bin/X11!

Report bugs to kdhedger@yahoo.co.uk

This was tested by creating a database at / then chmoding recursively the file system to have permissions of 0 and recursively chowning the filesystem to a non root user and a non root group, (I took a full backup before hand!), this made the file system unbootable!
Running perms from a recovery partition then repaired the whole thing and the system was bootable again.
The database on a system of 5.3G is 69.2M archived the size is 4M, (sizes will vary).