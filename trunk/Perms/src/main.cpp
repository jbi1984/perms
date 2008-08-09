/*
.
*/
/*
** perms
** Copyright (C) Keith Hedger, 2008.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**/

//Mon 31 Jul 2006 12:30:55 BST 

//#include <gnome.h>
//#include <glib/gstdio.h>
//#include <glib.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#define DBNAME "./.permsDB"

struct option long_options[] =
	{
		{"create",0,0,'c'},
		{"repair",0,0,'r'},
		{"quiet",0,0,'q'},
		{"nolinks",0,0,'n'},
		{"system",0,0,'s'},
		{"users",0, 0,'u'},
		{"exclude",1,0,'e'},
		{"leave",0,0,'l'},
		{"help",0,0,'?'},
		{0, 0, 0, 0}
	};


FILE	*fp;
bool	report=true;
bool	create=false;
bool	repair=false;
bool	nolinks=false;
bool	incsystem=false;
bool	incusers=false;
bool	samedevice=true;

char	cwd[256];
char	*excludes[256];
int	excludescnt=0;
int	thisdevice;


bool insidesysdirs(char *filename)
{
	struct	stat	linkstat;
	int	staterr=-1;
	char	buffer[4096];
	bool retval=true;

	staterr=lstat(filename,&linkstat);

	if (S_ISLNK(linkstat.st_mode))
		{
		staterr=readlink (filename,buffer,4095);
		if (staterr==-1)
			retval=false;
		else
			buffer[staterr]=0;
		}
	else
		{
		strcpy(buffer,filename);
		}

	if ((incsystem==false) && (strncmp("/proc",buffer,5)==0 || strncmp("/sys",buffer,4)==0 || strncmp("/dev",buffer,4)==0 || strncmp("/tmp",buffer,4)==0 || strncmp("/lost+found",buffer,11)==0))
		retval=false;

	if ((incusers==false) && (strncmp(buffer,"/home",5)==0))
		retval=false;


	return retval;
}

bool skipexclude(char *filename)
{
	int cnt=0;
	bool retval=false;
	while (cnt<excludescnt)
		{
		if (strcasecmp(filename,excludes[cnt++])==0)
			{
			retval=true;
			break;
			}
		}

	return retval;
}

bool skipdir(char *filename)
{
	bool	retval=false;

	if (strcasecmp(filename,".")==0)
		retval=true;
	if (strcasecmp(filename,"..")==0)
		retval=true;

	return retval;
}

void repairfiles(void)
{
	char	data[40];
	char	buffer[4096];

	fp = fopen(DBNAME,"r");
	if (fp==0)
		{
		printf("Can't open DB file for reading\n");
		return;
		}
	
	while(true)
		{
		if (fgets(buffer,4095,fp)!=NULL)
			{
			fgets(data,40,fp);
			char *perms,*uid,*gid;

			perms=strtok(data,":");
			uid=strtok(NULL,":");
			gid=strtok(NULL,":");			
			buffer[strlen(buffer)-1]=0;

			chown (buffer,atol(uid),atol(gid));
			chmod (buffer,atol(perms));
			if (report==true)
				{
				printf("Setting permissions,uid and gid of %s to :%o:%s:%s",buffer,atoi(perms),uid,gid);
				}
			}
		else
			break;
		}
}

void parsedir(char *filename)
{
	DIR	*dir_p;
	struct	dirent	*dir_entry_p;
	struct	stat	stat_p,linkstat;
	int	staterr=-1;
	char	buffer[4096];

	dir_p = opendir(filename);
	if (dir_p==NULL)
		return;

	while( NULL != (dir_entry_p = readdir(dir_p)))
		{
		if (skipdir(dir_entry_p->d_name)==true)
			continue;

		if (strcasecmp(filename,"/")==0)//root dir special case i hate special cases!
			sprintf(buffer,"/%s",dir_entry_p->d_name);		
		else	
			sprintf(buffer,"%s/%s",filename,dir_entry_p->d_name);

		if (skipexclude(buffer)==true)
			continue;

		staterr=stat(buffer,&stat_p);

		if ( staterr==-1)
			printf(" Error occoured attempting to stat %s\n",buffer);
		else
			{
			if (stat_p.st_dev!=thisdevice && samedevice==true)
				continue;

			if (report==true)
				printf("permissions for %s=%o owner=%i group=%i\n",buffer,stat_p.st_mode,stat_p.st_uid,stat_p.st_gid);
		
			if (create==true)
				fprintf(fp,"%s\n%i:%i:%i\n",buffer,stat_p.st_mode,stat_p.st_uid,stat_p.st_gid);

			if (S_ISDIR(stat_p.st_mode))
				{
				if (nolinks==true)
					{
					staterr=lstat(buffer,&linkstat);
					if ( staterr==-1)
						{
						printf(" Error occoured attempting to stat %s\n", dir_entry_p->d_name);
						}
					if (S_ISLNK(linkstat.st_mode))
						continue;
					}

				if (insidesysdirs(buffer)==true)
					parsedir(buffer);
				}
			}
		}

	closedir(dir_p);
	return;

}

