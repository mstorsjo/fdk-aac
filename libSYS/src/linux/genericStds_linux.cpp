/**************************  Fraunhofer IIS FDK SysLib  **********************

                        (C) Copyright Fraunhofer IIS (2006)
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
   Author(s):   Manuel Jander
   Description: Linux genericStds (mostly kernel SRAM driver bindings)

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/


/*
 * NOTE: it makes only sense to enable this if you also have the corresponding
 * GNU/Linux kernel driver to access fast SRAM.
 */
#if defined(__arm__) /* || defined(__mips__) */

/**
 * EABI static linking problem workaround
 *
 * These function are normally present in libc.a but
 * apparently can be linked only statically.
 * While using C++ (iisisoff) that is a problem,
 * because it wont work (static global constructors
 * cause problems with static linked programs).
 * So the workaround is to include those functions here,
 * because libSYS.a is linked statically, and libc can be
 * linked dynamically as usual.
 *
 * Add more EABI functions here if you get unresolved
 * symbols of EABI functions.
 */
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void __aeabi_memcpy(void *dest, void *src, int size)
{
  memcpy(dest, src, size);
}
void __aeabi_memcpy4(void *dest, void *src, int size)
{
  memcpy(dest, src, size);
}
void __aeabi_memmove4(void *dest, void *src, int size)
{
  memmove(dest, src, size);
}
void __aeabi_memclr(void *ptr, int size)
{
  memset(ptr, 0, size);
}
void __aeabi_memclr4(void *ptr, int size)
{
  memset(ptr, 0, size);
}
#ifdef __cplusplus
}
#endif

/* Include Linux kernel config, or set ARCH and processor macros directly */
/*
#define CONFIG_ARCH_MXC
#define CONFIG_ARCH_MX25
*/

#if defined(CONFIG_ARCH_OMAP3)
#define KERNEL_SRAM_SIZE       65536
#elif defined(CONFIG_ARCH_MX31)
#define KERNEL_SRAM_SIZE       16384
#elif defined(CONFIG_ARCH_MX25)
#define KERNEL_SRAM_SIZE      131072
#elif defined(CONFIG_ARCH_MX35)
#define KERNEL_SRAM_SIZE      131072
#else
#define KERNEL_SRAM_SIZE 0
#endif

#if (KERNEL_SRAM_SIZE > 0)
#define KERNEL_SCRATCH_SIZE (4096)
#define FDK_SCRATCHBUF_SIZE (KERNEL_SCRATCH_SIZE/sizeof(INT))
#define DATA_L1_A_SIZE (KERNEL_SRAM_SIZE-KERNEL_SCRATCH_SIZE)

#define RESOURCE_scratchBuffer
#define FUNCTION_FDKprolog
#define FUNCTION_FDKepilog

static unsigned char *L1_DATA_A=NULL;
static unsigned char *_a=NULL;


#ifdef RESOURCE_scratchBuffer
static INT *__scratchBuffer;
static unsigned char *__pScratchBuffer = NULL;
#endif


#ifdef __linux__

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static int fd;
static inline void * getSram(void)
{
  unsigned long *ptr = NULL;

  /* Open driver */
  fd = open("/dev/sram", 0);
  if (fd < 0)
  {
    printf("Unable to open /dev/sram. Fallback to malloc\n");
    /* Signal "no sram driver at use". */
    fd = -1;
    /* Return malloced pointer (fallback) */
    return FDKaalloc(KERNEL_SRAM_SIZE, 8);
  }

  /* Get memory mapped into CPU (virtual) address space */
  ptr = (unsigned long *)mmap(NULL, KERNEL_SRAM_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if(ptr == MAP_FAILED)
  {
     printf("Unable to mmap(). Fallback to malloc\n");
     /* Give up on the sram driver */
     close(fd);
     /* Signal "no sram driver at use". */
     fd = -1;
     /* Return malloced pointer (fallback) */
     ptr = (unsigned long *)FDKaalloc(KERNEL_SRAM_SIZE, 8);
  }


  /* Return pointer to sram */
  return (void*)ptr;
}

static inline void freeSram(void* ptr)
{
  /* Check if sram driver is being used. */
  if (fd == -1)
  {
    FDKafree(ptr);
    return;
  }

  /* Unmap memory */
  munmap(ptr, KERNEL_SRAM_SIZE);
  /* Close driver */
  close(fd);

  return;
}

#else

static inline void * getSram(void)
{
  return FDKaalloc(KERNEL_SRAM_SIZE, 8);
}
static inline void * freeSram(void* ptr)
{
  FDKafree(ptr);
}

#endif


#ifdef FUNCTION_FDKprolog
void FDKprolog(void)
{
   unsigned char *addr = (unsigned char*)getSram();


   if (addr == NULL)
   {
     printf("SRAM allocation failed ! This is fatal.\n");
     exit(-1);
   }

#ifdef RESOURCE_scratchBuffer
   __scratchBuffer = (INT*) ( addr + (KERNEL_SRAM_SIZE-KERNEL_SCRATCH_SIZE) );
   __pScratchBuffer = addr + (KERNEL_SRAM_SIZE);
#endif

   printf("SRAM @ 0x%08x\n", (unsigned int) addr);
   atexit(FDKepilog);

   FDKprolog_generic();
}
#endif

#ifdef FUNCTION_FDKepilog
void FDKepilog(void)
{
   /* Because of atexit(), make sure to call this only once */
   if (L1_DATA_A != NULL)
   {
     freeSram(L1_DATA_A);
     L1_DATA_A = NULL;

     FDKepilog_generic();
   }
}
#endif

#endif /* KERNEL_SRAM > 0 */

#endif /* ifdef __arm__ */

