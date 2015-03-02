#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>

/* 
 * This simple default synchronization mechanism allows only creature at a time to
 * eat.   The globalCatMouseSem is used as a a lock.   We use a semaphore
 * rather than a lock so that this code will work even before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */
//static struct semaphore *globalCatMouseSem;
static struct cv *catMouseCV;
static struct cv *mouseEatingCV;
static struct cv *catEatingCV;
static volatile int numOfCatIsEating;
static volatile int numOfMouseIsEating;
static volatile int * bowlsary;
static struct lock * mutex;

/* 
 * The CatMouse simulation will call this function once before any cat or
 * mouse tries to each.
 *
 * You can use it to initialize synchronization and other variables.
 * 
 * parameters: the number of bowls
 */
void
catmouse_sync_init(int bowls)
{
  /* replace this default implementation with your own implementation of catmouse_sync_init */

  // (void)bowls; /* keep the compiler from complaining about unused parameters */
  // globalCatMouseSem = sem_create("globalCatMouseSem",1);
  // if (globalCatMouseSem == NULL) {
  //   panic("could not create global CatMouse synchronization semaphore");
  // }
  // return;
  numOfMouseIsEating = 0;
  numOfCatIsEating = 0;
  bowlsary = kmalloc(bowls*sizeof(int));
  for(int i=0;i<bowls;i++) {
    bowlsary[i] = 0;
  }
  // for(int i = 0;i<bowls;i++){
  //   kprintf("test bowlsary: %d\n",bowlsary[i]);
  // }

  catMouseCV = cv_create("catMouseCV");
  mouseEatingCV = cv_create("mouseEatingCV");
  catEatingCV = cv_create("catEatingCV");
  if(catMouseCV == NULL){
    panic("could not create CatMouse synchronization CV");
  }
  if(mouseEatingCV == NULL){
    panic("could not create mouse synchronization CV");
  }
  if(catEatingCV == NULL){
    panic("could not create mouse synchronization CV");
  }
  mutex = lock_create("mutex");
  if(mutex == NULL){
    panic("could not create mutex synchronization lock");
  }
  return;
}

/* 
 * The CatMouse simulation will call this function once after all cat
 * and mouse simulations are finished.
 *
 * You can use it to clean up any synchronization and other variables.
 *
 * parameters: the number of bowls
 */
void
catmouse_sync_cleanup(int bowls)
{
  /* replace this default implementation with your own implementation of catmouse_sync_cleanup */
  // (void)bowls; /* keep the compiler from complaining about unused parameters */
  // KASSERT(globalCatMouseSem != NULL);
  // sem_destroy(globalCatMouseSem);
  (void)bowls;
  KASSERT(catMouseCV != NULL);
  KASSERT(catEatingCV != NULL);
  KASSERT(mouseEatingCV != NULL);
  KASSERT(mutex != NULL);
  cv_destroy(catMouseCV);
  cv_destroy(catEatingCV);
  cv_destroy(mouseEatingCV);
  lock_destroy(mutex);
}


/*
 * The CatMouse simulation will call this function each time a cat wants
 * to eat, before it eats.
 * This function should cause the calling thread (a cat simulation thread)
 * to block until it is OK for a cat to eat at the specified bowl.
 *
 * parameter: the number of the bowl at which the cat is trying to eat
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
cat_before_eating(unsigned int bowl) 
{
  /* replace this default implementation with your own implementation of cat_before_eating */
  // (void)bowl;  /* keep the compiler from complaining about an unused parameter */
  // KASSERT(globalCatMouseSem != NULL);
  // P(globalCatMouseSem);
  lock_acquire(mutex);
// kprintf("this cat wants to eat from bowl %d\n",bowl);
// kprintf("CAT DETECT! %d mice are eating\n",numOfMouseIsEating);
  KASSERT(catMouseCV != NULL);
  KASSERT(catEatingCV != NULL);
  KASSERT(mouseEatingCV != NULL);
  KASSERT(mutex != NULL);
//   while(numOfMouseIsEating != 0){
//     cv_wait(catMouseCV,mutex);
// kprintf("mouse is eating, this cat has to wait on bowl %d\n",bowl);
//   }
//   while(bowlsary[bowl-1] != 0){
//     cv_wait(catEatingCV,mutex);
// kprintf("this cat has to wait on bowl %d\n",bowl);
//   }

  while(numOfMouseIsEating > 0 || bowlsary[bowl-1] != 0){
    if(numOfMouseIsEating > 0){cv_wait(catMouseCV,mutex);}
    if(bowlsary[bowl-1] != 0){cv_wait(catEatingCV,mutex);}
  } 
