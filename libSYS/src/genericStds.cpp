/**************************  Fraunhofer IIS FDK SysLib  **********************

                        (C) Copyright Fraunhofer IIS (2002)
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
   Description: - Generic memory, stdio, string, etc. function wrappers or
                  builtins.
                - OS dependant function wrappers.

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include "genericStds.h"

#include <math.h>

/* library info */
#define SYS_LIB_VL0 1
#define SYS_LIB_VL1 1
#define SYS_LIB_VL2 25
#define SYS_LIB_TITLE "System Integration Library"
#define SYS_LIB_BUILD_DATE __DATE__
#define SYS_LIB_BUILD_TIME __TIME__

  #include <stdlib.h>
  #include <stdio.h>
  #include <string.h>
    #include <stdarg.h>


/***************************************************************
 * memory allocation monitoring variables
 ***************************************************************/


/* Include OS/System specific implementations. */
#if defined(__linux__)	/* cppp replaced: elif */
  #include "linux/genericStds_linux.cpp"
#endif


#if !(defined(USE_BUILTIN_STRING_FUNCTIONS) || defined(__SYMBIAN32__))
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef FUNCTION_FDKprintf
void  FDKprintf( const char* szFmt, ...)    {
  va_list ap;
  va_start(ap, szFmt);
  vprintf(szFmt, ap);
  va_end(ap);
#ifdef ARCH_WA_FLUSH_CONSOLE
  fflush(stdout);
#endif
}
#endif

#ifndef FUNCTION_FDKprintfErr
void  FDKprintfErr( const char* szFmt, ...) {
  va_list ap;
  va_start(ap, szFmt);
#if defined(ARCH_WA_SLOWCON)
  vprintf(szFmt, ap);
#else
  vfprintf(stderr, szFmt, ap);
#endif
  va_end(ap);
#ifdef ARCH_WA_FLUSH_CONSOLE
  fflush(stderr);
#endif
}
#endif

int FDKgetchar(void) { return getchar(); }

INT  FDKfprintf(FDKFILE  *stream,  const  char *format, ...) {
  INT chars = 0;
  va_list ap;
  va_start(ap, format);
  chars += vfprintf((FILE*)stream, format, ap);
  va_end(ap);
  return chars;
}

#ifndef FUNCTION_FDKsprintf
INT  FDKsprintf(char *str, const char *format, ...) {
  INT chars = 0;
  va_list ap;
  va_start(ap, format);
  chars += vsprintf(str, format, ap);
  va_end(ap);
  return chars;
}
#endif

#else

void FDKprintf( const char* szFmt, ...) { /* stub! */; }
void FDKprintfErr( const char* szFmt, ...) { /* stub! */; }
INT  FDKfprintf(FILE  *stream,  const  char *format, ...) { /*stub ! */; }
INT  FDKsprintf(char *str, const char *format, ...) { /* stub! */; }

#endif

/************************************************************************************************/


const char *FDKstrchr(const char *s, INT c)                       { return strchr(s, c); }
const char *FDKstrstr(const char *haystack, const char *needle)   { return strstr(haystack, needle); }
#ifndef FUNCTION_FDKstrcpy
char *FDKstrcpy(char *dest, const char *src)                      { return strcpy(dest, src); }
#endif
char *FDKstrncpy(char *dest, const char *src, UINT n)             { return strncpy(dest, src, n); }

/*************************************************************************
 * DYNAMIC MEMORY management (heap)
 *************************************************************************/

#ifndef FUNCTION_FDKcalloc
void *FDKcalloc (const UINT n, const UINT size)
{
  void* ptr;

  ptr = calloc(n, size);

  return ptr;
}
#endif

#ifndef FUNCTION_FDKmalloc
void *FDKmalloc (const UINT size)
{
  void* ptr;

  ptr = malloc(size);

  return ptr;
}
#endif

#ifndef FUNCTION_FDKfree
void  FDKfree (void *ptr)
{
  /* FDKprintf("f, heapSize: %d\n", heapSizeCurr); */
  free((INT*)ptr);
}
#endif

#ifndef FUNCTION_FDKaalloc
void *FDKaalloc(const UINT size, const UINT alignment)
{
  void *addr, *result=NULL;
  addr = FDKcalloc(1, size + alignment + sizeof(void*));               /* Malloc and clear memory.         */

  if (addr!=NULL)
  {
    result = ALIGN_PTR((unsigned char*)addr + sizeof(void*));    /* Get aligned memory base address. */
    *(((void**)result) - 1) = addr;                /* Save malloc'ed memory pointer.   */
  }

  return result;                                 /* Return aligned address.          */
}
#endif

