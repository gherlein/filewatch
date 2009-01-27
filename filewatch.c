/*----------------------------------------------------------------------*\

  gch_filewatch.c

  Copyright (c) 2008 Gregory C. Herlein.
  
  Written by Greg Herlein <gherlein@herlein.com>
  
------------------------------------------------------------------------*/

/*----------------------------< Defines >-------------------------------*/
#define QUEUE_LEN 128
#define MAX_WATCH_FILES 4
/*----------------------------< Includes >------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <linux/inotify.h>

/*--------------------------< Declarations >----------------------------*/
struct watch_list_t
{
  char         szFile[128];
  char         szCommand[128];
  int          wd;
};
/*------------------------< Global Variables >--------------------------*/
struct watch_list_t     watch_list[MAX_WATCH_FILES];
/*-------------------------< Local Variables >--------------------------*/
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
  syslog(LOG_DAEMON|LOG_DEBUG,szMessage);
  return;
}
/*----------------------------------------------------------------------*/
int main(int argc,char *argv[])
{
  int                     fd,wd,x;
  ssize_t                 len,i;
  struct inotify_event    equeue[QUEUE_LEN];
  struct watch_list_t*    pwatch_list = watch_list;
  char                    szCommand[256];
  char                    szMessage[256];
  
  daemon();
  
  memset(equeue,0,sizeof(struct inotify_event)*QUEUE_LEN);

  strcpy(watch_list[0].szFile,"/home/dbf/FORMDATA.DBF");
  sprintf(watch_list[0].szCommand,"/usr/local/bin/watchfile.pl %s",
          watch_list[0].szFile);

  strcpy(watch_list[1].szFile,"/home/dbf/INFRFILE.DBF");
  sprintf(watch_list[1].szCommand,"/usr/local/bin/watchfile.pl %s",
          watch_list[1].szFile);

  strcpy(watch_list[2].szFile,"/home/dbf/SITEFILE.DBF");
  sprintf(watch_list[2].szCommand,"/usr/local/bin/watchfile.pl %s",
          watch_list[2].szFile);

  strcpy(watch_list[3].szFile,"/home/dbf/TENTFILE.DBF");
  sprintf(watch_list[3].szCommand,"/usr/local/bin/watchfile.pl %s",
          watch_list[3].szFile);

  fd=inotify_init();
  if(fd==-1)
  {
    perror("inotify_init");
    exit(EXIT_FAILURE);
  }


  for(x=0;x<MAX_WATCH_FILES;x++)
  {
    watch_list[x].wd=inotify_add_watch(fd,
                                       watch_list[x].szFile,
                                       IN_CLOSE_WRITE);

    sprintf(szMessage,"watching %s",watch_list[x].szFile);
    locallog(szMessage);
  }
  

  while(1)
  {
    len = read (fd,equeue,QUEUE_LEN);
    i=0;
    while(i<len)
    {
      struct inotify_event *event = (struct inotify_event*) &equeue[i];
      if(event->mask && IN_CLOSE_WRITE)
      {
//      print_event(event);
        for(x=0;x<MAX_WATCH_FILES;x++)
        {
          if(event->wd == watch_list[x].wd)
          {
//          printf("%s COMMAND: %s\n",pwatch_list->szFile,pwatch_list->szCommand);
            system(watch_list[x].szCommand);
            sprintf(szMessage,"executed: %s",watch_list[x].szCommand);
            locallog(szMessage);
          }
        }
      }
      i++;
    }
  }
  inotify_rm_watch(fd);
}
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/