//kprintf("this cat finishes waiting on bowl %d\n",bowl);
  bowlsary[bowl-1] = 1;
  numOfCatIsEating++;
//kprintf("%d cat are eating\n",numOfCatIsEating);
  lock_release(mutex);
}

/*
 * The CatMouse simulation will call this function each time a cat finishes
 * eating.
 *
 * You can use this function to wake up other creatures that may have been
 * waiting to eat until this cat finished.
 *
 * parameter: the number of the bowl at which the cat is finishing eating.
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
cat_after_eating(unsigned int bowl) 
{
  /* replace this default implementation with your own implementation of cat_after_eating */
  // (void)bowl;   // keep the compiler from complaining about an unused parameter 
  // KASSERT(globalCatMouseSem != NULL);
  // V(globalCatMouseSem);

  lock_acquire(mutex);
  KASSERT(catMouseCV != NULL);
  KASSERT(catEatingCV != NULL);
  KASSERT(mouseEatingCV != NULL);
  KASSERT(mutex != NULL);
  
  if(numOfMouseIsEating == 0){
    cv_signal(catEatingCV,mutex);
  }

  bowlsary[bowl-1] = 0;
//kprintf("this cat finishes on bowl %d\n",bowl);
  numOfCatIsEating--;
//kprintf("%d cat is still eating\n",numOfCatIsEating);
  if(numOfCatIsEating == 0){
    cv_signal(catMouseCV,mutex);
  }
  lock_release(mutex);
}

/*
 * The CatMouse simulation will call this function each time a mouse wants
 * to eat, before it eats.
 * This function should cause the calling thread (a mouse simulation thread)
 * to block until it is OK for a mouse to eat at the specified bowl.
 *
 * parameter: the number of the bowl at which the mouse is trying to eat
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
mouse_before_eating(unsigned int bowl) 
{
  /* replace this default implementation with your own implementation of mouse_before_eating */
  // (void)bowl;   //keep the compiler from complaining about an unused parameter 
  // KASSERT(globalCatMouseSem != NULL);
  // P(globalCatMouseSem);
  lock_acquire(mutex);
  KASSERT(catMouseCV != NULL);
  KASSERT(catEatingCV != NULL);
  KASSERT(mouseEatingCV != NULL);
  KASSERT(mutex != NULL);

//kprintf("MOUSE DETECT! %d cat are eating\n",numOfCatIsEating);
//   while(numOfCatIsEating != 0){
//     cv_wait(catMouseCV,mutex);
// kprintf("cat is eating, this mouse has to wait on bowl %d\n",bowl);
//   }
// kprintf("cat finishes eating, this mouse no longer to wait on bowl %d\n",bowl);
//   while(bowlsary[bowl-1] != 0){
//     cv_wait(mouseEatingCV,mutex);
// kprintf("other mouse is on bowl %d, this one has to wait\n",bowl);
//   }

  while(numOfCatIsEating > 0 || bowlsary[bowl-1] != 0){
    if(numOfCatIsEating > 0){cv_wait(catMouseCV,mutex);}
    if(bowlsary[bowl-1] != 0){cv_wait(mouseEatingCV,mutex);}
  } 


//kprintf("this mouse can eat on bowl %d\n",bowl);
  bowlsary[bowl-1] = 1;
  numOfMouseIsEating++;
//kprintf("%d mice are eating\n",numOfMouseIsEating);
  lock_release(mutex);
}

/*
 * The CatMouse simulation will call this function each time a mouse finishes
 * eating.
 *
 * You can use this function to wake up other creatures that may have been
 * waiting to eat until this mouse finished.
 *
 * parameter: the number of the bowl at which the mouse is finishing eating.
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
mouse_after_eating(unsigned int bowl) 
{
  /* replace this default implementation with your own implementation of mouse_after_eating */
  // (void)bowl;   keep the compiler from complaining about an unused parameter 
  // KASSERT(globalCatMouseSem != NULL);
  // V(globalCatMouseSem);
  lock_acquire(mutex);
  KASSERT(catMouseCV != NULL);
  KASSERT(catEatingCV != NULL);
  KASSERT(mouseEatingCV != NULL);
  KASSERT(mutex != NULL);

  if(numOfCatIsEating == 0){
    cv_signal(mouseEatingCV,mutex);
  }
//kprintf("this mouse finishes on bowl %d\n",bowl);
  bowlsary[bowl-1] = 0;
  
  numOfMouseIsEating--;
//kprintf("%d mouse is still eating\n",numOfMouseIsEating);
  if(numOfMouseIsEating == 0){
    cv_signal(catMouseCV,mutex);
  }
  lock_release(mutex);
}