#ifndef FUNCTION_FDKafree
void  FDKafree (void *ptr)
{
  void *addr;
  addr = *(((void**)ptr)-1); /* Get pointer to malloc'ed memory. */
  FDKfree(addr);                /* Free malloc'ed memory area.      */
}
#endif


#ifndef FUNCTION_FDKcalloc_L

/*--------------------------------------------------------------------------*
 * DATA MEMORY L1/L2 (fallback)
 *--------------------------------------------------------------------------*/



/*--------------------------------------------------------------------------*
 * FDKcalloc_L
 *--------------------------------------------------------------------------*/
void *FDKcalloc_L(const UINT dim, const UINT size, MEMORY_SECTION s)
{
  int a_size;

  if (s == SECT_DATA_EXTERN)
    goto fallback;

  a_size = ((dim*size+3)&0xfffffffc); /* force 4 byte alignment (1111 .... 1111 1100) */





  //printf("Warning, out of internal memory\n");

fallback:
  return FDKcalloc(dim, size);
}
#endif /* FUNCTION_FDKcalloc_L */

#ifndef FUNCTION_FDKfree_L
void  FDKfree_L (void *p)
{

    FDKfree(p);
}
#endif /* FUNCTION_FDKfree_L */

#ifndef FUNCTION_FDKaalloc_L
void *FDKaalloc_L(const UINT size, const UINT alignment, MEMORY_SECTION s)
{
  void *addr, *result=NULL;
  addr = FDKcalloc_L(1, size + alignment + sizeof(void*), s);       /* Malloc and clear memory.         */

  if (addr!=NULL)
  {
    result = ALIGN_PTR((unsigned char *)addr + sizeof(void*));    /* Get aligned memory base address. */
    *(((void**)result) - 1) = addr;                /* Save malloc'ed memory pointer.   */
  }

  return result;                                 /* Return aligned address.          */
}
#endif

#ifndef FUNCTION_FDKafree_L
void  FDKafree_L (void *ptr)
{
  void *addr;

  addr = *(((void**)ptr)-1); /* Get pointer to malloc'ed memory. */
  FDKfree_L(addr);                /* Free malloc'ed memory area.      */
}
#endif



/*---------------------------------------------------------------------------------------
 * FUNCTION:    FDKmemcpy
 * DESCRIPTION: - copies memory from "src" to "dst" with length "size" bytes
 *              - compiled with FDK_DEBUG will give you warnings
 *---------------------------------------------------------------------------------------*/
void FDKmemcpy(void *dst, const void *src, const UINT size)
{

  /* do the copy */
  memcpy(dst, src, size);
}

void FDKmemmove(void *dst, const void *src, const UINT size)     { memmove(dst, src, size); }
void FDKmemset(void *memPtr, const INT value, const UINT size)   { memset(memPtr, value, size); }
void FDKmemclear(void *memPtr, const UINT size)                  { FDKmemset(memPtr,0,size); }
UINT FDKstrlen(const char *s)                                    { return (UINT)strlen(s); }

/* Compare function wrappers */
INT FDKmemcmp(const void *s1, const void *s2, const UINT size)  { return memcmp(s1, s2, size); }
INT FDKstrcmp(const char *s1, const char *s2)                   { return strcmp(s1, s2); }
INT FDKstrncmp(const char *s1, const char *s2, const UINT size) { return strncmp(s1, s2, size); }


/* Math function wrappers. Only intended for compatibility, not to be highly optimized. */

INT FDKabs(INT j) { return abs(j); }
double FDKfabs(double x) { return fabs(x); }
double FDKpow(double x, double y) { return pow(x,y); }
double FDKsqrt(double x) { return sqrt(x); }
double FDKatan(double x) { return atan(x); }
double FDKlog(double x) { return log(x); }
double FDKsin(double x) { return sin(x); }
double FDKcos(double x) { return cos(x); }
double FDKexp(double x) { return exp(x); }
double FDKatan2(double y, double x) { return atan2(y, x); }
double FDKacos(double x) { return acos(x); }
double FDKtan(double x) { return tan(x); }
double FDKfloor(double x) { return floor(x); }
double FDKceil(double x) { return ceil(x); }

INT   FDKatoi(const char *nptr) { return atoi(nptr); }
long  FDKatol(const char *nptr) { return atol(nptr); }
float FDKatof(const char *nptr) { return (float)atof(nptr); }


