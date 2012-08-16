/*----------------------------------------------------------------------*\

  filewatch.c

  Copyright (c) 2008-2009 Gregory C. Herlein.
  
  Written by Greg Herlein <gherlein@herlein.com>
  
------------------------------------------------------------------------*/

/*----------------------------< Defines >-------------------------------*/
#define DEBUG_PRINT
#define POLLING_METHOD

#define QUEUE_LEN 128
#define BUF_LEN   256
#define MAX_WATCH_FILES 16
#define FILEWATCH "filewatch"

/*----------------------------< Includes >------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <linux/inotify.h>

#include <libconfig.h>

/*--------------------------< Declarations >----------------------------*/
int ReadFileData(char* szIni,int num_files);
void print_event(struct inotify_event *event);
static void locallog(char* szMessage);
void        DoFileCommand(int x);

struct watch_list_t
{
  char         szFile[BUF_LEN];
  char         szCommand[BUF_LEN];
  char         szMD5[BUF_LEN];
  int          wd;
};
/*------------------------< Global Variables >--------------------------*/
struct watch_list_t     watch_list[MAX_WATCH_FILES];
/*-------------------------< Local Variables >--------------------------*/
/*----------------------------------------------------------------------*/
int main(int argc,char *argv[])
{
  int                     fd,wd,x;
  ssize_t                 len,i;
  struct inotify_event    equeue[QUEUE_LEN];
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

  config_init(&cfg);

  /* Read the file. If there is an error, report it and exit. */
  if(! config_read_file(&cfg, "example.cfg"))
  {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg));
    config_destroy(&cfg);
    return(EXIT_FAILURE);
  }


  /* Get the store name. */
  if(config_lookup_string(&cfg, "name", &str))
    printf("Store name: %s\n\n", str);
  else
    fprintf(stderr, "No 'name' setting in configuration file.\n");

  /* Output a list of all books in the inventory. */
  setting = config_lookup(&cfg, "inventory.books");
  if(setting != NULL)
  {
    int count = config_setting_length(setting);
    int i;

    printf("%-30s  %-30s   %-6s  %s\n", "TITLE", "AUTHOR", "PRICE", "QTY");

    for(i = 0; i < count; ++i)
    {
      config_setting_t *book = config_setting_get_elem(setting, i);

      /* Only output the record if all of the expected fields are present. */
      const char *title, *author;
      double price;
      int qty;

      if(!(config_setting_lookup_string(book, "title", &title)
           && config_setting_lookup_string(book, "author", &author)
           && config_setting_lookup_float(book, "price", &price)
           && config_setting_lookup_int(book, "qty", &qty)))
        continue;

      printf("%-30s  %-30s  $%6.2f  %3d\n", title, author, price, qty);
    }
    putchar('\n');
  }

  /* Output a list of all movies in the inventory. */
  setting = config_lookup(&cfg, "inventory.movies");
  if(setting != NULL)
  {
    unsigned int count = config_setting_length(setting);
    unsigned int i;

    printf("%-30s  %-10s   %-6s  %s\n", "TITLE", "MEDIA", "PRICE", "QTY");
    for(i = 0; i < count; ++i)
    {
      config_setting_t *movie = config_setting_get_elem(setting, i);

      /* Only output the record if all of the expected fields are present. */
      const char *title, *media;
      double price;
      int qty;

      if(!(config_setting_lookup_string(movie, "title", &title)
           && config_setting_lookup_string(movie, "media", &media)
           && config_setting_lookup_float(movie, "price", &price)
           && config_setting_lookup_int(movie, "qty", &qty)))
        continue;

      printf("%-30s  %-10s  $%6.2f  %3d\n", title, media, price, qty);
    }
    putchar('\n');
  }

  config_destroy(&cfg);
  return(EXIT_SUCCESS);
  
  
  if(argc>1)
  {
    szFile=argv[1];
  } else
  {
    printf("Use:  %s [ini filename]\n",argv[0]);
    exit(-1);
  }
  
#ifdef DAEMONIZE  
//  daemon();
#endif
  
  /* get the number of files to watch */
  x=ReadIniArg(szFile, FILEWATCH,"num_files",szBuffer,BUF_LEN);
  if(x)
  {
    num_watch_files=atoi(szBuffer);
  }
  
  /* get the sleep time  */
  x=ReadIniArg(szFile, FILEWATCH,"sleep_time",szBuffer,BUF_LEN);
  if(x)
  {
    sleep_time=atoi(szBuffer);
  }

  sprintf(szMessage,"watching %d files every %d seconds",
          num_watch_files,sleep_time);
  locallog(szMessage);
  

  
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

#ifdef EVENT_QUEUE_METHOD  
  while(1)
  {
    len = read (fd,equeue,QUEUE_LEN);
    i=0;
    while(i<len)
    {
      struct inotify_event *event = (struct inotify_event*) &equeue[i];
      if(event->mask && IN_CLOSE_WRITE)
      {
        print_event(event);
        for(x=0;x<MAX_WATCH_FILES;x++)
        {
          if(event->wd == watch_list[x].wd)
          {
            sprintf(szMessage,"file %s closed",watch_list[x].szFile);
            locallog(szMessage);

            /* check the MD5 to see if the file changed */
            MD5File(watch_list[x].szFile,szMD5new);
            nDiff=strcmp(watch_list[x].szMD5,szMD5new);
            if(nDiff)
            {
              DoFileCommand(x);
              strcpy(watch_list[x].szMD5,szMD5new);
            }
          }
        }
      }
      i++;
    }
    sleep(2);
  }
  inotify_rm_watch(fd);
#endif

#ifdef POLLING_METHOD
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
/*----------------------------------------------------------------------*/
void
DoFileCommand(int x)
{
  char                    szMessage[BUF_LEN];

  system(watch_list[x].szCommand);
  sprintf(szMessage,"executed: %s",watch_list[x].szCommand);
  locallog(szMessage);
}
/*----------------------------------------------------------------------*/
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



