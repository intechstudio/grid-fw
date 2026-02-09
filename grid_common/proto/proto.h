#ifndef PT_PROTO_H
#define PT_PROTO_H

/*
Copyright (c) 2004-2005, Swedish Institute of Computer Science.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. Neither the name of the Institute nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS `AS IS' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

Author: Adam Dunkels 
*/

/*
The protothread implementation used here is the one originally written by
Adam Dunkels (and Oliver Schmidt). I've lifted out only the parts relevant
to this project, and the changes were so slight that it still seems sane to
include the license under which their protothread library was released under.
*/

/** Local continuations
    
    Local continuations form the basis for the protothread implementation.
    A local continuation can be set in a function to capture its state.
    After it has been set, a local continuation can be resumed in order to
    restore the state of the function at the point where the local continuation
    was set.
*/

/** Implementation of local continuations based on the switch() statement.
*/

/* WARNING! lc implementation using switch() does not work if an
   LC_SET() is done within another switch() statement! */

typedef unsigned int proto_lc_t;

/** Initialize a local continuation.
  * Also unsets any previously set continuation state.
*/
#define LC_INIT(s) s = 0;

/** Resume a local continuation.
  * Resumes a previously set local continuation, restoring the state of the
  * function when the local continuation was set. If it was not set previously,
  * the resume operation does nothing.
*/
#define LC_RESUME(s) switch(s) { case 0:

/** Set a local continuation.
  * Saves the state of the function at the point where the operation was last
  * executed. This state does not include the call stack or local (automatic)
  * variables, only the program counter (and such CPU registers that need to be
  * saved).
*/
#define LC_SET(s) s = __LINE__; case __LINE__:

/** Mark the end of a local continuation usage.
  * The end operation signifies that local continuations should not be used
  * further on in the function. This operation is only required by some
  * implementations of local continuations, like the one here that relies on
  * the switch construct.
*/
#define LC_END(s) }

typedef struct proto_pt_t
{
    proto_lc_t lc;
} proto_pt_t;

#define PT_WAITING 0
#define PT_YIELDED 1
#define PT_EXITED  2
#define PT_ENDED   3

/** Initialize a protothread.
  * Initialization must be done prior to starting execution of the protothread.
*/
#define PT_INIT(pt) LC_INIT((pt)->lc)

/** Declaration of a protothread.
  * All protothreads must be declared with this macro, which constrains the
  * signature of the function implementing it to return a character.
*/
#define PT_THREAD(fun_name_and_args) char fun_name_and_args

/** Declare the start of a protothread inside the function that implements it.
  * Should be placed at the beginning of the function. All C statements above
  * this will be executed each time the protothread is scheduled.
*/
#define PT_BEGIN(pt) \
{ char PT_YIELD_FLAG = 1; (void)PT_YIELD_FLAG; \
  LC_RESUME((pt)->lc)

/** Declare the end of a protothread inside the function that implements it.
  * Always used together with a matching PT_BEGIN() macro.
*/
#define PT_END(pt) LC_END((pt)->lc); PT_YIELD_FLAG = 0; \
                   PT_INIT(pt); return PT_ENDED; }

/** Block and wait until a condition is true.
  * Blocks the protothread until the specified condition is true.
*/
#define PT_WAIT_UNTIL(pt, condition) \
do { \
    LC_SET((pt)->lc); \
    if(!(condition)){ return PT_WAITING; } \
} while(0)

/** Block and wait while a condition is true.
  * Blocks the protothread while the specified condition is true.
*/
#define PT_WAIT_WHILE(pt, condition) PT_WAIT_UNTIL(pt, !(condition))

/** Block and wait until a child protothread completes.
  * Schedules a child protohread. The current protothread will block until the
  * child protohread completes. The child protothread must be manually
  * initialized with the PT_INIT() macro before PT_WAIT_THREAD() is used.
*/
#define PT_WAIT_THREAD(pt, thread) PT_WAIT_WHILE((pt), PT_SCHEDULE(thread))

/** Spawn a child protothread and wait until it exits.
  * This macro can only be used inside a protothread.
*/
#define PT_SPAWN(pt, child, thread) \
do { \
    PT_INIT(child); \
    PT_WAIT_THREAD(pt, thread); \
} while(0)

/** Restart the protothread.
  * Causes the running protothread to block and to restart its execution
  * at the place of the PT_BEGIN() macro.
*/
#define PT_RESTART(pt) \
do { \
    PT_INIT(pt); \
    return PT_WAITING; \
} while(0)

/** Exit the protothread.
  * If the protothread was spawned by another one, the parent protothread will
  * become unblocked and can continue to run.
*/
#define PT_EXIT(pt) \
do { \
    PT_INIT(pt); \
    return PT_EXITED; \
} while(0)

/** Schedule a protothread.
  * The return value of this expression is non-zero if the protothread is
  * running, or zero if it has exited.
*/
#define PT_SCHEDULE(f) ((f) < PT_EXITED)

/** Yield from the current protothread.
  * Essentially, it acts as a one-time blocking wait to be rescheduled.
*/
#define PT_YIELD(pt) \
do { \
    PT_YIELD_FLAG = 0; \
    LC_SET((pt)->lc); \
    if(PT_YIELD_FLAG == 0){ return PT_YIELDED; } \
} while(0)

/** Yield from the current protothread until a condition occurs.
  * Essentially, it acts as a one-time blocking wait, and then a conditional
  * wait (always blocking at least once).
*/
#define PT_YIELD_UNTIL(pt, condition) \
do { \
    PT_YIELD_FLAG = 0; \
    LC_SET((pt)->lc); \
    if(PT_YIELD_FLAG == 0 || !(condition)){ return PT_YIELDED; } \
} while(0)

#endif /* PT_PROTO_H */
