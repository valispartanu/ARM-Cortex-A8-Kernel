/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"
#include "libc.h"

#include <stdlib.h>

uint32_t stack_difference = 0x00001000;
//utils for determining the last pcb and pipe used
int last_pcb_used = 0;
int last_pipe_used = -1;
//table for processes
pcb_t procTab[ MAX_PROCS ];
//table for pipes
pipe_t pipeTab[ MAX_PIPES ];
//array to keep timer value for each philosopher
int timer[ MAX_PROCS ];
//flag for pipe initialization
int flag = 0;
pcb_t* executing = NULL;

extern void     main_console();
extern uint32_t tos_console;
extern void     main_P3();
extern void     main_P4();
extern void     main_P5();
extern void     main_waiter();
extern void     main_philo();
extern uint32_t tos_all_processes;

//helper function for printing
void console_print( char* str ) {
    for ( int i = 0; i < strlen( str ); i++ ) {
        PL011_putc( UART0, str[ i ], true );
    }
}

//initializing the pipes
void initPipes(){
  //The first 16 pipes are created to send timer data (for eat and think)
  for(int i = 0; i < MAX_PHILS; i++){
    pipe(-10,i);
  }
  //16 new pipes are created to send the fork status
  for(int i = 0; i < MAX_PHILS; i++){
    pipe(-10, i);
    //the fork status is set to free == 0, for all forks at the beginning
    write_pipe(MAX_PHILS + i, 0);
  }
  // pipes are initialised, set flag
  flag = 1;
}

//helper function to identify user programs by name
char* take_name(uint32_t name) {
  if(name == (uint32_t) main_P3) return "P3";
  else if(name == (uint32_t) main_P4) return "P4";
  else if(name == (uint32_t) main_P5) return "P5";
  else if(name == (uint32_t) main_waiter) return "WAITER";
  else if(name == (uint32_t) main_philo) return "PHILO";
  return "";
}

void dispatch( ctx_t* ctx, pcb_t* prev, pcb_t* next ) {
  char prev_pid = '?', next_pid = '?';

  if( NULL != prev ) {
    memcpy( &prev->ctx, ctx, sizeof( ctx_t ) ); // preserve execution context of P_{prev}
    prev_pid = '0' + prev->pid;
  }
  if( NULL != next ) {
    memcpy( ctx, &next->ctx, sizeof( ctx_t ) ); // restore  execution context of P_{next}
    next_pid = '0' + next->pid;
  }

    PL011_putc( UART0, '[',      true );
    print_number(prev->pid);
    PL011_putc( UART0, '-',      true );
    PL011_putc( UART0, '>',      true );
    print_number(next->pid);
    PL011_putc( UART0, ']',      true );

    executing = next;                          // update   executing process to P_{next}

  return;
}

void schedule( ctx_t* ctx ) {
    //scheduling the philosophers
    /*if( procTab[executing->pid].name == "PHILO" || procTab[executing->pid].name == "WAITER" ){
    int max_pr = 0;
    int new_pr;
    int next = executing->pid;
    for( int i = 0; i <= last_pcb_used; i++ ) {
        new_pr = procTab[ i ].priority;
        /*updating the priority to the maximum available and save pid of the process
        * with this priority( to know which process to execute next)
        if((new_pr > max_pr) && (procTab[ i ].status == STATUS_READY)) {
            max_pr = new_pr;
            next = i;
        }
    }
    //aging
    for( int j = 0; j <= last_pcb_used; j++ ) {
        if(procTab[ j ].priority < MAX_PRIORITY && procTab[ j ].status == STATUS_READY){
            procTab[ j ].priority++;
        }
      }
      //call dispatch to produce a context switch
      dispatch( ctx, executing, &procTab[ next ] );
      executing->status = STATUS_READY;
      procTab[ next ].status = STATUS_EXECUTING;
  }*/
   //for the rest of the user processes
    //else{
       int max_pr = 0;
       int new_pr;
       int next = executing->pid;
       for( int i = 0; i <= last_pcb_used; i++ ) {
           //add the age to the priority of each process
           new_pr = procTab[ i ].priority + procTab[ i ].age;
           /*updating the priority to the maximum available and save pid of the process
           * with this priority( to know which process to execute next)*/
           if((new_pr > max_pr) && ((procTab[ i ].status == STATUS_EXECUTING) ||
               (procTab[ i ].status == STATUS_READY))) {
               max_pr = new_pr;
               next = i;
           }
       }
       /*aging algorithm( increase the age for all processes except the one
       * currently executing). In this way, we prevent a process to monopolize
       the processor and encourage the rest of the processes to execute next */
       procTab[ next ].age = 0;
       for( int j = 0; j <= last_pcb_used; j++ ) {
           if(j!=next && procTab[ j ].status != STATUS_TERMINATED){
               procTab[ j ].age +=1;
           }
        }
        //call dispatch to produce a context switch
        dispatch( ctx, executing, &procTab[ next ] );
        executing->status = STATUS_READY;
        procTab[ next ].status = STATUS_EXECUTING;
    //}

    return;
}

