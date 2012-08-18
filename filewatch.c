/*----------------------------------------------------------------------*\

  filewatch.c

  Copyright (c) 2008-2009 Gregory C. Herlein.
  
  Written by Greg Herlein <gherlein@herlein.com>
  
------------------------------------------------------------------------*/

/*----------------------------< Defines >-------------------------------*/
#define DEBUG_PRINT
#define POLLING_METHOD

//#define QUEUE_LEN 128
#define BUF_LEN   256
#define MAX_WATCH_FILES 1024

#define OPCODE_READ  1
#define OPCODE_WRITE 2

/*----------------------------< Includes >------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <linux/inotify.h>
#include <sys/types.h>
#include <dirent.h>     
#include <fnmatch.h>

#include <libconfig.h>

/*--------------------------< Declarations >----------------------------*/
//int ReadFileData(char* szIni,int num_files);
void print_event(struct inotify_event *event);
static void locallog(char* szMessage);
void        DoFileCommand(int x);

struct watch_list_t
{
  char         szFile[BUF_LEN];
  char         szCommand[BUF_LEN];
  char         szMD5[BUF_LEN];
};
/*------------------------< Global Variables >--------------------------*/

struct watch_list_t     watch_list[MAX_WATCH_FILES];

/*-------------------------< Local Variables >--------------------------*/
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
scanDirectory(const char* dir,const char* pattern)
{
  DIR *dp;
  struct dirent *ep;
  char szMD5New[256];
  char szMD5Old[256];
  char szFile[256];
  char szFile5[256];
  
  dp = opendir (dir);
  if (dp != NULL)
  {
    while (ep = readdir (dp))
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
            printf("old md5 detected and matched: [%s]\n",szMD5Old);
          } else
          {
            printf("NEW md5!\n");
          }
        } else
        {
          // did not exist
          int len=MD5File(szFile,szMD5New);
          printf("new md5 generated: [%s]\n",szMD5New);
          int md5len=md5fileop(szFile5,szMD5New,OPCODE_WRITE);
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

int main(int argc,char *argv[])
{
  int                     fd,wd,x;
  ssize_t                 len,i;
//  struct inotify_event    equeue[QUEUE_LEN];
  struct watch_list_t*    pwatch_list = watch_list;
  char                    szCommand[BUF_LEN];
  char                    szMessage[BUF_LEN];
  char*                   szFile=NULL;
  char                    szBuffer[BUF_LEN];
  int                     num_watch_files=0;
  int                     nDiff=0;
  char                    szMD5new[BUF_LEN];
  int                     sleep_time=60;

  config_t cfg;
  config_setting_t *setting;
  const char *str;

  if(argc>1)
  {
    szFile=argv[1];
  } else
  {
    printf("Use:  %s [cfg-file]\n",argv[0]);
    exit(-1);
  }
  
  config_init(&cfg);

  printf("reading config file [%s]\n",szFile);

  if(! config_read_file(&cfg,szFile))
  {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg));
    config_destroy(&cfg);
    return(EXIT_FAILURE);
  }

  setting=config_lookup(&cfg, "folders");
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

      printf(".");
      if(n1==0) printf("error n1\n");
      if(n2==0) printf("error n2\n");
      if(n3==0) printf("error n3\n");
      

      printf("%-20s  %-20s  %-20s\n", path,file,script);

      scanDirectory(path,(const char*)file);


    }
    putchar('\n');
  } else
  {
    printf("could not read config file\n");
  }

  int n=config_lookup_int(&cfg, "sleep_time", &sleep_time);
  
  
#ifdef DAEMONIZE  
//  daemon();
#endif
  
  sprintf(szMessage,"watching %d folders every %d seconds",
          num_watch_files,sleep_time);
  locallog(szMessage);



  
  config_destroy(&cfg);
  return(EXIT_SUCCESS);
  
#ifdef OLDWAY
  /* set up the basic paramters */
  ReadFileData(szFile,num_watch_files);
  memset(equeue,0,sizeof(struct inotify_event)*QUEUE_LEN);
  fd=inotify_init();
  if(fd==-1)
  {
    perror("inotify_init");
    exit(EXIT_FAILURE);
  }

  /* do the work */
  for(x=0;x<num_watch_files;x++)
  {
    watch_list[x].wd=inotify_add_watch(fd,
                                       watch_list[x].szFile,
                                       IN_CLOSE_WRITE);

    sprintf(szMessage,"watching %s",watch_list[x].szFile);
    locallog(szMessage);
  }

  while(1)
  {
    sleep(sleep_time);
    for(x=0;x<num_watch_files;x++)
    {
     /* check the MD5 to see if the file changed */
      MD5File(watch_list[x].szFile,szMD5new);
      nDiff=strcmp(watch_list[x].szMD5,szMD5new);
      if(nDiff)
      {
        DoFileCommand(x);
        strcpy(watch_list[x].szMD5,szMD5new);
        sprintf(szMessage,"file %s new MD5: %s",
                watch_list[x].szFile,
                watch_list[x].szMD5);
        locallog(szMessage);
      }
    }
  }
#endif
  
}
void
DoFileCommand(int x)
{
  char                    szMessage[BUF_LEN];

  system(watch_list[x].szCommand);
  sprintf(szMessage,"executed: %s",watch_list[x].szCommand);
  locallog(szMessage);
}
/*----------------------------------------------------------------------*/
#ifdef OLDWAY
int
ReadFileData(char* szIni, int num_files)
{
  char szBase[]="file_";
  char szFile[16];
  int  x=0;
  int  n=0;

  num_files++;
  for(x=0;x<num_files;x++)
  {
    sprintf(szFile,"%s%d",szBase,x+1);
    n=ReadIniArg(szIni,szFile,"filename",
                 watch_list[x].szFile,
                 BUF_LEN);
    if(n=0) return -1;
      
    n=ReadIniArg(szIni,szFile,"script",
                 watch_list[x].szCommand,
                 BUF_LEN);
    if(n==0) return -1;

    MD5File(watch_list[x].szFile,watch_list[x].szMD5);

#ifdef DEBUG_PRINT
    printf("%d: %s [%s] {%s}\n",
           x,
           watch_list[x].szFile,
           watch_list[x].szCommand,
           watch_list[x].szMD5);
#endif
    
  }
  return x+1;
}
#endif
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
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
/*----------------------------------------------------------------------*/
static void
locallog(char* szMessage)
{
  syslog(LOG_DAEMON|LOG_DEBUG,"filewatch: %s",szMessage);
#ifdef DEBUG_PRINT
  printf("%s\n",szMessage);
#endif  
  
  return;
}
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/



