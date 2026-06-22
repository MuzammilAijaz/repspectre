---
id: QUTEST_NOTES
aliases: []
tags: []
---

Useful information when working with qutest/qspy for writing tests.

# Reminders

## Want to use timers?

-> make sure to set current object in script before using tick:

```py
current_obj(OBJ_TE, "l_fifoCheckerAO.timer")
tick()
```

-> And set the object dictionary:

```c
QS_OBJ_DICTIONARY(&l_fifoCheckerAO.timer);
```
