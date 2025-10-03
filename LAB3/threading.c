#include <threading.h>
#include <stdio.h>
#include <stdlib.h>

//trampoline is used to be able to call task with arguments.
static void task_trampoline(uintptr_t fptr, intptr_t arg1, intptr_t arg2){
  void (*user_func)(int32_t, int32_t) = (void (*)(int32_t,int32_t))fptr; //make pointers into actual functions
  user_func((int32_t)arg1, (int32_t)arg2); //call the functions with 2 arguments
  t_finish(); //t finish is called to finish tasks
}

void t_init(){
  // TODO
  for (int i = 0; i < NUM_CTX; i++) { //by default, contests are marked invalid
    contexts[i].state = INVALID; 
  }
  if (getcontext(&contexts[0].context) == -1) { //capture the main program's context
    perror("getcontext");
    exit(1);
  }
  contexts[0].state = VALID; //main context marked valid
  current_context_idx = 0; //start with main context as first context
  return;
}

int32_t t_create(fptr foo, int32_t arg1, int32_t arg2)
{
  // TODO
  volatile int id = -1; //makes compiler not throw issues when id is changed
  for (int i = 1; i < NUM_CTX; i++){ //find open context slot in the matrix
    if (contexts[i].state == INVALID || contexts[i].state == DONE){
      id = i;
      break;
    }
  }
  if (id == -1) return 1; //if there is no open spot

  ucontext_t *ctx = &contexts[id].context; 
  if (getcontext(ctx) == -1) return 1; //initialize context
  void *stk = malloc(STK_SZ); //crestsa memory for new context
  if (!stk) return 1;

  ctx->uc_stack.ss_sp = stk;
  ctx->uc_stack.ss_size = STK_SZ;
  ctx->uc_stack.ss_flags = 0;
  ctx->uc_link = &contexts[0].context; //when context ends, return to main
  
  contexts[id].state = VALID; //mark context as ready
  
  makecontext(ctx, (void (*)())task_trampoline,3,(uintptr_t)foo, (intptr_t)arg1, (intptr_t)arg2); //set up trampoline to call task with the call arguments
  return 0;
}

int32_t t_yield()
{
  // TODO
  int valid_count = 0;
  for (int i = 0; i < NUM_CTX; i++) { //count # of contexts (not this one)
    if (i != current_context_idx && contexts[i].state == VALID){
      valid_count++;
    }
  }
  //there are no workers left
  if (valid_count == 0) return 0;

  int prev = current_context_idx;
  int next = (current_context_idx + 1) % NUM_CTX;
  
  while (contexts[next].state != VALID){ //find next valid worker
    next = (next + 1) % NUM_CTX;
    if (next == current_context_idx) break;
  }
  if (next != current_context_idx && contexts[next].state == VALID) { //if worker is fond, switch to it
    current_context_idx = (uint8_t)next;
    if (swapcontext(&contexts[prev].context, &contexts[next].context) == -1){
      perror("swapcontext");
      exit(1);
    }
  }
  return valid_count;
}
void t_finish()
{
  // TODO
  uint8_t idx = current_context_idx;
  
  contexts[idx].state = DONE; //mark worker as done
  if (contexts[idx].context.uc_stack.ss_sp){ //free stack
    free(contexts[idx].context.uc_stack.ss_sp);
    contexts[idx].context.uc_stack.ss_sp = NULL;
  }
  for (int i = 0; i < NUM_CTX; i++) { //switch to next valid worker 
    if (contexts[i].state == VALID) {
      current_context_idx = (uint8_t)i;
      setcontext(&contexts[i].context);
    }
  }
  for (int i = 1; i < NUM_CTX; i++) { //free stack of all done workers
    if (contexts[i].state == DONE && contexts[i].context.uc_stack.ss_sp) {
      free(contexts[i].context.uc_stack.ss_sp);
      contexts[i].context.uc_stack.ss_sp = NULL;
    }
  }
  //no workers left, switch back to main
  current_context_idx = 0;
  setcontext(&contexts[0].context);
  exit(0);
}