void hilevel_handler_rst( ctx_t* ctx ) {

  TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

  GICC0->PMR          = 0x000000F0; // unmask all            interrupts
  GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
  GICC0->CTLR         = 0x00000001; // enable GIC interface
  GICD0->CTLR         = 0x00000001; // enable GIC distributor

  //enabling the IRQ interrupt
  int_enable_irq();
  /* Invalidate all entries in the process table, so it's clear they are not
   * representing valid (i.e., active) processes.
   */
  for( int i = 0; i < MAX_PROCS; i++ ) {
    procTab[ i ].status = STATUS_INVALID;
  }

  /* Invalidate all entries in the pipes table, so it's clear they are not
   * representing valid (i.e., active) pipes.
   */
  for( int j = 0; j < MAX_PIPES; j++ ){
    pipeTab[ j ].status = STATUS_INVALID;
  }

  /* - the CPSR value of 0x50 means the processor is switched into USR mode,
   *   with IRQ interrupts enabled, and
   * - the PC and SP values match the entry point and top of stack.
   */
  //populate the 0-th PCB for the console; On RESET, start in console
  memset( &procTab[ 0 ], 0, sizeof( pcb_t ) ); // initialise 0-th PCB = console
  procTab[ 0 ].pid      = 0;
  procTab[ 0 ].status   = STATUS_READY;
  procTab[ 0 ].tos      = ( uint32_t )( &tos_console  );
  procTab[ 0 ].ctx.cpsr = 0x50;
  procTab[ 0 ].ctx.pc   = ( uint32_t )( &main_console );
  procTab[ 0 ].ctx.sp   = procTab[ 0 ].tos;
  procTab[ 0 ].priority = 1;
  procTab[ 0 ].age      = 0;
  procTab[ 0 ].niceness = 0;
  procTab[ 0 ].name = "CNSL";

  /* Once the PCBs are initialised, we arbitrarily select the 0-th PCB to be
   * executed: there is no need to preserve the execution context, since it
   * is invalid on reset (i.e., no process was previously executing).
   */

  dispatch( ctx, NULL, &procTab[ 0 ] );

  return;
}

void hilevel_handler_irq(ctx_t* ctx) {
  // Step 2: read  the interrupt identifier so we know the source.

  uint32_t id = GICC0->IAR;

  // Step 4: handle the interrupt, then clear (or reset) the source.

  if( id == GIC_SOURCE_TIMER0 ) {
    //check if the pipes were initialized and decrease the timer for each philosopher
    if(flag){
      for(int i = 0; i < MAX_PHILS; i++){
         timer[i]--;
         if(timer[i] == 0){
           //send 0 to trigger ending of eat/think
           waiting_write(i,0);
         }
       }
     }

      schedule(ctx);
      TIMER0->Timer1IntClr = 0x01;
  }

  // Step 5: write the interrupt identifier to signal we're done.

  GICC0->EOIR = id;

  return;
}