void printhelp(void)
{
	printf("Usage: perms [OPTION] [STARTDIRECTORY]\n");
	printf("Create a database of permissions, uids and guids for files stating at [STARTDIRECTORY]\n");
	printf("-c, --create	create a database\n");
	printf("-r, --repair	repair file permissions using the database in [STARTDIRECTORY] if given if not in the current directory\n");
	printf("-q, --quiet	only report errors\n");
	printf("-n, --nolinks	don't recurse into folders that are links\n");
	printf("-s, --system	recurse into the system folders /sys,/proc,/dev, /tmp, /lost+found (Setting permissions on these files doesn't make a lot of sense as they are (mostly) recreated at boot time)\n");
	printf("-u, --users	same as above but for the /home folder\n");
	printf("		if /home is on a seperate partition you may need to use the -l switch as well\n");
	printf("-e, --exclude	exclude a directory from the database, one --exclude for every folder FULL path names only\n");
	printf("-l, --leave	go into other file systems, default is to stay on the same device\n");
	printf("-h, -?, --help	print this help\n\n");
	printf("[STARTDIRECTORY] is optional if not included the current directory is used\n\n");
	printf("Examples:\n\n");
	printf("List the permssions for all files in directory:'/etc but not for /etc/webmin'\n");
	printf("perms -e /etc/webmin /etc\n");
	printf("Create a database in / including the /home folder\n");
	printf("sudo perms --create --users /\n");
	printf("Repair permissions for user 'someuser', obviously you must have created the database first!\n");
	printf("cd /home/someuser && sudo perms --repair\n");
	printf("Repair permissions for the current directory\n");
	printf("perms --repair\n\n");
	printf("You may need to run perms as root to be able to read/repair certain folders.\n");
	printf("Stat errors for a file usually indicate a broken link or a circular link reference.\n");
	printf("ie /usr/bin/X11/X11 which is a symlink to usr/bin/X11!\n\n");
	printf("Report bugs to kdhedger@yahoo.co.uk\n");
}

int main(int argc, char **argv)
{
	int c;

	while (1)
		{
		int option_index = 0;
		c = getopt_long (argc, argv, ":e:qcrnus?hl",long_options, &option_index);
		if (c == -1)
			break;

		switch (c)
			{
			case 'c':
				create=true;
				repair=false;
				break;

			case 'r':
				repair=true;
				create=false;
				break;

			case 'q':
				report=false;
				break;

			case 'n':
				nolinks=true;
				break;
		
			case 's':
				incsystem=true;
				break;
		
			case 'u':
				incusers=true;
				break;
		
			case 'e':
				excludes[excludescnt++]=optarg;
				break;
		
			case 'l':
				samedevice=false;
				break;
		
			case '?':
			case 'h':
				printhelp();
				return 0;
				break;

			default:
				printf ("?? Unknown argument ??\n");
				return 1;
			break;
			}
		}
	
	if (optind < argc)
		{
		if (chdir(argv[optind]) == -1)
			{
			printf("Can't CD into directory %s\n",argv[optind]);
			return 1;
			}
		}


struct	stat	stat_p;

	stat(".",&stat_p);
	thisdevice=stat_p.st_dev;

	if (create==true)
		{
		fp = fopen(DBNAME,"w");
		if (fp==0)
			{
			printf("Can't open DB file for writing\n");
			return 1;
			}

		fprintf(fp,"%s\n%i:%i:%i\n",".",stat_p.st_mode & 0777,stat_p.st_uid,stat_p.st_gid);
		}

	if (report==true)
		printf("permissions for %s=%o owner=%i group=%i\n",".",stat_p.st_mode,stat_p.st_uid,stat_p.st_gid);

	if (repair==true)
		{
		repairfiles();
		return 0;
		}

	getcwd(cwd,255);
	parsedir((char*)cwd);

	if (fp!=0)
		fclose(fp);
	return 0;
}

