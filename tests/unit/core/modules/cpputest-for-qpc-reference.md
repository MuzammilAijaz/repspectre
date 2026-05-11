---
id: cpputest-qpc-reference
aliases: []
tags: []
---
# Notes
- Posted events also go in global event pool and may cause leakage, if not consumed

- Using dummy objects (a recorder) requires custom clearing/flushing of all the events from the pool
  as it puts events into internal storage, preventing QP from freeing the memory.

# Test code snippets for various tasks

## check for posting of events to another AO
```c
// Dummy AO that will receive direct QACTIVE_POST events
auto dummy = std::unique_ptr<DefaultDummyActiveObject>(
        new DefaultDummyActiveObject(
            DefaultDummyActiveObject::EventBehavior::RECORDER));
// must be started before it can receive posts
dummy->dummyStart(qf_ctrl::UNIT_UNDER_TEST_PRIORITY - 1);
// IMPORTANT: redirect global pointer BEFORE stimulus
g_sensorAO = dummy->getQActive();

startAOAndMoveToBootingState(); // start boot

qf_ctrl::ProcessEvents();

// retrieve what was posted to the dummy AO
auto recordedEvent = dummy->getRecordedEvent();

CHECK_TRUE(recordedEvent != nullptr);
CHECK_EQUAL(INITIALIZE_MPU_SIG, recordedEvent->sig);
```
