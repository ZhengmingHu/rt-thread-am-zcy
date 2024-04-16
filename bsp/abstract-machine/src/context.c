#include <am.h>
#include <klib.h>
#include <rtthread.h>

//#define GLOB_VAR
#define LOCAL_VAR

#define ADDR_ALIGN_BASE sizeof(uintptr_t)
#define ADDR_ALIGNED(addr) (((addr) + ADDR_ALIGN_BASE - 1)&(~((uintptr_t)(ADDR_ALIGN_BASE-1))))

#ifdef GLOB_VAR

rt_ubase_t current_context;
rt_ubase_t to_context;

static Context* ev_handler(Event e, Context *c) {
  switch (e.event) {
    case EVENT_YIELD:
        current_context = (rt_ubase_t)&c;
        c = *(Context **)to_context;
        c->mepc += 4;
      break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  to_context = to;
  yield();
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  from = current_context;
  to_context = to; 
  yield();
}
#endif

#ifdef LOCAL_VAR

static Context* ev_handler(Event e, Context *c) {
  switch (e.event) {
    case EVENT_YIELD:
        rt_thread_t thread = rt_thread_self();
        c = *(Context **)(thread->user_data);
        c->mepc += 4;
      break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  rt_thread_t thread = rt_thread_self();
  rt_ubase_t user_data_reg = thread->user_data;
  thread->user_data = to;
  yield();
  thread->user_data = user_data_reg;
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  rt_thread_t thread = rt_thread_self();
  rt_ubase_t user_data_reg = thread->user_data;
  from = thread->user_data;
  thread->user_data = to;
  yield();
  thread->user_data = user_data_reg;
}

#endif

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}

static void entry_wrapper (void *params) {
    rt_ubase_t* stack_top = (rt_ubase_t *)params;
    rt_ubase_t entry = *stack_top;
    stack_top--;
    rt_ubase_t parameter = *stack_top;
    stack_top--;
    rt_ubase_t exit = *stack_top;

    ((void (*)(void *))entry)((void *)parameter);
    ((void (*)())exit)();
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  Area kstack;
  kstack.end = (rt_uint8_t *) ADDR_ALIGNED((uintptr_t)stack_addr);
  kstack.start = (rt_uint8_t *) ADDR_ALIGNED((uintptr_t)stack_addr) - RT_MAIN_THREAD_STACK_SIZE;

  rt_ubase_t* stack_top = (rt_ubase_t *) kstack.end - sizeof(Context) - 1;
  Context* c = kcontext(kstack, entry_wrapper, (void *)(stack_top));
  *stack_top = (rt_ubase_t)tentry;
  stack_top--;
  *stack_top = (rt_ubase_t)parameter;
  stack_top--;
  *stack_top = (rt_ubase_t)texit; 
  
  return (rt_uint8_t*)c;
}
