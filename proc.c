/*****************************************************************
*       proc.c - simplified for CPSC405 Lab by Gusty Cooper, University of Mary Washington
*       adapted from MIT xv6 by Zhiyi Huang, hzy@cs.otago.ac.nz, University of Otago
*		Modified by Karl Ast and Pratima Kandel
********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "defs.h"
#include "proc.h"
#include <stdbool.h>
#include <time.h>

static void wakeup1(int chan);
int arrivalt = 1;


// Dummy lock routines. Not needed for lab
void acquire(int *p) {
    return;
}

void release(int *p) {
    return;
}

// enum procstate for printing
char *procstatep[] = { "UNUSED", "EMPRYO", "SLEEPING", "RUNNABLE", "RUNNING", "ZOMBIE" };

// Table of all processes
struct {
  int lock;   // not used in Lab
  struct proc proc[NPROC];
} ptable;

// Initial process - ascendent of all other processes
static struct proc *initproc;
struct proc *head;
// Used to allocate process ids - initproc is 1, others are incremented
int nextpid = 1;

// Funtion to use as address of proc's PC
void
forkret(void)
{
}

// Funtion to use as address of proc's LR
void
trapret(void)
{
}

// Initialize the process table
void
pinit(void)
{
  memset(&ptable, 0, sizeof(ptable));
}
//find proc for roundrobin
static struct proc* find_proc(int pid) {
    // design and implement this function
   struct proc *p = head;
   while(p != NULL){
    if( p -> pid == pid){
       return p;
    }
   p = p->next;

   }
    return 0;
}
// Look in the process table for a process id
// If found, return pointer to proc
// Otherwise return 0.
static struct proc*
findproc(int pid)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->pid == pid)
      return p;
  return 0;
}

//reomve the process from  the front of the  queue
//This is used for the Round Robin schedule algorithm
int dequeue_proc(){
        struct proc *del;
        if( head == NULL){
        return 0;

        }
        if( head ->next == NULL){
           head = NULL;
           return 0;
        }else{
        del = head;
        head = head -> next;
        head -> prev = NULL;
        return 1;
        }

}

//changes front of the queue process's state to RUNNING and the others RUNNABLE 
//This function is used when the schedule algorithm change from Fair to Round Robin 
//Since RR have different Data structure and algorithm, by changing the state we can be more consitent with the RR's algorithm.
void clear_rr(){

struct proc *s = head;
if (s != NULL){

	s->state = RUNNING;
}
while(s->next != NULL){
       s->next->state = RUNNABLE;
       s = s->next;
}
return;
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  srand(time(NULL));
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->next = NULL;
  p->prev = NULL;
  //Assign burst time
  p->bursttime = (rand() % 10) + 1;
  p->arrivaltime = arrivalt++; 
  
  //Assign niceness
  p->niceness = rand() % 40 - 20;
  
  //Assign time until completion
  p->virtual_runtime = rand() % 5 + 1;

  p->context = (struct context*)malloc(sizeof(struct context));
  memset(p->context, 0, sizeof *p->context);
  p->context->pc = (uint)forkret;
  p->context->lr = (uint)trapret;
 
  return p;
}

// Set up first user process.
int
userinit(void)
{
  struct proc *p;
  p = allocproc();
  initproc = p;
  p->sz = PGSIZE;
  strcpy(p->cwd, "/");
  strcpy(p->name, "userinit"); 
  p->state = RUNNING;
  curr_proc = p;
  head = curr_proc;
  return p->pid;
}

//inserts the queue at the end
// This is used for the RR scheduling
int enqueue_proc(struct proc *p) {
    if ( p == NULL){
          return 0;
}
    struct proc *s = head;
	if(s == NULL){
		head = p;
		return 1;
	}

            p->next = NULL;
        if (s->next == NULL){
          
                p->prev=s;
                s->next=p;
        }else{
                
        while (s->next != NULL){
                s = s->next;

        }

        s->next = p;
        p->prev = s;
}

return 1;
}
// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
Fork(int fork_proc_id)
{
  int pid;
  struct proc *np, *fork_proc;

  // Find current proc
  if ((fork_proc = findproc(fork_proc_id)) == 0)
    return -1;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  np->sz = fork_proc->sz;
  np->parent = fork_proc;
  // Copy files in real code
  strcpy(np->cwd, fork_proc->cwd);
 
  pid = np->pid;
  struct proc *s = head;
   if( s == NULL){
	   np->state = RUNNING;
   }else{
  	np->state = RUNNABLE;
   }
  strcpy(np->name, fork_proc->name);
  //enqueue the process
  enqueue_proc(np);
  return pid;
}

//Removes the process from the queue if it finds the pid by using find_pid process.

int kill_rr(int proc_id){


  struct proc *s = find_proc(proc_id);
    if(s == NULL){
      return -1;
   }
   if(s->prev == NULL){
      dequeue_proc();
        return 0;
    }

   if( s->next != NULL){
      struct proc *next = s->next;
      struct proc *prev = s->prev;
      prev->next = next;
      next->prev = prev;

   }else{
      struct proc *prev = s->prev;
      prev->next = NULL;
    }

 return 0;



}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
int
Exit(int exit_proc_id)
{
 
  //kills the process if it exists from the queue 
  kill_rr(exit_proc_id);
  struct proc *p, *exit_proc;

  // Find current proc
  if ((exit_proc = findproc(exit_proc_id)) == 0)
    return -2;

  if(exit_proc == initproc) {
    printf("initproc exiting\n");
    return -1;
  }

  // Close all open files of exit_proc in real code.

  acquire(&ptable.lock);

  wakeup1(exit_proc->parent->pid);

  // Place abandoned children in ZOMBIE state - HERE
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == exit_proc){
      p->parent = initproc;
      p->state = ZOMBIE;
    }
  }

  exit_proc->state = ZOMBIE;

  // sched();
  release(&ptable.lock);
  return 0;
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
// Return -2 has children, but not zombie - must keep waiting
// Return -3 if wait_proc_id is not found
int
Wait(int wait_proc_id)
{
  struct proc *p, *wait_proc;
  int havekids, pid;

  // Find current proc
  if ((wait_proc = findproc(wait_proc_id)) == 0)
    return -3;

  acquire(&ptable.lock);
  for(;;){ // remove outer loop
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != wait_proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        p->kstack = 0;
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || wait_proc->killed){
      release(&ptable.lock);
      return -1;
    }
    if (havekids) { // children still running
      Sleep(wait_proc_id, wait_proc_id);
      release(&ptable.lock);
      return -2;
    }

  }
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
int
Sleep(int sleep_proc_id, int chan)
{
 //removes the pid from the queue if it exists
  kill_rr(sleep_proc_id);
  struct proc *sleep_proc;
  // Find current proc
  if ((sleep_proc = findproc(sleep_proc_id)) == 0)
    return -3;

  sleep_proc->chan = chan;
  sleep_proc->state = SLEEPING;
  return sleep_proc_id;
}

//show the sleeping proc
//Used for RR scheduling 
void show_sleep(){


  struct proc *p;
  bool t = false;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == SLEEPING){
        printf("sleeping pid: %d chan: %d\n", p->pid, p->chan);
        t = true;
        }
}
if (t == false){
printf("No Process in Sleep\n");
}
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(int chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
     //enqueue the sleeping proc
      enqueue_proc(p);
    }
}


void
Wakeup(int chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}



// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
Kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(int algorithm)
{
// A continous loop in real code
//  if(first_sched) first_sched = 0;
//  else sti();

  int burst = 0;
  int new_burst = 0;
  struct proc *s = head;

  struct proc *p;
  
  
  switch(algorithm) {  
	case ROUNDROBIN:	
                //Round Robin
		if (s == NULL){
			return;
		}

		burst = s->bursttime;
		s-> state = RUNNABLE;
		//check for the burst time
		if(burst > QUANTUM){
			new_burst = burst - QUANTUM;
			s->bursttime = new_burst;
			struct proc *d = s;
                        //dequeue proc
			dequeue_proc();
			if(s->next != NULL){
				s = s->next;
			}
			//change the next process schedule to running
			s->state = RUNNING;
			//enqueue proc
			enqueue_proc(d);
          
		}else{
			
			dequeue_proc();
			if( s->next != NULL){
				s=s->next;
			}
			}
			//change the next process schedule to running
		        s->state = RUNNING;
		break;
	
  
		case FAIR:
			//Fair Scheduling
			
			
			//Pause current running process
			acquire(&ptable.lock);
			for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
				if(p->state == RUNNING){
					p->state = RUNNABLE;
					break;
				}
			}
			
			release(&ptable.lock);

			acquire(&ptable.lock);
		
			//Find the lowest nice value.
			int nicest = LEAST_NICE;
			
			for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
				if(p->state != RUNNABLE)
				continue;	

				if (nicest > p->niceness)
					nicest = p->niceness;
			}
			
			//Choose the process that has the lowest nice value
			for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
				if(p->niceness == nicest && p->state == RUNNABLE) {
					// Switch to chosen process.
					curr_proc = p;
					p->state = RUNNING;
					break;
				}
			}
			release(&ptable.lock);
			
			//Decrement Virtual Runtime
			acquire(&ptable.lock);
			for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
				if(p->state == RUNNING) {
					p->virtual_runtime--;
					if (p->virtual_runtime <= 0) {
						p->state = ZOMBIE;
					}
				}
			}
			release(&ptable.lock);
			
			break;
	}
	
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->pid > 0)
      printf("pid: %d, parent: %d, state: %s, niceness: %d, virtual runtime: %d\n", p->pid, p->parent == 0 ? 0 : p->parent->pid, procstatep[p->state], p->niceness, p->virtual_runtime);
}

//Prints process from the queue 
void print_proc(struct proc *p) {
    if (p == NULL)
        return;
    printf("pid: %d, parent: %d state: %s arrival time: %d burst time: %d\n", p->pid, p->parent == 0 ? 0 : p->parent->pid, procstatep[p->state], p->arrivaltime, p->bursttime);
}

//Prints process from the queue.
void print_procs() {
    struct proc *p = head;
    if( head == NULL){
        return;
     }else{
    do {
        print_proc(p);
        p = p->next;
    } while (p != NULL);

    printf("\n");
}
}
