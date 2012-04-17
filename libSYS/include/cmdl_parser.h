/**************************  Fraunhofer IIS FDK SysLib  **********************

                        (C) Copyright Fraunhofer IIS (1999)
                               All Rights Reserved

    Please be advised that this software and/or program delivery is
    Confidential Information of Fraunhofer and subject to and covered by the

    Fraunhofer IIS Software Evaluation Agreement
    between Google Inc. and  Fraunhofer
    effective and in full force since March 1, 2012.

    You may use this software and/or program only under the terms and
    conditions described in the above mentioned Fraunhofer IIS Software
    Evaluation Agreement. Any other and/or further use requires a separate agreement.


   $Id$
   Author(s):
   Description: command line parser

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

/** \file   cmdl_parser.h
 *  \brief  Command line parser.
 *
 *  The command line parser can extract certain data fields out of a character
 *  string and assign values to variables. It has 2 main functions. One to parse
 *  a command line in the form of standard C runtime "argc" and "argv" parameters,
 *  and the other to assemble these parameters reading text lines from a file in
 *  case the C runtime does not provide them.
 */

#ifndef  __PARSER_H
#define __PARSER_H



#include "machine_type.h"

#define CMDL_MAX_STRLEN 255
#define CMDL_MAX_ARGC    30

/* \cond */
/* Type definition for text */


#ifdef WIN32
  #include <tchar.h>
  #ifndef _tstof  /* For Visual Studio 6 */
    #ifdef _UNICODE
      #include <wchar.h>
      #define _tstof(x)   (float) wcstod(x, NULL)  /* For Visual Studio 6 */
    #else
      #define _tstof      atof
    #endif
  #endif

  #ifndef _tstol  /* For Visual Studio 6 */
    #ifdef _UNICODE
      #define _tstol      _wtol
    #else
      #define _tstol      atol
    #endif
  #endif

  #ifndef _tstoi  /* For Visual Studio 6 */
    #ifdef _UNICODE
      #define _tstoi      _wtoi
    #else
      #define _tstoi      atoi
    #endif
  #endif

  #ifndef TEXTCHAR
    #define TEXTCHAR char
  #endif

  #ifndef _TEXT
    #define _TEXT
  #endif

#else /* WIN32 */

    #define TEXTCHAR char
    #define _tcslen(a)  FDKstrlen(a)
    #define _tcscpy     strcpy
    #define _tcscmp     FDKstrcmp
    #define _tcsncmp    FDKstrncmp
    #define _tscanf     scanf
    #define _TEXT(x)    x
    #define _tfopen     fopen
    #define _ftprintf   fprintf
    #define _tcsncpy    FDKstrncpy
    #define _tstof      FDKatof
    #define _tstol      FDKatol
    #define _tstoi      FDKatoi
    #define _tcstol     strtol
    #define _istdigit   isdigit
#endif /* WIN32 */

/* \endcond */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 *  Scans argc, argv and a scanf style format string for parameters and stores the
 *  values in the variable number of pointers passed to the function.

 For example:
   \code
   #define ARG_PARAM  "(-a %d) (-v %1)"
   #define ARG_VALUE &config->aot, &verbose
   int nFoundArgs = IIS_ScanCmdl(argc, argv, ARG_PARAM, ARG_VALUE);
   \endcode
   wheras the wild-cards (\%d, \%1, ..) define the data type of the argument:
	- \%1 boolean (e. g. -x)
	- \%d integer (e. g. -x 23)
	- \%f float (e. g. -x 3.4)
	- \%y double (e. g. -x 31415926535897932384626433832795028841971693993751)
	- \%s string (e. g. -x "file.dat")
	- \%u unsigned character (e. g. -x 3)
	- \%c signed character (e. g. -x -3)
    More examples on how to use it are located in every (encoder/decoder) example framework.

 * \param argc      Number of arguments.
 * \param argv      Complete character string of the command line arguments.
 * \param pReqArgs  A list of parameters and a corresponding list of memory addresses to
 *                  assign each parameter to.
 *
 * \return  Number of found arguments.
 */
INT IIS_ScanCmdl(INT argc, TEXTCHAR* argv[], const TEXTCHAR* pReqArgs, ...);

#ifdef __cplusplus
}
#endif

/**
 *  Reads a text file, assembles argc and argv parameters for each text line
 *  and calls the given function for each set of argc, argv parameters.
 *
 * \param param_filename  Name of text file that should be parsed.
 * \param pFunction       Pointer to function that should be called for every text line found.
 *
 * \return  0 on success, 1 on failure.
 */
INT IIS_ProcessCmdlList(const TEXTCHAR* param_filename, int (*pFunction)(int, TEXTCHAR**));


#endif /* __PARSER_H */
