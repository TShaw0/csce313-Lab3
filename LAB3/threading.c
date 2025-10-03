#include <threading.h>
#include <stdio.h>
#include <stdlib.h>

static void task_trampoline(uintptr_t fptr, intptr_t arg1, intptr_t arg2){
  void (*user_func)(int32_t, int32_t) = (void (*)(int32_t,int32_t))fptr;
  user_func((int32_t)arg1, (int32_t)arg2);
  t_finish();
}

void t_init(){
  // TODO
  for (int i = 0; i < NUM_CTX; i++) {
    contexts[i].state = INVALID;
  }
  if (getcontext(&contexts[0].context) == -1) {
    perror("getcontext");
    exit(1);
  }
  contexts[0].state = VALID;
  current_context_idx = 0;
  return;
}

int32_t t_create(fptr foo, int32_t arg1, int32_t arg2)
{
  // TODO
  volatile int id = -1;
  for (int i = 1; i < NUM_CTX; i++){
    if (contexts[i].state == INVALID || contexts[i].state == DONE){
      id = i;
      break;
    }
  }
  if (id == -1) return 1;

  ucontext_t *ctx = &contexts[id].context;
  if (getcontext(ctx) == -1) return 1;
  void *stk = malloc(STK_SZ);
  if (!stk) return 1;

  ctx->uc_stack.ss_sp = stk;
  ctx->uc_stack.ss_size = STK_SZ;
  ctx->uc_stack.ss_flags = 0;
  ctx->uc_link = &contexts[0].context;
  
  contexts[id].state = VALID;
  
  makecontext(ctx, (void (*)())task_trampoline,3,(uintptr_t)foo, (intptr_t)arg1, (intptr_t)arg2);
  return 0;
}

int32_t t_yield()
{
  // TODO
  int valid_count = 0;
  for (int i = 0; i < NUM_CTX; i++) {
    if (i != current_context_idx && contexts[i].state == VALID){
      valid_count++;
    }
  }
  //there are no workers left
  if (valid_count == 0) return 0;

  int prev = current_context_idx;
  int next = (current_context_idx + 1) % NUM_CTX;
  while (contexts[next].state != VALID){
    next = (next + 1) % NUM_CTX;
    if (next == current_context_idx) break;
  }
  if (next != current_context_idx && contexts[next].state == VALID) {
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
  
  contexts[idx].state = DONE;
  //free stack
  if (contexts[idx].context.uc_stack.ss_sp){
    free(contexts[idx].context.uc_stack.ss_sp);
    contexts[idx].context.uc_stack.ss_sp = NULL;
  }
  //switch to next valid worker
  for (int i = 0; i < NUM_CTX; i++) {
    if (contexts[i].state == VALID) {
      current_context_idx = (uint8_t)i;
      setcontext(&contexts[i].context);
    }
  }
  
  // no other workers, switch back to main
  current_context_idx = 0;
  setcontext(&contexts[0].context);
  exit(0);
}
