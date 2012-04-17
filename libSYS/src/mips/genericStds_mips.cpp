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
   Description:

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#define RESOURCE_scratchBuffer
#define FUNCTION_FDKprolog
#define FUNCTION_FDKepilog

#define MIPS_VIRTUAL_START  (0x80000000)
/* value below is defined in simulator config (MipsMemIntf-{24KE,4KE}.cfg) */
#define MIPS_SDE_SCRATCHPAD (0x00058000)

//#define MIPS_SRAM_SIZE  (32768)
#define MIPS_SRAM_SIZE  (4096)

#define MIPS_SCRATCH_SIZE (4096)
#define DATA_L1_A_SIZE (MIPS_SRAM_SIZE-MIPS_SCRATCH_SIZE)




#ifdef RESOURCE_scratchBuffer
#define FDK_SCRATCHBUF_SIZE 1024
static LONG *___scratchBuffer = NULL;
static LONG *__scratchBuffer = NULL;
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
    printf("Unable to access sram. Fallback to malloc\n");
    /* Signal "no sram driver at use". */
    fd = -1;
    /* Return malloced pointer (fallback) */
    return malloc(MIPS_SRAM_SIZE);
  }

  /* Get memory mapped into CPU (virtual) address space */
  ptr = (unsigned long *)mmap(NULL, MIPS_SRAM_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if(ptr == MAP_FAILED)
  {
     printf("Unable to access sram. Fallback to malloc\n");
     /* Give up on the sram driver */
     close(fd);
     /* Signal "no sram driver at use". */
     fd = -1;
     /* Return malloced pointer (fallback) */
     ptr = (unsigned long *)malloc(MIPS_SRAM_SIZE);
  }


  /* Return pointer to sram */
  return (void*)ptr;
}

static inline void freeSram(void* ptr)
{
  /* Check if sram driver is being used. */
  if (fd == -1)
  {
    free(ptr);
    return;
  }

  /* Unmap memory */
  munmap(ptr, MIPS_SRAM_SIZE);
  /* Close driver */
  close(fd);

  return;
}

#elif defined(__SDE_MIPS__)

#include <stdio.h>
#include <mips/cpu.h>

static int hasISPRAM, hasDSPRAM;

static inline void * getSram(void)
{
  void *addr;
  unsigned int Config;

  Config = mips_getconfig();
  hasISPRAM = (Config >> 24) & 1;
  hasDSPRAM = (Config >> 23) & 1;

  FDKprintf("Config ISP/DSP: %d/%d\n", hasISPRAM, hasDSPRAM);

  if (hasDSPRAM) {
    long paddr, laddr;

    FDKprintf("wrong\n");
    paddr = MIPS_SDE_SCRATCHPAD;
    /* Fixed mapping of kseg0: 0x80000000-0x9fffffff virtual => 0x00000000-0x1fffffff physical */
    laddr = MIPS_VIRTUAL_START + MIPS_SDE_SCRATCHPAD;
    addr = (void*)(laddr);
  } else {
    FDKprintf("ok\n");
    addr = malloc(MIPS_SRAM_SIZE);
    FDKprintf("addr %d\n", (int)addr);
  }
  return addr;
}
static inline void freeSram(void* ptr)
{
  if (!hasDSPRAM) {
    free(ptr);
  }
}

#else

static inline void * getSram(void)
{
  return malloc(MIPS_SRAM_SIZE);
}
static inline void freeSram(void* ptr)
{
  free(ptr);
}

#endif


#ifdef FUNCTION_FDKprolog
void FDKprolog(void)
{
   unsigned char *addr;

#ifdef _MIPS_ARCH_MIPS32R2
   unsigned status;
   asm volatile("mfc0 %0, $12, 0;\n" : "=r" (status));
   status |= (1 << 24);
   asm volatile("mtc0 %0, $12, 0;\n" :: "r" (status));
#endif

   addr = (unsigned char*)getSram();
   if (addr == NULL) {
     FDKprintfErr("SRAM allocation failed ! This is fatal.\n");
     exit(-1);
   } else {
     FDKprintf("SRAM @ 0x%08x, size = 0x%x\n", (unsigned int) addr, MIPS_SRAM_SIZE);
   }


#ifdef RESOURCE_scratchBuffer
   ___scratchBuffer = (LONG*)(addr + MIPS_SRAM_SIZE - MIPS_SCRATCH_SIZE);
#endif

   atexit(FDKepilog);

   FDKprolog_generic();
}
#endif

#ifdef FUNCTION_FDKepilog
void FDKepilog(void)
{

#ifdef _MIPS_ARCH_MIPS32R2
   unsigned status;
   asm volatile("mfc0 %0, $12, 0;\n" : "=r" (status));
           status &= ~(1 << 24);
   asm volatile("mtc0 %0, $12, 0;\n" :: "r" (status));
#endif

   FDKepilog_generic();
}
#endif


#if !defined(__linux__)

#define FUNCTION_FDKclock

#ifndef MIPS_CPU_CLK
#define MIPS_CPU_CLK 100000000
#endif

INT FDKclock(void) {
  INT clk;

  asm volatile ("mfc0 %0,$9 " : "=r" (clk));
  return clk;
}

#endif /* !defined(__linux__) */
