#ifdef OLD_WAY
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

  strcpy(watch_list[4].szFile,"/home/dbf/ZIMBRA.DBF");
  sprintf(watch_list[4].szCommand,"/usr/local/bin/zimbra-contacts.sh %s",
          watch_list[4].szFile);
#endif

