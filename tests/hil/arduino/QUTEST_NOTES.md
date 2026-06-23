---
id: QUTEST_NOTES
aliases: []
tags: []
---

Useful information when working with qutest/qspy for writing tests.

# Reminders

## Want to use timers?

### Starting timers in the microcontroller (Arduino framework)

```c
#include <Ticker.h> // arduino timer

Ticker l_ticker;
static void onTick() {
    QF_onClockTick();
}

void setup() {
    l_ticker.attach_ms(1, onTick);
}

extern "C" void QF_onClockTick(void) {
    QF_TICK_X(0U, (void *)0);
}
```

### Timers inside 

-> make sure to set current object in script before using tick:

```py
current_obj(OBJ_TE, "l_fifoCheckerAO.timer")
tick()
```

-> And set the object dictionary:

```c
QS_OBJ_DICTIONARY(&l_fifoCheckerAO.timer);
```
