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

std::unique_ptr<cms::test::DefaultDummyActiveObject> setupDummyObject(QActive** g_aoPointer) {

    using namespace cms::test;
    // Dummy AO that will receive direct QACTIVE_POST events
    auto dummy = std::unique_ptr<DefaultDummyActiveObject>(
      new DefaultDummyActiveObject(
        DefaultDummyActiveObject::EventBehavior::RECORDER));
    // must be started before it can receive posts
    dummy->dummyStart(qf_ctrl::UNIT_UNDER_TEST_PRIORITY - 1);
    // IMPORTANT: redirect global pointer BEFORE stimulus
    *g_aoPointer = dummy->getQActive();

    return dummy;
}
