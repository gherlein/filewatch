/*----------------------------------------------------------------------*\

  File:         inifile.c
  
  Basic functions to read/write from an ini file.
  
  Author:   Greg Herlein
  
  Copyright 1998-2009 Gregory C. Herlein

\*----------------------------------------------------------------------*/
/*----------------------------< Defines >-------------------------------*/
/*----------------------------< Includes >------------------------------*/
/* std lib */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* other lib */
#include "inifile.h"
/*---------------------------< Definitions >----------------------------*/
/*--------------------------< Declarations >----------------------------*/
/*------------------------< Global Variables >--------------------------*/
/*-------------------------< Local Variables >--------------------------*/
/*----------------------------------------------------------------------*/
int
ReadIniArg(char* szFileName, const char* szSection,
		       const char* szParam,char* szBuffer, int nLen)
{
  /* returns the number of characters copied into the buffer if a match
     is found, otherwise returns a 0 */
  FILE*		fpIn = NULL;
  char 		szTemp[128];
  char*		p = NULL;
  
  /* clean buffer and open ini file */
  memset(szTemp,0x00, sizeof(szTemp));
  fpIn = fopen(szFileName,"r");
  if (fpIn == NULL) {
    return 0;
  }
  
  /* skip to the appropriate section */
  while(1) {
    p=fgets(szTemp,sizeof(szTemp),fpIn); 
    /* if an error occured */
    if(p==NULL) { 
      if(feof(fpIn)!=0) {
        fclose(fpIn);
        return 0;
      } else
      fclose(fpIn);
      return 0;
    } /* end of error condition code */

    /* find the szSection */
    p=strstr(szTemp,szSection);
    if (p!=NULL) {
      break;
    }
    else continue;
  } 
  
  /*** now find the szParam ***/
  while (1) {
    p=fgets(szTemp,sizeof(szTemp),fpIn);
    /* if an error occured */
    if(p==NULL) {
      if(feof(fpIn)!=0) {
        fclose(fpIn);
        return 0;
      }
    } 

    /* ensure there are no \r's or \n's left */
    p=strchr(szTemp,'\r');
    if (p!=NULL) p[0]=0x00;
    p=strchr(szTemp,'\n');
    if (p!=NULL) p[0]=0x00;

    /* find the '=' */
    p=strchr(szTemp,'=');
    if(p==NULL) continue;
    else p[0]=0x00;

    /* compare the string to the passed in szParam */
    if(strcmp(szParam,szTemp)!=0) continue;
      
    /* now start at p+1 */
    p++;
    strncpy(szBuffer,p,nLen);
    fclose(fpIn);
    return nLen;
  } 
  return 0; /* just in case - should never get here */
}
/*----------------------------------------------------------------------*/
  








