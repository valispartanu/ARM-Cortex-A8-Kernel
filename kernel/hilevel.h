/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __HILEVEL_H
#define __HILEVEL_H

// Include functionality relating to newlib (the standard C library).

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <string.h>

// Include functionality relating to the platform.

#include   "GIC.h"
#include "PL011.h"
#include "SP804.h"

// Include functionality relating to the   kernel.

#include "lolevel.h"
#include     "int.h"

/* The kernel source code is made simpler and more consistent by using
 * some human-readable type definitions:
 *
 * - a type that captures a Process IDentifier (PID), which is really
 *   just an integer,
 * - an enumerated type that captures the status of a process, e.g.,
 *   whether it is currently executing,
 * - a type that captures each component of an execution context (i.e.,
 *   processor state) in a compatible order wrt. the low-level handler
 *   preservation and restoration prologue and epilogue, and
 * - a type that captures a process PCB.
 */

#define MAX_PROCS 32
#define MAX_PIPES 100
#define MAX_PHILS 16
#define MAX_PRIORITY 25

typedef int pid_t;

typedef enum {
  STATUS_INVALID,

  STATUS_CREATED,
  STATUS_TERMINATED,

  STATUS_READY,
  STATUS_EXECUTING,
  STATUS_WAITING
} status_t;

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef struct {
     pid_t      pid; // Process IDentifier (PID)
  status_t   status; // current status
  uint32_t      tos; // address of Top of Stack (ToS)
     ctx_t      ctx; // execution context
       int priority; // priority
       int      age; // age
       int niceness; // niceness
      char*    name; //process name( easier to distinct philosophers from the rest of the programs)
} pcb_t;

typedef struct {
     pid_t          pipeID; // Pipe IDentifier
  status_t          status; // current status
       int            data;
       int          sender;
       int        receiver;
} pipe_t;

extern void initPipes();
extern void console_print(char* str);

#endif
