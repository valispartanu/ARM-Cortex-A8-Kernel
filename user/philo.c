#include "philo.h"

void pick_up_fork(int id){
  //obtaining the id to match the fork pipe id ( from 16 to 31)
  id += MAX_PHILS;
  //wait for the fork
  waiting_read(id);
  //updating the fork status to taken = -1
  waiting_write(id, -1);
}

void release_fork(int id){
  //obtaining the id to match the fork pipe id ( from 16 to 31)
  id += MAX_PHILS;
  //updating the fork status to free == 0
  waiting_write(id, 0);
}
//helper function for signaling "release forks"
void print_fork_release(int id){
    console_print("\n  P");
    print_number(id);
    console_print(" releases forks ");
    print_number((id+1) % MAX_PHILS);
    console_print(" and ");
    print_number(id);
    console_print("!");
}
//helper function for signaling "start thinking"
void print_start_think(int id){
    console_print("\n  P");
    print_number(id);
    console_print(" starts to think!");
}
//helper function for signaling "done thinking"
void print_done_think(int id){
    console_print("\n  P");
    print_number(id);
    console_print(" is done thinking!");
}
//helper function for signaling "start eating"
void print_start_eat(int id){
    console_print("\n  P");
    print_number(id);
    console_print(" starts to eat!");
}
//helper function for signaling "done eating"
void print_done_eat(int id){
    console_print("\n  P");
    print_number(id);
    console_print(" is done eating!");
}

int min(int x, int y){
  return (x < y ? x : y);
}

int max(int x, int y){
  return (x > y ? x : y);
}

void think(int id){
  print_start_think(id);
  //trigger timer to produce a delay for thinking
  reset_timer(id);
  //wait for timer to finish (send 0) and continue
  waiting_read(id);
  print_done_think(id);
}

void eat(int id){
  print_start_eat(id);
  //trigger timer to produce a delay for eating
  reset_timer(id);
  //wait for timer to finish (send 0) and continue
  waiting_read(id);
  print_done_eat(id);
}

void main_philo() {
  //update philosopher id by calling the update_philo_id() sys call
  int id = update_philo_id();
  think(id);
  //wait and pick up left fork
  pick_up_fork(min(id, (id+1) % MAX_PHILS));
  //wait and pick up right fork
  pick_up_fork(max(id, (id+1) % MAX_PHILS));
  eat(id);
  //release right fork
  release_fork((id+1) % MAX_PHILS);
  //release left fork
  release_fork(id);
  print_fork_release(id);
  exit( EXIT_SUCCESS );
}
