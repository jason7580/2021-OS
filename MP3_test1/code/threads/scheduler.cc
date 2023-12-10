// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

// MP3
int L1_cmp(Thread* t1, Thread* t2){
    return t1->cmptime >= t2->cmptime ? 1:-1;
}

int L2_cmp(Thread* t1, Thread* t2){
    return t1->priority <= t2->priority ? 1:-1;
}

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    readyList = new List<Thread *>; 
    toBeDestroyed = NULL;

    // MP3
    L1 = new SortedList<Thread *>(L1_cmp);
    L2 = new SortedList<Thread *>(L2_cmp);
    L3 = new List<Thread *>;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 

    // MP3
    delete L1;
    delete L2;
    delete L3;
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);
    // readyList->Append(thread);

    // MP3
    thread->enqueue = kernel->stats->totalTicks;
    if(thread->priority >= 100){
        L1->Insert(thread);
        DEBUG('z', "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L[1]");
    } else if(thread->priority >= 50){
        L2->Insert(thread);
        DEBUG('z', "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L[2]");
    } else{
        L3->Append(thread);
        DEBUG('z', "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L[3]");
    } 
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    // if (readyList->IsEmpty()) {
	// 	return NULL;
    // } else {
    // 	return readyList->RemoveFront();
    // }

    // MP3
    if (!L1->IsEmpty()) {
        Thread* thread = L1->RemoveFront();
        DEBUG('z', "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is removed from queue L[1]");
		return thread;
    } else if (!L2->IsEmpty()) {
		Thread* thread = L2->RemoveFront();
        DEBUG('z', "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is removed from queue L[2]");
		return thread;
    } else if (!L3->IsEmpty()) {
		Thread* thread = L3->RemoveFront();
        DEBUG('z', "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is removed from queue L[3]");
		return thread;
    } else {
    	return NULL;
    }

}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;

    // MP3
    nextThread->start = kernel->stats->totalTicks;
    DEBUG('z', "[E] Tick [" << kernel->stats->totalTicks << "]: Thread [" << nextThread->getID() << "] is now selected\
for execution, thread [" << oldThread->getID() << "] is replaced, and it has executed [" << oldThread->burst << "] ticks");
    oldThread->burst = 0;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyList->Apply(ThreadPrint);
}

// MP3
void
Scheduler::aging()
{
    Thread* thread;
    ListIterator<Thread *> *iter;
    int totalTicks = kernel->stats->totalTicks;
    int wait;

    if(!L1->IsEmpty()){
        iter = new ListIterator<Thread *>(L1);

        for(; !iter->IsDone(); iter->Next()){
            thread = iter->Item();

            if(thread->aging) wait = totalTicks - thread->aging;
            else wait = totalTicks - thread->enqueue;

            if(wait > 1500){
                int oldPriority = thread->priority;
                thread->aging = totalTicks;
                thread->priority = thread->priority + 10 > 149 ? 149 : thread->priority + 10;

                DEBUG('z', "[C] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] changes its priority from ["
                << oldPriority << "] to [" << thread->priority << "]");
            }
        }
        delete iter;
    }
    if(!L2->IsEmpty()){
        iter = new ListIterator<Thread *>(L2);

        for(; !iter->IsDone(); iter->Next()){
            thread = iter->Item();

            if(thread->aging) wait = totalTicks - thread->aging;
            else wait = totalTicks - thread->enqueue;

            if(wait > 1500){
                DEBUG('z', "[C] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] changes its priority from ["
                << thread->priority << "] to [" << thread->priority + 10 << "]");

                thread->aging = totalTicks;
                thread->priority += 10;
                if(thread->priority > 99){
                    L1->Insert(thread);
                    L2->Remove(thread);

                    DEBUG('z', "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is removed from queue L[2]");
                    DEBUG('z', "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L[1]");
                    
                    if(kernel->currentThread->priority < 100) kernel->currentThread->preempted = true;
                    else if(thread->predict < kernel->currentThread->predict) kernel->currentThread->preempted = true;
                }
            }
        }
        delete iter;
    }
    if(!L3->IsEmpty()){
        iter = new ListIterator<Thread *>(L3);

        for(; !iter->IsDone(); iter->Next()){
            thread = iter->Item();

            if(thread->aging) wait = totalTicks - thread->aging;
            else wait = totalTicks - thread->enqueue;

            if(wait > 1500){
                DEBUG('z', "[C] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] changes its priority from ["
                << thread->priority << "] to [" << thread->priority + 10 << "]");

                thread->aging = totalTicks;
                thread->priority += 10;
                if(thread->priority > 49){
                    L2->Insert(thread);
                    L3->Remove(thread);
                    DEBUG('z', "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is removed from queue L[3]");
                    DEBUG('z', "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L[2]");
                    
                    if(kernel->currentThread->priority < 50) kernel->currentThread->preempted = true;
                }
            }
        }
        delete iter;
    }
}