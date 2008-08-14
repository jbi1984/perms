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
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdarg.h>
#include <fcntl.h>

#define DBNAME "./.permsDB"
#define VERSION "0.0.9"

struct option long_options[] =
	{
		{"create",0,0,'c'},
		{"repair",0,0,'r'},
		{"quiet",0,0,'q'},
		{"nolinks",0,0,'n'},
		{"system",0,0,'S'},
		{"users",0, 0,'u'},
		{"exclude",1,0,'e'},
		{"leave",0,0,'l'},
		{"verify",0,0,'V'},
		{"fix",0,0,'f'},
		{"strip",1,0,'s'},
		{"prefix",1,0,'p'},
		{"version",0,0,'v'},
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
bool	verify=false;
bool	fix=false;
bool	strip=false;
bool	prefix=false;

char	cwd[256];
char	*excludes[256];
int	excludescnt=0;
int	thisdevice;
char	*stripbuffer;
char	prefixbuffer[256]=".";

void printout(const char *fmt, ...)
{
	va_list	ap;
	int	i;
	char	*s;

	if (report==false)
		return;

	va_start(ap,fmt);
	while (*fmt)
		{
        	switch(*fmt++)
        		{
			case 's':           
				s=va_arg(ap,char *);
				printf("%s",s);
				break;
			case 'i':           
				i=va_arg(ap,int);
				printf("%i",i);
				break;
			case 'o':           
				i=va_arg(ap,int);
				printf("%o",i);
				break;
			case 'n':
				printf("\n");
				break;
			}
		}
	va_end(ap);
}

bool insidesysdirs(char *filename)
{
	struct	stat	linkstat;
	int	staterr=-1;
	char	buffer[4096];
	bool retval=true;

	staterr=lstat(filename,&linkstat);

	if (S_ISLNK(linkstat.st_mode))
		{
		staterr=readlink(filename,buffer,4095);
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

void addprefix(char *filename)
{
	char	buffer[4096]={};

	strcat(buffer,prefixbuffer);
	strcat(buffer,filename);
	strcpy(filename,buffer);
}

void stripdirs(char *filename)
{
	char	buffer[4096];
	char	*startofdirs;
	int	len=-1;
	int	start=-1;
	int	striplen=strlen(stripbuffer);
	int	namelen=strlen(filename);

	startofdirs=strstr(filename,stripbuffer);
	if (startofdirs!=NULL)
		{
		len=(long)startofdirs-(long)filename;
		memcpy(&buffer[0],&filename[0],len);
		memcpy(&buffer[len],&filename[len+striplen],namelen-(len+striplen));		
		buffer[namelen-(len+striplen)]=0;
		strcpy(filename,buffer);
		}
}

void verifyperms(void)
{
	char	data[40];
	char	buffer[4096];
	struct	stat	stat_p;
	int	staterr=-1;

	bool	uiderror;
	bool	giderror;
	bool	permserror;

	fp = fopen(DBNAME,"r");
	if (fp==0)
		{
		fprintf(stderr,"Can't open DB file for reading\n");
		return;
		}

	while(true)
		{
		if (fgets(buffer,4095,fp)!=NULL)
			{
			if (strip==true)
				stripdirs((char*)&buffer[0]);

			if (prefix==true || strip==true)
				addprefix((char*)&buffer[0]);

			uiderror=false;
			giderror=false;
			permserror=false;

			fgets(data,40,fp);
			char *perms,*uid,*gid;

			perms=strtok(data,":");
			uid=strtok(NULL,":");
			gid=strtok(NULL,":");			
			buffer[strlen(buffer)-1]=0;

			printout("sss","Verifying permissions of ",buffer,"...");
			staterr=stat (buffer, &stat_p);
			if ( staterr==-1)
				{
				printout("sn","FAIL");
				fprintf(stderr,"Error occoured attempting to stat %s\n",buffer);
				continue;
				}
			else
				{
				if (stat_p.st_mode!=atol(perms))
					permserror=true;
				if (stat_p.st_uid!=atol(uid))
					uiderror=true;
				if (stat_p.st_gid!=atol(gid))
					giderror=true;
				}

			if ((permserror==true || uiderror==true || giderror==true))
				{
				printout("sn","FAIL");
				if (permserror==true)
					printout("soson","Permissions are ",stat_p.st_mode,", should be ",atol(perms));

				if (uiderror==true)
					printout("sisin","User is ",stat_p.st_uid,", should be ",atol(uid));
				if (giderror==true)
					printout("sisin","Group is ",stat_p.st_gid,", should be ",atol(gid));

				}
			else
				printout("sn","OK");
			if (fix==true)
				{
				if (permserror==true)
					{
					printout("sn","Fixing permissions");
					chmod (buffer,atol(perms));
					}
				if (uiderror==true || giderror==true)
					{
					printout("sn","Fixing users and groups");
					chown(buffer,atol(uid),atol(gid));
					}
				}

			}
		else
			break;
		}
	return;
}

void repairfiles(void)
{
	char	data[40];
	char	buffer[4096];

	fp = fopen(DBNAME,"r");
	if (fp==0)
		{
		fprintf(stderr,"Can't open DB file for reading\n");
		return;
		}
	
	while(true)
		{
		if (fgets(buffer,4095,fp)!=NULL)
			{
		if (strip==true)
			stripdirs((char*)&buffer[0]);

		if (prefix==true || strip==true)
			addprefix((char*)&buffer[0]);

			fgets(data,40,fp);
			char *perms,*uid,*gid;

			perms=strtok(data,":");
			uid=strtok(NULL,":");
			gid=strtok(NULL,":");			
			buffer[strlen(buffer)-1]=0;

			chown (buffer,atol(uid),atol(gid));
			chmod (buffer,atol(perms));
			printout("sssossss","Setting permissions,uid and gid of ",buffer," to :",atoi(perms),":",uid,":",gid);
			}
		else
			break;
		}
}

void parsedir(char *filename)
{
	DIR	*dir_p;
	struct	dirent	*dir_entry_p;
	struct	stat	stat_p,linkstat,errstat;

	
	int	staterr=-1;
	int	lstaterr=-1;
	int	errstaterr=-1;
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
			fprintf(stderr," Error occoured attempting to stat %s\n",buffer);
		else
			{
			if (stat_p.st_dev!=thisdevice && samedevice==true)
				continue;
			printout("sssosisin","Permissions for ",buffer,"=",stat_p.st_mode," owner=",stat_p.st_uid," group=",stat_p.st_gid);

			if (create==true)
				fprintf(fp,"%s\n%i:%i:%i\n",buffer,stat_p.st_mode,stat_p.st_uid,stat_p.st_gid);

			if (S_ISDIR(stat_p.st_mode))
				{
				if (nolinks==true)
					{
					lstaterr=lstat(buffer,&linkstat);
					if (lstaterr==-1)
						{
						fprintf(stderr," Error occoured attempting to stat %s\n", dir_entry_p->d_name);
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
printf("Usage: perms [OPTION] [STARTDIRECTORY]\n"
	"Create a database of permissions, uids and guids for files stating at [STARTDIRECTORY]\n"
	" -c, --create	create a database\n"
	" -e, --exclude=[DIR] exclude a directory from the database, one --exclude for every folder FULL path names only, or '.' for current directory\n"
	" -f, --fix	when used with the --verify switch will attempt to repair the permissions, users and groups\n"
	" -h, -?, --help	print this help\n\n"
	" -l, --leave	go into other file systems, default is to stay on the same device\n"
	" -n, --nolinks	don't recurse into folders that are links\n"
	" -q, --quiet	only report errors\n"
	" -r, --repair	repair file permissions using the database in [STARTDIRECTORY] if given if not in the current directory\n"
	" -s, --strip=[DIR]	strip [DIR] out of the filepaths when creating, repairing or verifying a database and replace with '.' (for current directory)\n"
	" -u, --users	same as above but for the /home folder\n"
	" -v, --version	output version information and exit\n"
	" -S, --system	recurse into the system folders /sys,/proc,/dev, /tmp, /lost+found (Setting permissions on these files doesn't make a lot of sense as they are (mostly) recreated at boot time)\n"
	" -V, --verify	verify file permissions using the database in [STARTDIRECTORY] if given if not in the current directory\n"
	"[STARTDIRECTORY] is optional if not included the current directory is used\n\n"
	"Examples:\n\n"
	"List the permssions for all files in directory '/etc but not for /etc/webmin'\n"
	" perms -e /etc/webmin /etc\n"
	"Create a database in / including the /home folder\n"
	" sudo perms --create --users /\n"
	"Repair permissions for user 'someuser', obviously you must have created the database first!\n"
	" cd /home/someuser && sudo perms --repair\n"
	"Verify and fix  permissions for the current directory\n"
	" perms --verify --fix\n"
	"Repair permissions for the current directory\n"
	" perms --repair\n\n"
	"You may need to run perms as root to be able to read/repair certain folders.\n\n"
	"Stat errors for a file usually indicate a broken link, a deleted file (when repairing or verifying) or a circular link reference, ie /usr/bin/X11/X11 which is a symlink to /usr/bin/X11!\n\n"
	"Report bugs to kdhedger@yahoo.co.uk\n"
	);
}

int main(int argc, char **argv)
{

	int c;
	struct	stat	stat_p;

	while (1)
		{
		int option_index = 0;
		c = getopt_long (argc, argv, ":esp:qcrnuS?hlVfv",long_options, &option_index);
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
		
			case 'S':
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
		
		
			case 'V':
				verify=true;
				break;
		
			case 'v':
				printf("perms %s\n",VERSION);
				return 0;
				break;
		
			case 'f':
				fix=true;
				break;
		
			case 's':
				strip=true;
				stripbuffer=optarg;
				break;

			case 'p':
				prefix=true;
				strcpy(prefixbuffer,optarg);
				break;
		
			case '?':
			case 'h':
				printhelp();
				return 0;
				break;

			default:
				fprintf(stderr,"?? Unknown argument ??\n");
				return 1;
			break;
			}
		}
	
	if (optind < argc)
		{
		if (chdir(argv[optind]) == -1)
			{
			fprintf(stderr,"Can't CD into directory %s\n",argv[optind]);
			return 1;
			}
		}

	stat(".",&stat_p);
	thisdevice=stat_p.st_dev;
	getcwd(cwd,255);

	if (create==true)
		{
		fp = fopen(DBNAME,"w");
		if (fp==0)
			{
			fprintf(stderr,"Can't open DB file for writing\n");
			return 1;
			}

		fprintf(fp,"%s\n%i:%i:%i\n",".",stat_p.st_mode,stat_p.st_uid,stat_p.st_gid);
		}

	if (verify==true)
		{
		verifyperms();
		return 0;
		}

	printout("sosisin","Permissions for .=",stat_p.st_mode," owner=",stat_p.st_uid," group=",stat_p.st_gid);

	if (repair==true)
		{
		repairfiles();
		return 0;
		}

	if (strip==true)
		stripdirs((char*)&cwd[0]);

	if (prefix==true || strip==true)
		addprefix((char*)&cwd[0]);

	parsedir((char*)cwd);

	if (fp!=0)
		fclose(fp);

	return 0;
}

