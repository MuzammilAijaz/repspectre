#include "unit_test_utils.hpp"

// cpputest-for-qpc
#include "cmsDummyActiveObject.hpp"
#include "cmsTestPublishedEventRecorder.hpp"
#include "cms_cpputest_qf_ctrl.hpp"

#include "cassert"
// cpputest
#include "CppUTest/TestHarness.h"
#include "qevtUniquePtr.hpp"

#include <cstdio> // for snprintf

cms::test::PublishedEventRecorder * mRecorder = nullptr;

void setRecorder(cms::test::PublishedEventRecorder * recorder) {
    mRecorder = recorder;
}

const char* PubSubSignalToString(int sig) {
    // Static buffer so it persists after the function returns
    // (Note: Not thread-safe, but totally fine for single-threaded unit tests)
    static char buffer[32];

    switch (sig) {
        case STARTING_PUB_SUB_SIG:      return "STARTING_PUB_SUB_SIG";
        case START_BOOT_SIG:            return "START_BOOT_SIG";
        case SYSTEM_OPERATIONAL_SIG:    return "SYSTEM_OPERATIONAL_SIG";
        case INITIALIZE_BLUETOOTH_SIG:  return "INITIALIZE_BLUETOOTH_SIG";
        case BLUETOOTH_INITIALIZED_SIG: return "BLUETOOTH_INITIALIZED_SIG";
        case START_ADVERTISEMENT_SIG:   return "START_ADVERTISEMENT_SIG";
        case BLUETOOTH_CONNECTED_SIG:   return "BLUETOOTH_CONNECTED_SIG";
        case BLUETOOTH_DISCONNECTED_SIG:return "BLUETOOTH_DISCONNECTED_SIG";
        case BLUETOOTH_SEND_DATA_SIG:   return "BLUETOOTH_SEND_DATA_SIG";
        case BLUETOOTH_POLL_SIG:        return "BLUETOOTH_POLL_SIG";
        case INITIALIZE_MPU_SIG:        return "INITIALIZE_MPU_SIG";
        case MPU_INITIALIZED_SIG:       return "MPU_INITIALIZED_SIG";
        case MPU_UNINITIALIZED_SIG:     return "MPU_UNINITIALIZED_SIG";
        case MPU_ISR_SIG:               return "MPU_ISR_SIG";
        case MPU_FIFO_FULL:             return "MPU_FIFO_FULL";
        case SAMPLES_WRITTEN_SIG:       return "SAMPLES_WRITTEN_SIG";
        case START_WINDOWING_SIG:       return "START_WINDOWING_SIG";
        case WRITE_LOCATION_SIG:        return "WRITE_LOCATION_SIG";
        case WINDOW_READY_SIG:          return "WINDOW_READY_SIG";
        case ACTIVATE_MOTION_INFERENCE: return "ACTIVATE_MOTION_INFERENCE";
        case MOTION_STATE_CHANGED_SIG:  return "MOTION_STATE_CHANGED_SIG";
        case MOTION_REP_COUNTED_SIG:    return "MOTION_REP_COUNTED_SIG";
        case INFERENCE_DONE_SIG:        return "INFERENCE_DONE_SIG";
        case ERROR_SENSOR_I2C_MASTER:   return "ERROR_SENSOR_I2C_MASTER";
        case ERROR_BSP_INIT:            return "ERROR_BSP_INIT";
        case ERROR_BLUETOOTH_INIT:      return "ERROR_BLUETOOTH_INIT";
        case ERROR_BLUETOOTH_ADV:       return "ERROR_BLUETOOTH_ADV";

        case MAX_PUB_SUB_SIG:           return "MAX_PUB_SUB_SIG";
        default: {
            // Format unhandled/dynamic integers cleanly
            std::snprintf(buffer, sizeof(buffer), "UNKNOWN_SIG(%d)", sig);
            return buffer;
        }
    }
}

cms::QEvtUniquePtr checkRecordedEventSignal(PubSubSignal signal) {
    CHECK_TRUE(mRecorder != nullptr);

	cms::QEvtUniquePtr event = mRecorder->getRecordedEvent();
    CHECK_TRUE(event != nullptr);

    STRCMP_EQUAL(
        PubSubSignalToString(signal),
        PubSubSignalToString(static_cast<PubSubSignal>(event->sig))
    );

    return event;
}

std::unique_ptr<cms::test::DefaultDummyActiveObject> setupDummyObject(QActive** g_aoPointer, uint8_t priority) {

    using namespace cms::test;
    // Dummy AO that will receive direct QACTIVE_POST events
    auto dummy = std::unique_ptr<DefaultDummyActiveObject>(
      new DefaultDummyActiveObject(
        DefaultDummyActiveObject::EventBehavior::RECORDER));
    // must be started before it can receive posts
    dummy->dummyStart(priority);
    // IMPORTANT: redirect global pointer BEFORE stimulus
    *g_aoPointer = dummy->getQActive();

    return dummy;
}
