// cpputest-for-qpc
#include "cmsTestPublishedEventRecorder.hpp"
#include "cms_cpputest_qf_ctrl.hpp"

#include "pub_sub_signals.h"
#include <CppUTest/UtestMacros.h>

/* Returns recorded event for further checking of event signal */
cms::QEvtUniquePtr checkRecordedEventSignal(PubSubSignal signal);
void setRecorder(cms::test::PublishedEventRecorder * recorder);

/**
 * @brief Creates a dummy active object and sets it to `g_aoPointer`.
 *
 * @usage Dummy AO has the ability to record events, which can be used to
 * see if an event has been posted to that particular active object (`g_aoPointer`).
 *
 * @code
 * auto recordedEvent = dummy->getRecordedEvent();
 *
 * CHECK_TRUE(recordedEvent != nullptr);
 * CHECK_EQUAL(INITIALIZE_MPU_SIG, recordedEvent->sig);
 * @endcode
 */
std::unique_ptr<cms::test::DefaultDummyActiveObject> setupDummyObject(QActive** g_aoPointer, uint8_t priority);
