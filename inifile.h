/**************************************************************************
  File:      inifile.h
  
  Purpose:	Functions for easy reading and writing of
                system initialization files    
  
  Copyright 1998-2009 Herlein Gregory C. Herlein

  **************************************************************************/

#ifndef _INIFILE_H
#define _INIFILE_H

int ReadIniArg(char* szFileName, const char* szSection,
		       const char* szParam,char* szBuffer, int nLen);


#endif /* _INIFILE_H */