/* ==================== FILE I/O ====================== */

#if !defined(FUNCTION_FDKfopen)
FDKFILE *FDKfopen(const char *filename, const char *mode) { return fopen(filename, mode); }
#endif
#if !defined(FUNCTION_FDKfclose)
INT FDKfclose(FDKFILE *fp) { return fclose((FILE*)fp);}
#endif
#if !defined(FUNCTION_FDKfseek)
INT FDKfseek(FDKFILE *fp, LONG OFFSET, int WHENCE) { return fseek((FILE*)fp, OFFSET, WHENCE);}
#endif
#if !defined(FUNCTION_FDKftell)
INT FDKftell(FDKFILE *fp) { return ftell((FILE*)fp); }
#endif
#if !defined(FUNCTION_FDKfflush)
INT FDKfflush(FDKFILE *fp) { return fflush((FILE*)fp); }
#endif
const INT FDKSEEK_SET = SEEK_SET;
const INT FDKSEEK_CUR = SEEK_CUR;
const INT FDKSEEK_END = SEEK_END;

#if !defined(FUNCTION_FDKfwrite)
UINT FDKfwrite(void *ptrf, INT size, UINT nmemb, FDKFILE *fp) { return fwrite(ptrf, size, nmemb, (FILE*)fp); }
#endif
#if !defined(FUNCTION_FDKfread)
UINT FDKfread(void *dst, INT size, UINT nmemb, FDKFILE *fp) { return fread(dst, size, nmemb, (FILE*)fp); }
#endif
#if !defined(FUNCTION_FDKfgets)
char* FDKfgets(void *dst, INT size, FDKFILE *fp) { return fgets((char *)dst, size, (FILE*)fp); }
#endif
#if !defined(FUNCTION_FDKrewind)
void FDKrewind(FDKFILE *fp) { FDKfseek((FILE*)fp,0,FDKSEEK_SET); }
#endif


UINT FDKfwrite_EL(void *ptrf, INT size, UINT nmemb, FDKFILE *fp) {

    if (IS_LITTLE_ENDIAN()) {
      FDKfwrite(ptrf, size, nmemb, fp);
    } else {
      UINT n;
      INT s;

      UCHAR *ptr = (UCHAR*) ptrf;

      for (n=0; n<nmemb; n++) {
        for (s=size-1; s>=0; s--) {
          //FDKprintf("char = %c\n", (char)*(ptr+s));
          FDKfwrite(ptr + s, 1, 1, fp);
        }
        ptr = ptr + size;
      }
    }
    return nmemb;
}


UINT FDKfread_EL(void *dst, INT size, UINT nmemb, FDKFILE *fp) {
  UINT n, s0, s1, err;
  UCHAR tmp, *ptr;
  UCHAR tmp24[3];

  /* Enforce alignment of 24 bit data. */
  if (size == 3) {
    ptr = (UCHAR*)dst;
    err = 0;
    for (n=0; n<nmemb; n++) {
      if ((err = FDKfread(tmp24, 1, 3, fp)) != 3) {
        return err;
      }
      *ptr++ = tmp24[0];
      *ptr++ = tmp24[1];
      *ptr++ = tmp24[2];
      /* Sign extension */
      if (tmp24[2] & 0x80) {
        *ptr++ = 0xff;
      } else {
        *ptr++ = 0;
      }
    }
    err = nmemb;
    size = sizeof(LONG);
  } else {
    if ((err = FDKfread(dst, size, nmemb, fp)) != nmemb) {
      return err;
    }
  }
  if (!IS_LITTLE_ENDIAN() && size > 1) {
    ptr = (UCHAR*)dst;
    for (n=0; n<nmemb; n++) {
      for (s0=0, s1=size-1; s0 < s1; s0++, s1--) {
        tmp = ptr[s0];
        ptr[s0] = ptr[s1];
        ptr[s1] = tmp;
      }
      ptr += size;
    }
  }
  return err;
}

INT FDKfeof(FDKFILE *fp) { return feof((FILE*)fp); }

/* Global initialization/cleanup */

#if defined(_DEBUG) && defined(_WIN32) && !defined(_WIN32_WCE)
  #define _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>
#endif




void FDKprintDisclaimer(void)
{
  FDKprintf(
  "This program is protected by copyright law and international treaties.\n"  \
  "Any reproduction or distribution of this program, or any portion\n"        \
  "of it, may result in severe civil and criminal penalties, and will be\n"   \
  "prosecuted to the maximum extent possible under law.\n\n");
}

