
/* -----------------------------------------------------------------------------
Software Copyright License for The Fraunhofer FDK Extended High Efficiency AAC
Encoder Software for Android

© Copyright 1995 - 2025 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1.    INTRODUCTION

The Fraunhofer FDK Extended High Efficiency AAC Encoder Software for Android
("FDK Extended High Efficiency AAC Encoder") is software that implements the
encoding of digital audio according to the MPEG-D Unified Speech and Audio
Coding (USAC) standard and MPEG-D Dynamic Range Control (DRC) standard. This FDK
Extended High Efficiency AAC Encoder Software is intended to be used on a wide
variety of Android devices. It is technically not suited to encode content for
digital radio broadcasting services, including DRM and similar standards.

Patent licenses for necessary patent claims for the FDK Extended High Efficiency
AAC Encoder Software (including those of Fraunhofer), for the use in commercial
products and services, may be obtained from the respective patent owners
individually and/or from Via Licensing Alliance (www.via-la.com).

Fraunhofer supports the development of Extended High Efficiency AAC products and
services by offering additional software, documentation, and technical advice.
In addition, it operates the xHE-AAC Trademark Program to ease interoperability
testing of end products. Please visit http://www.xhe-aac.com for more
information.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees, provided that you
satisfy the following conditions:

You must retain the complete text of this software license in redistributions of
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto in source code form.

You must retain the complete text of this software license in the documentation
and/or other materials provided with redistributions of the FDK Extended High
Efficiency AAC Encoder Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the
FDK Extended High Efficiency AAC Encoder Software and your modifications thereto
to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived
from this software without prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto.

Your modified versions of the FDK Extended High Efficiency AAC Encoder Software
must carry prominent notices stating that you changed the software and the date
of any change. For modified versions of the FDK Extended High Efficiency AAC
Encoder Software, the term "Fraunhofer FDK Extended High Efficiency AAC Encoder
Software for Android" must be replaced by the term "Third-Party Modified Version
of the Fraunhofer FDK Extended High Efficiency AAC Encoder Software for
Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty for patent non-infringement with respect to this
software. You may use this FDK Extended High Efficiency AAC Encoder Software or
modifications thereto only for purposes that are authorized by appropriate
patent licenses.

4.    DISCLAIMER

This FDK Extended High Efficiency AAC Encoder Software is provided by Fraunhofer
on behalf of the copyright holders and contributors "AS IS" and WITHOUT ANY
EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied
warranties of merchantability and fitness for a particular purpose. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if advised
of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - FDK Extended High Efficiency
AAC Encoder
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "IISutillib/ngsalloc.h"
#include "IISutillib/errorhnd.h"

static struct ERROR_INFO ERROR_ERRORHND_NOMEMORY = {
    "errorhnd.c", 0, "error()", "errorhnd.c: the error handler has run out of memory", 0};

void ERR_NORETURN _abort(char *text, const char *file, const int line, const char *function) {
  (void)text;
  (void)file;
  (void)line;
  (void)function;

  exit(20);
}

static char *mystrdup(const char *c) {
  char *s;
  unsigned int n;

  if (!c) return 0;
  n = (unsigned int)strlen(c);
  s = (char *)iisMalloc(n + 1);
  if (s) strcpy(s, c);
  return s;
}

HANDLE_ERROR_INFO iisUtil_ERROR(const char *file, const int line, const char *function, const char *fmt, ...) {
  const char *fn;
  char tmp[512];
  unsigned int errLen;
  HANDLE_ERROR_INFO e;

  va_list ap;

  e = (HANDLE_ERROR_INFO)iisCalloc(sizeof(struct ERROR_INFO), 1);
  if (!e) {
    return (&ERROR_ERRORHND_NOMEMORY);
  }

  va_start(ap, fmt);

  vsprintf(tmp, fmt, ap);
  errLen = (unsigned int)strlen(tmp) + 1;

  e->errText = (char *)iisMalloc(errLen);
  if (!e->errText) {
    iisFree(e);
    va_end(ap);
    return (&ERROR_ERRORHND_NOMEMORY);
  }

  if (file) {
#if (defined(WIN32) || defined(WIN64))
    fn = strrchr(file, '\\');
#else
    fn = strrchr(file, '/');
#endif
    if (fn)
      fn++;
    else
      fn = file;
  } else {
    fn = file;
  }

  e->module = mystrdup(fn);
  e->lineNo = line;
  e->procedure = mystrdup(function);

  strcpy(e->errText, tmp);

  va_end(ap);

  return (e);
}

HANDLE_ERROR_INFO _handBack(const char *file, const int line, const char *function,
                            HANDLE_ERROR_INFO e) {
  HANDLE_ERROR_INFO e1;

  if (e == 0) return e;

  e1 = (HANDLE_ERROR_INFO)iisCalloc(sizeof(struct ERROR_INFO), 1);
  if (e1) {
    HANDLE_ERROR_INFO e2;
    const char *fn;

    if (file) {
#if (defined(WIN32) || defined(WIN64))
      fn = strrchr(file, '\\');
#else
      fn = strrchr(file, '/');
#endif
      if (fn)
        fn++;
      else
        fn = file;
    } else {
      fn = file;
    }
    e1->module = mystrdup(fn);
    e1->lineNo = line;
    e1->procedure = mystrdup(function);

    e2 = e;
    while (e2->traceBack) e2 = e2->traceBack;

    e2->traceBack = e1;
  }
  return e;
}

static long hash(char *s) {
  short x = 0;

  if (!s) return 0;

  while (*s) {
    x <<= 1;
    x ^= *s++ * 257;
  }
  return x;
}

static int fmtErr(HANDLE_ERROR_INFO e, const char *fmt, char *s, int levels) {
  int i;
  int len = 0;

  if (s) *s = 0;
  for (i = 0; i < levels && e; i++, e = e->traceBack) {
    long errcode = (hash(e->module) << 16) + e->lineNo;
    const char *fmt1 = fmt;
    char fmt2[3];

    while (*fmt1) {
      while (*fmt1 && *fmt1 != '%' && *fmt1 != '\\') {
        if (s) *s++ = *fmt1;
        fmt1++;
        len++;
      }

      switch (*fmt1) {
        case '\0':
          continue;

        case '\\':
          fmt2[0] = fmt1[0];
          fmt2[1] = fmt1[1];
          fmt2[2] = 0;
          if (s) {
            sprintf(s, "%s", fmt2);
            s += strlen(s);
          }
          len++;

          fmt1++;
          if (!*fmt1) continue;
          fmt1++;
          break;

        case '%':
          fmt1++;
          switch (*fmt1) {
            case '%':
              if (s) *s++ = '%';
              len++;
              break;
            case '#':
              if (s) {
                sprintf(s, "%08lx", errcode);
                s += strlen(s);
              }
              len += 8;
              break;
            case 'e':
              if (e->errText) {
                if (s) {
                  sprintf(s, "%s", e->errText);
                  s += strlen(s);
                }
                len += (int)strlen(e->errText);
              }
              break;
            case 'f':
              if (e->procedure) {
                if (s) {
                  sprintf(s, "%s", e->procedure);
                  s += strlen(s);
                }
                len += (int)strlen(e->procedure);
              }
              break;
            case 'm':
              if (e->module) {
                if (s) {
                  sprintf(s, "%s", e->module);
                  s += strlen(s);
                }
                len += (int)strlen(e->module);
              }
              break;
            case 'n':
              if (s) {
                sprintf(s, "%ld", e->lineNo);
                s += strlen(s);
              }
              len += 1 + (e->lineNo ? (int)floor(log((float)e->lineNo) / log(10.0)) : 0);
              break;
            case '\0':
              continue;
          }
          fmt1++;
          break;
      }
    }
  }
  if (s) *s = 0;
  len++;
  return len;
}

void errorText(HANDLE_ERROR_INFO e, const char *fmt, char *s, int levels) {
  fmtErr(e, fmt, s, levels);
}

int errorLength(HANDLE_ERROR_INFO e, const char *fmt, int levels) {
  return fmtErr(e, fmt, 0, levels);
}

void freeErrorTraceback(HANDLE_ERROR_INFO e) {
  while (e) {
    HANDLE_ERROR_INFO e1 = e->traceBack;
    if (e->module) iisFree(e->module);
    if (e->procedure) iisFree(e->procedure);
    if (e->errText) iisFree(e->errText);
    iisFree(e);
    e = e1;
  }
}

long errorCode(HANDLE_ERROR_INFO e) {
  long val = (hash(e->module) << 16) + e->lineNo;

  return val;
}

long handleErrorUtillib(HANDLE_ERROR_INFO err) {
  long errCode = 0;

  if (noError != err) {
    char *errMessage = NULL;
    int errLength = 0;
    char errFormat[] = "(ID:%#) %m:%n: %f (%e)\n";
    const int errLevels = 1;

    errLength = errorLength(err, errFormat, errLevels);
    errMessage = (char *)iisCalloc(1, errLength * sizeof(char));
    if (NULL == errMessage) {
      return (errCode = -1);
    }
    errorText(err, errFormat, errMessage, errLevels);

    if (errMessage != NULL) {
      iisFree(errMessage);
      errMessage = NULL;
    }
    errCode = errorCode(err);
    freeErrorTraceback(err);
  }

  return errCode;
}
