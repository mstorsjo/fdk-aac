
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

#ifndef IIS_DISABLE_ALL_WARNINGS_H
#define IIS_DISABLE_ALL_WARNINGS_H

#if defined(_MSC_VER)
#define IIS_DISABLE_ALL_WARNINGS_BEGIN \
  __pragma(warning(push, 0))
#define IIS_DISABLE_ALL_WARNINGS_END \
  __pragma(warning(pop))
#elif (defined(__GNUC__) || defined(__GNUG__)) && !defined(__clang__)
#define IIS_DISABLE_ALL_WARNINGS_BEGIN                                                                                                                                                                                                                                                                                                                                                                     \
  _Pragma("GCC diagnostic push")                                                                                                                                                                                                                                                                                                                                                                           \
      _Pragma("GCC diagnostic ignored \"-Wpragmas\"")                                                                                                                                                                                                                                                                                                                                                      \
          _Pragma("GCC diagnostic ignored \"-Wpedantic\"")                                                                                                                                                                                                                                                                                                                                                 \
              _Pragma("GCC diagnostic ignored \"-Wextra\"")                                                                                                                                                                                                                                                                                                                                                \
                  _Pragma("GCC diagnostic ignored \"-Waddress\"")                                                                                                                                                                                                                                                                                                                                          \
                      _Pragma("GCC diagnostic ignored \"-Warray-bounds=1\"")                                                                                                                                                                                                                                                                                                                               \
                          _Pragma("GCC diagnostic ignored \"-Warray-compare\"")                                                                                                                                                                                                                                                                                                                            \
                              _Pragma("GCC diagnostic ignored \"-Warray-parameter=2\"")                                                                                                                                                                                                                                                                                                                    \
                                  _Pragma("GCC diagnostic ignored \"-Wbool-compare\"")                                                                                                                                                                                                                                                                                                                     \
                                      _Pragma("GCC diagnostic ignored \"-Wbool-operation\"")                                                                                                                                                                                                                                                                                                               \
                                          _Pragma("GCC diagnostic ignored \"-Wc++11-compat\"")                                                                                                                                                                                                                                                                                                             \
                                              _Pragma("GCC diagnostic ignored \"-Wcatch-value\"")                                                                                                                                                                                                                                                                                                          \
                                                  _Pragma("GCC diagnostic ignored \"-Wchar-subscripts\"")                                                                                                                                                                                                                                                                                                  \
                                                      _Pragma("GCC diagnostic ignored \"-Wcomment\"")                                                                                                                                                                                                                                                                                                      \
                                                          _Pragma("GCC diagnostic ignored \"-Wdangling-pointer=2\"")                                                                                                                                                                                                                                                                                       \
                                                              _Pragma("GCC diagnostic ignored \"-Wduplicate-decl-specifier\"")                                                                                                                                                                                                                                                                             \
                                                                  _Pragma("GCC diagnostic ignored \"-Wenum-compare\"")                                                                                                                                                                                                                                                                                     \
                                                                      _Pragma("GCC diagnostic ignored \"-Wformat\"")                                                                                                                                                                                                                                                                                       \
                                                                          _Pragma("GCC diagnostic ignored \"-Wformat-overflow\"")                                                                                                                                                                                                                                                                          \
                                                                              _Pragma("GCC diagnostic ignored \"-Wformat-truncation\"")                                                                                                                                                                                                                                                                    \
                                                                                  _Pragma("GCC diagnostic ignored \"-Wint-in-bool-context\"")                                                                                                                                                                                                                                                              \
                                                                                      _Pragma("GCC diagnostic ignored \"-Wimplicit\"")                                                                                                                                                                                                                                                                     \
                                                                                          _Pragma("GCC diagnostic ignored \"-Wimplicit-int\"")                                                                                                                                                                                                                                                             \
                                                                                              _Pragma("GCC diagnostic ignored \"-Wimplicit-function-declaration\"")                                                                                                                                                                                                                                        \
                                                                                                  _Pragma("GCC diagnostic ignored \"-Winit-self\"")                                                                                                                                                                                                                                                        \
                                                                                                      _Pragma("GCC diagnostic ignored \"-Wlogical-not-parentheses\"")                                                                                                                                                                                                                                      \
                                                                                                          _Pragma("GCC diagnostic ignored \"-Wmain\"")                                                                                                                                                                                                                                                     \
                                                                                                              _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")                                                                                                                                                                                                                                  \
                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wmemset-elt-size\"")                                                                                                                                                                                                                                  \
                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wmemset-transposed-args\"")                                                                                                                                                                                                                       \
                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wmisleading-indentation\"")                                                                                                                                                                                                                   \
                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wmismatched-dealloc\"")                                                                                                                                                                                                                   \
                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wmismatched-new-delete\"")                                                                                                                                                                                                            \
                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wmissing-attributes\"")                                                                                                                                                                                                           \
                                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wmissing-braces\"")                                                                                                                                                                                                           \
                                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wmultistatement-macros\"")                                                                                                                                                                                                \
                                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wnarrowing\"")                                                                                                                                                                                                        \
                                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wnonnull\"")                                                                                                                                                                                                      \
                                                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wnonnull-compare\"")                                                                                                                                                                                          \
                                                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wopenmp-simd\"")                                                                                                                                                                                          \
                                                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wparentheses\"")                                                                                                                                                                                      \
                                                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wpessimizing-move\"")                                                                                                                                                                             \
                                                                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wpointer-sign\"")                                                                                                                                                                             \
                                                                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wrange-loop-construct\"")                                                                                                                                                                 \
                                                                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wreorder\"")                                                                                                                                                                          \
                                                                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wrestrict\"")                                                                                                                                                                     \
                                                                                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wreturn-type\"")                                                                                                                                                              \
                                                                                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wsequence-point\"")                                                                                                                                                       \
                                                                                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wsign-compare\"")                                                                                                                                                     \
                                                                                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wsizeof-array-div\"")                                                                                                                                             \
                                                                                                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wsizeof-pointer-div\"")                                                                                                                                       \
                                                                                                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wsizeof-pointer-memaccess\"")                                                                                                                             \
                                                                                                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wstrict-aliasing\"")                                                                                                                                  \
                                                                                                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wstrict-overflow=1\"")                                                                                                                            \
                                                                                                                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wswitch\"")                                                                                                                                   \
                                                                                                                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wtautological-compare\"")                                                                                                                 \
                                                                                                                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wtrigraphs\"")                                                                                                                        \
                                                                                                                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wuninitialized\"")                                                                                                                \
                                                                                                                                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wunknown-pragmas\"")                                                                                                          \
                                                                                                                                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wunused-function\"")                                                                                                      \
                                                                                                                                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wunused-label\"")                                                                                                     \
                                                                                                                                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wunused-value\"")                                                                                                 \
                                                                                                                                                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wunused-variable\"")                                                                                          \
                                                                                                                                                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wuse-after-free=3\"")                                                                                     \
                                                                                                                                                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wvla-parameter\"")                                                                                    \
                                                                                                                                                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wvolatile-register-var\"")                                                                        \
                                                                                                                                                                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wzero-length-bounds\"")                                                                       \
                                                                                                                                                                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wclobbered\"")                                                                            \
                                                                                                                                                                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wcast-function-type\"")                                                               \
                                                                                                                                                                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wdeprecated-copy\"")                                                              \
                                                                                                                                                                                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wempty-body\"")                                                               \
                                                                                                                                                                                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wenum-conversion\"")                                                      \
                                                                                                                                                                                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wignored-qualifiers\"")                                               \
                                                                                                                                                                                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wimplicit-fallthrough=3\"")                                       \
                                                                                                                                                                                                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"")                               \
                                                                                                                                                                                                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wmissing-parameter-type\"")                               \
                                                                                                                                                                                                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wold-style-declaration\"")                            \
                                                                                                                                                                                                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Woverride-init\"")                                \
                                                                                                                                                                                                                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wsign-compare\"")                             \
                                                                                                                                                                                                                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wstring-compare\"")                       \
                                                                                                                                                                                                                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wredundant-move\"")                   \
                                                                                                                                                                                                                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wtype-limits\"")                  \
                                                                                                                                                                                                                                                                                                                                          _Pragma("GCC diagnostic ignored \"-Wuninitialized\"")            \
                                                                                                                                                                                                                                                                                                                                              _Pragma("GCC diagnostic ignored \"-Wshift-negative-value\"") \
                                                                                                                                                                                                                                                                                                                                                  _Pragma("GCC diagnostic ignored \"-Wunused-parameter\"") \
                                                                                                                                                                                                                                                                                                                                                      _Pragma("GCC diagnostic ignored \"-Wunused-but-set-parameter\"")
#define IIS_DISABLE_ALL_WARNINGS_END \
  _Pragma("GCC diagnostic pop")
#elif defined(__clang__)
#define IIS_DISABLE_ALL_WARNINGS_BEGIN                       \
  _Pragma("clang diagnostic push")                           \
      _Pragma("clang diagnostic ignored \"-Wpragmas\"")      \
          _Pragma("clang diagnostic ignored \"-Wpedantic\"") \
              _Pragma("clang diagnostic ignored \"-Wall\"")
_Pragma("clang diagnostic ignored \"-Wextra\"")
#define IIS_DISABLE_ALL_WARNINGS_END \
  _Pragma("clang diagnostic pop")
#else
#define IIS_DISABLE_ALL_WARNINGS_BEGIN
#define IIS_DISABLE_ALL_WARNINGS_END
#endif

#endif
