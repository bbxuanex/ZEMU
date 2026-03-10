#include <common.h>

static Context *do_event(Event e, Context *c)
{
  switch (e.event)
  {
    // TODO: add more event dispatching.
  case EVENT_YIELD:
    printf("Boy, Done!");
    break;
  default:
    panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void)
{
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