void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {
  /* Based on the identifier (i.e., the immediate operand) extracted from the
   * svc instruction,
   *
   * - read  the arguments from preserved usr mode registers,
   * - perform whatever is appropriate for this system call, then
   * - write any return value back to preserved usr mode registers.
   */

  switch( id ) {
    //yield sys call, used in stage 1a by user programs to give control cooperatively
    case 0x00 : { // 0x00 => yield()

      schedule( ctx );

      break;
    }

    //write sys call, used for writing on the console
    case 0x01 : { // 0x01 => write( fd, x, n )

      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      for( int i = 0; i < n; i++ ) {
          PL011_putc( UART0, *x++, true );
      }
      ctx->gpr[ 0 ] = n;

      break;
    }

    //read sys call
    case 0x02 : { // 0x02 => read( fd, x, n )

      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      break;
    }

    //fork sys call, used for creating new child processes with unique pid
    case 0x03 : { // 0x03 => fork()

      //step 1: create child process with unique pid

      last_pcb_used++;
      //initialise pcb for child process
      memset(&procTab[ last_pcb_used ], 0 , sizeof(pcb_t));
      //allocate stack for child process
      procTab[ last_pcb_used ].tos = (uint32_t)(&tos_all_processes) - (stack_difference * (last_pcb_used-1));

      //populate pcb for child process
      procTab[ last_pcb_used ].pid = last_pcb_used;
      procTab[ last_pcb_used ].status = STATUS_READY;
      procTab[ last_pcb_used ].priority = 1;
      procTab[ last_pcb_used ].age = 0;
      procTab[ last_pcb_used ].niceness = 0;
      procTab[ last_pcb_used ].name = procTab[executing->pid].name;

      // step 2: replicate state of parent in child
      memcpy(&procTab[ last_pcb_used ].ctx, ctx, sizeof(ctx_t));
      uint32_t stack_size = procTab[ executing->pid ].tos - (uint32_t) ctx->sp;
      procTab[ last_pcb_used ].ctx.sp = procTab[ last_pcb_used ].tos - stack_size;
      memcpy((uint32_t*) procTab[ last_pcb_used ].ctx.sp, (uint32_t*) ctx->sp, stack_size);

      // step 3: return 0 for child, child PID for parent
      executing->ctx.gpr[ 0 ] = procTab[ last_pcb_used ].pid;
      procTab[ last_pcb_used ].ctx.gpr[ 0 ] = 0;

      break;
    }

    //exit sys call, used to perform normal termination
    case 0x04 :{ // exit( x )

      procTab[ executing->pid ].status = STATUS_TERMINATED;
      procTab[ executing->pid ].priority = -1;
      /*after a program exits, call schedule to determine the new priority of
      *the process to be executed next*/
      schedule( ctx );

      break;
    }

    //exec sys call
    case 0x05 :{ // exec( x )

      //replace current process image with new process image
      ctx->pc = (uint32_t)(ctx->gpr[ 0 ]);
      procTab[ last_pcb_used ].name = take_name(ctx -> gpr[0]);
      //reset stack pointer, continue to execute at the entry point of new program
      ctx->sp = procTab[ executing->pid ].tos;
      break;
    }

    //kill sys call, used by the 'terminate' command
    case 0x06 :{ // kill( pid, x )

      //assigning the first parameter of the kill( pid, x) function
      int pid = ctx->gpr[ 0 ];
      /*if we would kill the console, we would have no control over the
      *remaining executing processes. For the 'terminate 0' command, kill
      *all the processes except the console*/
      if(pid == 0){
          for(int j=1; j<= last_pcb_used+1; j++)
               procTab[ j ].status = STATUS_TERMINATED;
          ctx->gpr[ 0 ] = 0;
      }
      //else kill process with given pid
      else{
          for(int i=0; i<= last_pcb_used; i++){
              if((pid == procTab[ i ].pid) && (procTab[ i ].status != STATUS_TERMINATED)){
                  procTab[ i ].status = STATUS_TERMINATED;
                  procTab[ i ].priority =-1;
                  ctx->gpr[ 0 ] = 0;
              }
          }
      }

      break;
    }

    //pipe sys call, used for creating a new pipe(channel) between 2 processes
    case 0x08 :{ // pipe( send, rec )

      //increase the pipe index in the table
      last_pipe_used++;

      //initialise pipeTab for new pipe
      pipeTab[ last_pipe_used ].pipeID = last_pipe_used;
      pipeTab[ last_pipe_used ].status = STATUS_READY;
      //assigning the first parameter of the pipe(send, rec) function
      pipeTab[ last_pipe_used ].sender = (uint32_t) ctx->gpr[0];
      //assigning the second parameter of the pipe(send, rec) function
      pipeTab[ last_pipe_used ].receiver = (uint32_t) ctx->gpr[1];
      //data is set to -1 initially
      pipeTab[ last_pipe_used ].data = -1;

      break;
    }

    //write_pipe sys call, used to write data to the pipe
    case 0x09 :{ // write_pipe( id, data )

      //assigning the first parameter of the write_pipe( id, data) function
      int id = ctx->gpr[0];
      //assigning the second parameter of the write_pipe( id, data) function
      int data = ctx->gpr[1];

      //updating the data field of the pipe data structure
      pipeTab[id].data = data;
      break;
    }

    //read_pipe sys call, used for reading data from the pipe
    case 0x10 :{ // read_pipe( id )

      //assigning the parameter of the read_pipe( id ) function
      int id = ctx->gpr[0];
      //returning the data
      ctx->gpr[0] = pipeTab[id].data;
      break;
    }

    //close_pipe sys call, used to close a pipe, stop using it
    case 0x11 :{ // close_pipe( id )

      //assigning the parameter of the close_pipe( id ) function
      int id = ctx->gpr[ 0 ];
      //set the pipe with that id to unavailable
      pipeTab[ id ].status = STATUS_TERMINATED;
      ctx->gpr[ 0 ] = 0;

      break;
    }

    //update_philo_id sys call
    case 0x21 :{ // update_philo_id()

      int id = 0;
      for(int i = 0; i <= last_pcb_used; i++){
        if(i == executing->pid){
          //return id
          ctx->gpr[0] = id;
          break;
        }
        if(0 == strcmp( procTab[i].name, "PHILO" )){
        id++;
        }
      }
      break;
    }

    //update_philo_nb sys call
    case 0x22 :{ // update_philo_nb()

      int nb = 0;
      for(int i = 0; i <= last_pcb_used; i++){
        if(0 == strcmp( procTab[i].name, "PHILO" ) && procTab[i].status != STATUS_TERMINATED){
        nb++;
        }
      }
      //return number
      ctx->gpr[0] = nb;
      break;
    }

    //reset_timer sys call
    case 0x23 :{ // reset_timer( x )

      //assigning the parameter of the reset_timer( x ) function
      int id = ctx->gpr[0];
      //obtain a random value between 1 and 5
      int x = rand() % 5 + 1;
      /*update the timer for the philosopher with this id, the time for which a
      *philosopher eats or thinks*/
      timer[id] = x*x;
      break;
  }

    default   : { // 0x?? => unknown/unsupported

      break;
    }
  }

  return;
}
