/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "libc.h"

void print_number(int x){
  if(x<0){
    write(STDOUT_FILENO, "--", 2);
  }
  else if(x>9){
    PL011_putc( UART0, '0' + (x/10), true );
    PL011_putc( UART0, '0' + (x - ((x/10)*10)), true );
  }
  else{
    write(STDOUT_FILENO, " ", 1);
    PL011_putc( UART0, '0' + x, true );
  }
}

int waiting_read( int id ){
  int x = read_pipe( id );
  //wait until receive 0 on the pipe
  while(x == -1){
    x = read_pipe( id );
  }
  //update pipe
  write_pipe( id, -1 );
  return x;
}

void waiting_write( int id, int data ){
  int x = read_pipe( id );
  //wait until receive -1
  while(x != -1){
    x = read_pipe( id );
  }
  //update pipe
  write_pipe( id, data );
}

int  atoic( char* x ) {
  char* p = x; bool s = false; int r = 0;

  if     ( *p == '-' ) {
    s =  true; p++;
  }
  else if( *p == '+' ) {
    s = false; p++;
  }

  for( int i = 0; *p != '\x00'; i++, p++ ) {
    r = s ? ( r * 10 ) - ( *p - '0' ) :
            ( r * 10 ) + ( *p - '0' ) ;
  }

  return r;
}

void itoac( char* r, int x ) {
  char* p = r; int t, n;

  if( x < 0 ) {
     p++; t = -x; n = t;
  }
  else {
          t = +x; n = t;
  }

  do {
     p++;                    n /= 10;
  } while( n );

    *p-- = '\x00';

  do {
    *p-- = '0' + ( t % 10 ); t /= 10;
  } while( t );

  if( x < 0 ) {
    *p-- = '-';
  }

  return;
}

void yield() {
  asm volatile( "svc %0     \n" // make system call SYS_YIELD
              :
              : "I" (SYS_YIELD)
              : );

  return;
}

int write( int fd, const void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_WRITE
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_WRITE), "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int  read( int fd,       void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_READ
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_READ),  "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int  fork() {
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_FORK
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_FORK)
              : "r0" );

  return r;
}

void exit( int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  x
                "svc %0     \n" // make system call SYS_EXIT
              :
              : "I" (SYS_EXIT), "r" (x)
              : "r0" );

  return;
}

void exec( const void* x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 = x
                "svc %0     \n" // make system call SYS_EXEC
              :
              : "I" (SYS_EXEC), "r" (x)
              : "r0" );

  return;
}

int  kill( int pid, int x ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 =  pid
                "mov r1, %3 \n" // assign r1 =    x
                "svc %1     \n" // make system call SYS_KILL
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r)
              : "I" (SYS_KILL), "r" (pid), "r" (x)
              : "r0", "r1" );

  return r;
}

void nice( int pid, int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  pid
                "mov r1, %2 \n" // assign r1 =    x
                "svc %0     \n" // make system call SYS_NICE
              :
              : "I" (SYS_NICE), "r" (pid), "r" (x)
              : "r0", "r1" );

  return;
}

void pipe( int send, int rec ) {
    asm volatile( "mov r0, %1 \n" // assign r0 = send
                  "mov r1, %2 \n" // assign r1 = rec
                  "svc %0     \n" // make system call SYS_PIPE
                :
                : "I" (SYS_PIPE), "r" (send), "r" (rec)
                : "r0", "r1" );

    return;
}

void write_pipe( int id, int data ){
  asm volatile( "mov r0, %1 \n" // assign r0 = id
                "mov r1, %2 \n" // assign r1 = data
                "svc %0     \n" // make system call SYS_WRITE_PIPE
              :
              : "I" (SYS_WRITE_PIPE),  "r" (id), "r" (data)
              : "r0", "r1" );

  return;
}

int read_pipe( int id ){
  int r;
  asm volatile( "mov r0, %2 \n" // assign r0 =  id
                "svc %1     \n" // make system call SYS_READ_PIPE
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_READ_PIPE), "r" (id)
              : "r0" );

  return r;
}

void close_pipe( int id ) {

  asm volatile( "mov r0, %1 \n" // assign r0 = id
                "svc %0     \n" // make system call SYS_CLOSE
              :
              : "I" (SYS_CLOSE), "r" (id)
              : "r0" );

  return;
}

int update_philo_id() {
    int r;

    asm volatile( "svc %1     \n" // make svc call SYS_PHILO_ID
                  "mov %0, r0 \n" // assign r = r0
                : "=r" (r)
                : "I" (SYS_PHILO_ID)
                : "r0" );

    return r;
}

int update_philo_nb() {
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_PHILO_NO
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_PHILO_NO)
              : "r0");
  return r;
}

void reset_timer( int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  x
                "svc %0     \n" // make system call SYS_PHILO_TM
              :
              : "I" (SYS_PHILO_TM), "r" (x)
              : "r0" );

  return;
}
