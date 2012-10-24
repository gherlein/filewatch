/*----------------------------------------------------------------------*\

  filewatch.c

  Copyright (c) 2008-2012 Gregory C. Herlein.
  
  Written by Greg Herlein <gherlein@herlein.com>
  
------------------------------------------------------------------------*/

/*----------------------------< Includes >------------------------------*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <linux/inotify.h>
#include <sys/types.h>
#include <dirent.h>     
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#include <libconfig.h>
#include "md5.h"

/*----------------------------< Defines >-------------------------------*/
#define BUF_LEN          256
#define MAX_WATCH_FILES 1024

#define OPCODE_READ     1
#define OPCODE_WRITE    2

/*--------------------------< Declarations >----------------------------*/
static void locallog(char* szMessage);
int md5fileop(const char* filename,char* buffer, int opcode);
int doScript(const char* file,const char* script);
int genMD5(const char* dir, const char* file,const char* file5);
int scanDirectory(const char* dir,const char* pattern,const char* script);

/*------------------------< Global Variables >--------------------------*/

int                     daemon_mode=0;
char                    szMessage[BUF_LEN];

/*-------------------------< Local Variables >--------------------------*/
int main(int argc,char *argv[])
{
  char*                   szFile=NULL;
  int                     num_watch_files=0;
  int                     sleep_time=60;
  int                     run=1;
#if 0
  int                     fd,wd,x;
  ssize_t                 len,i;
  char                    szCommand[BUF_LEN];
  char                    szBuffer[BUF_LEN];
  int                     nDiff=0;
#endif
  
  config_t cfg;
  config_setting_t *setting;

  if(argc>1)
  {
    szFile=argv[1];
  } else
  {
    printf("Use:  %s [cfg-file]\n",argv[0]);
    exit(-1);
  }


  config_init(&cfg);

//  printf("reading config file [%s]\n",szFile);

  if(! config_read_file(&cfg,szFile))
  {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg));
    config_destroy(&cfg);
    return(EXIT_FAILURE);
  }

  setting=config_lookup(&cfg, "folders");
  int n=config_lookup_int(&cfg, "sleep_time", &sleep_time);
  if(n==0)
  {
    
  }
  n=config_lookup_int(&cfg, "daemon_mode", &daemon_mode);
  if(n==0)
  {
    
  }

  sprintf(szMessage,"watching every %d seconds",sleep_time);
  locallog(szMessage);

  if(daemon_mode==1)
  {
    /* Our process ID and Session ID */
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
      exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
      exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);

    /* Open any logs here */

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
      /* Log the failure */
      exit(EXIT_FAILURE);
    }

    /* Change the current working directory */
    if ((chdir("/")) < 0) {
      /* Log the failure */
      exit(EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
  }
  
  while(run)
  {
    if(setting != NULL)
    {
      num_watch_files = config_setting_length(setting);
      int i,n1,n2,n3;
    
      for(i = 0; i < num_watch_files; ++i)
      {
        config_setting_t *folder = config_setting_get_elem(setting, i);

        const char *path, *file, *script;
      
        n1=config_setting_lookup_string(folder, "path", &path);
      
        n2=config_setting_lookup_string(folder, "file", &file);
      
        n3=config_setting_lookup_string(folder, "script", &script);

//        printf(".");
        if(n1==0) printf("error n1\n");
        if(n2==0) printf("error n2\n");
        if(n3==0) printf("error n3\n");

//        printf("%-20s  %-20s  %-20s\n", path,file,script);

        scanDirectory(path,(const char*)file,(const char*)script);
      }
      putchar('\n');
    } else
    {
      printf("could not read config file\n");
    }

    sleep(sleep_time);
  }
  
  config_destroy(&cfg);
  return(EXIT_SUCCESS);
}
/*----------------------------------------------------------------------*/
static int
checkOpen(char* file)
{
  FILE *fp;
  char path[1035];
  char cmd[1035];

  sprintf(cmd,"/usr/bin/lsof |grep %s",file);

  fp = popen(cmd, "r");
  if (fp == NULL) {
    return 1;
  }

  while (fgets(path, sizeof(path)-1, fp) != NULL)
  {
    // any result means it's open
    return 1;
  }

  /* close */
  pclose(fp);
  
  return 0;
}
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
static void
locallog(char* szMessage)
{
  syslog(LOG_DAEMON|LOG_DEBUG,"filewatch: %s",szMessage);
  if(daemon_mode==0)
  {
    printf("%s\n",szMessage);
  }
  return;
}
/*----------------------------------------------------------------------*/
int
md5fileop(const char* filename,char* buffer, int opcode)
{
  FILE *file;
  unsigned long len;
  
  if(opcode==OPCODE_READ)
  {
    file = fopen(filename, "rb");
    if (!file)
    {
      return -1;
    }
    
    fseek(file, 0, SEEK_END);
    len=ftell(file);
    fseek(file, 0, SEEK_SET);
    
    fread(buffer, len, 1, file);
    fclose(file);
    
    return len;
  }
  if(opcode==OPCODE_WRITE)
  {
    file = fopen(filename, "wb");
    if (!file)
    {
      return -1;
    }
    
    len=strlen(buffer);
    fwrite(buffer, len, 1, file);
    fclose(file);
    
    return len;
    
  }
  
  return -1;
}
/*----------------------------------------------------------------------*/
int
doScript(const char* file,const char* script)
{
  char command[256];
  
  sprintf(command,"%s %s",script,file);
  sprintf(szMessage,"executing command line: [%s}",command);
  locallog(szMessage);
  int n=system(command);
  if(n==-1)
  {
    locallog("error executing command");
    return n;
  }
  else return n;
}
/*----------------------------------------------------------------------*/
int
genMD5(const char* dir, const char* file,const char* file5)
{
  char szMD5New[256];
  
  int len=MD5File((char*)file,szMD5New);
  if(len>0)
  {
    sprintf(szMessage,"new md5 [%s] generated for [%s]",szMD5New,file);
    locallog(szMessage);
    int md5len=md5fileop(file5,szMD5New,OPCODE_WRITE);
    return md5len;
  } else return -1;
}
/*----------------------------------------------------------------------*/
int
scanDirectory(const char* dir,const char* pattern,const char* script)
{
  DIR *dp;
  struct dirent *ep;
  char szMD5New[256];
  char szMD5Old[256];
  char szFile[256];
  char szFile5[256];

//  printf("scanning %s for %s...\n",dir,pattern);
  
  dp = opendir (dir);
  if (dp != NULL)
  {
    while ((ep = readdir (dp)))
    {
      int n=fnmatch (pattern, ep->d_name, 0);
      if(n==0)
      {
        // look to see if an md5 file exists
        sprintf(szFile,"%s/%s",dir,ep->d_name);
        sprintf(szFile5,"%s.%s",szFile,"md5");
        int md5len=md5fileop(szFile5,szMD5Old,OPCODE_READ);
        if(md5len>0)
        {
          // exists
          int len=MD5File(szFile,szMD5New);
          int match=strncmp(szMD5New,szMD5Old,len);
          if(match==0)
          {
#ifdef LOG_ALL
            sprintf(szMessage,"old md5 detected [%s] and matched for [%s]",
                    szMD5Old,szFile);
            locallog(szMessage);
#endif
          } else
          {
            // new md5 means file changed
            sprintf(szMessage,"new md5 [%s] for [%s]",
                    szMD5Old,szFile);
            locallog(szMessage);

            // if it's open, do nothing
            int n=checkOpen(szFile);
            if(n==0)
            {
              genMD5(dir,szFile,szFile5);
              doScript(szFile,script);
            }
          }
        } else
        {
          // did not exist
          sprintf(szMessage,"no old md5 for [%s]",szFile);
          locallog(szMessage);

          // if it's open, do nothing
          int n=checkOpen(szFile);
          if(n==0)
          {
            genMD5(dir,szFile,szFile5);
            doScript(szFile,script);
          }
        }
        
      }
    }
    (void) closedir (dp);
  }
  else
    perror ("Couldn't open the directory");
  
  return 0;
}
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
#ifdef INOTIFY
void print_event(struct inotify_event *event)
{
  if(event->len > 0)
  {
    printf("NAME: %s - ",event->name);
  }
  if(event->mask & IN_ACCESS) printf("IN_ACCESS ");
  if(event->mask & IN_MODIFY) printf("IN_MODIFY ");
  if(event->mask & IN_CLOSE_WRITE) printf("IN_CLOSE_WRITE ");
  if(event->mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
  if(event->mask & IN_OPEN) printf("IN_OPEN ");
  if(event->mask & IN_MOVED_FROM) printf("IN_MOVED_FROM ");
  if(event->mask & IN_MOVED_TO) printf("IN_MOVED_TO ");
  if(event->mask & IN_DELETE_SELF) printf("IN_DELETE_SELF ");
  if(event->mask & IN_MOVE_SELF) printf("IN_MOVE_SELF ");
  printf("\n");
  return;
}
#endif
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/



