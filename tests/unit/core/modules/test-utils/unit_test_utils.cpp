#include "unit_test_utils.hpp"

// cpputest-for-qpc
#include "cmsTestPublishedEventRecorder.hpp"
#include "cms_cpputest_qf_ctrl.hpp"
#include "cmsQAssertMockSupport.hpp"

#include "cassert"
// cpputest
#include "CppUTest/TestHarness.h"
#include "qevtUniquePtr.hpp"

cms::test::PublishedEventRecorder * mRecorder = nullptr;

void setRecorder(cms::test::PublishedEventRecorder * recorder) {
    mRecorder = recorder;
}

cms::QEvtUniquePtr checkRecordedEventSignal(PubSubSignal signal) {
    CHECK_TRUE(mRecorder != nullptr);

	cms::QEvtUniquePtr event = mRecorder->getRecordedEvent();
    CHECK_TRUE(event != nullptr);
    CHECK_EQUAL(signal, event->sig);

    return event;
}
