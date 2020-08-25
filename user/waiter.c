#include "waiter.h"

extern void main_philo();

void main_waiter() {
  initPipes();
  while(update_philo_nb() < MAX_PHILS){
    //keep creating philosopher processes( until reach 16)
    fork();
    exec(&main_philo);
  }

  //kill all the processes at the end
  int id;
  id += update_philo_nb();
  for(int i=1; i<= id; i++){
     kill(i, 0);
  }
}
