// cpputest-for-qpc
#include "cmsTestPublishedEventRecorder.hpp"
#include "cms_cpputest_qf_ctrl.hpp"
#include "cmsQAssertMockSupport.hpp"

// cpputest
#include "CppUTest/TestHarness.h"

#include "sequencerAO.h"
#include "Fake_BSP.h"
#include "sensorAO.h"
#include "Fake_SensorAO.h"
#include "Fake_Sensor.h"

#include "pub_sub_signals.h"
#include <CppUTest/UtestMacros.h>
#include "unit_test_utils.hpp"

// Test group
TEST_GROUP(SequencerAOGroup)
{
    QActive* mUnderTest = nullptr; // Active Object under test
    QActive* Fake_sensorAO = nullptr;

    // Storage for the event queue of the sequencerAO, holding 10 events max
    std::array<const QEvt*, 10> underTestEventQueueStorage;

    // Storage for the event queue of the Fake_sensorAO, holding 10 events max
    std::array<const QEvt*, 10> Fake_sensorAOEventQueueStorage;

    // Records the events
    cms::test::PublishedEventRecorder* mRecorder = nullptr;

    void setup() final
    {
        using namespace cms::test;

        // Setup fake QP runtime with maximum signals and tick rate
        qf_ctrl::Setup(200, 200);

        // Create Event Recorder ; a cpputest-for-qpc mechanism for recording events
        mRecorder = PublishedEventRecorder::CreatePublishedEventRecorder(
            qf_ctrl::RECORDER_PRIORITY, // priority of the fake qp runtime
            Q_USER_SIG,
            MAX_PUB_SUB_SIG
        );

        setRecorder(mRecorder);

        SensorAO_ctor(&Fake_Sensor_interface);
        Fake_sensorAO = g_sensorAO;
        SequencerAO_ctor(&FakeBSPinterface);
        Fake_BSP_ctor();
        mUnderTest = g_sequencerAO; // this will be out AO under test
        CHECK_TRUE(mUnderTest != nullptr);

        // Clear event queue buffer with nullptr
        underTestEventQueueStorage.fill(nullptr);
    }

    void teardown() final
    {
        Fake_BSP_dtor();
        SequencerAO_dtor();
        SensorAO_dtor();
        mUnderTest = nullptr; // this will be out AO under test

        // clears cpputest mock subsystem
        mock().clear();

        // destory cpputest-for-qpc
        cms::test::qf_ctrl::Teardown();

        delete mRecorder;
    }

    void startAOUnderTest()
    {
        using namespace cms::test;

        QACTIVE_START(
            mUnderTest,
            qf_ctrl::UNIT_UNDER_TEST_PRIORITY,
            underTestEventQueueStorage.data(),
            underTestEventQueueStorage.size(),
            nullptr,
            0,
            nullptr
        );

        qf_ctrl::ProcessEvents();
    }

    void startSensorAO()
    {
        using namespace cms::test;

        QACTIVE_START(
            Fake_sensorAO,
            qf_ctrl::DUMMY_AO_A_PRIORITY,
            Fake_sensorAOEventQueueStorage.data(),
            Fake_sensorAOEventQueueStorage.size(),
            nullptr,
            0,
            nullptr
        );
        qf_ctrl::ProcessEvents();

        auto event = mRecorder->getRecordedEvent();
        CHECK_TRUE(event != nullptr); // make sure event is called from AO
        CHECK_EQUAL(MPU_UNINITIALIZED_SIG, event->sig);
    }

    void startAOAndMoveToBootingState(void) {
        using namespace cms::test;

        startSensorAO();
        startAOUnderTest();

        auto* e = Q_NEW(QEvt, START_BOOT_SIG);
        qf_ctrl::PublishAndProcess(e, mRecorder);
    }

};

TEST(SequencerAOGroup, AOSmokeTest)
{
    startAOUnderTest();
}

// =============================================================================
// | Initialization
// =============================================================================


// =============================================================================
// | Domain Logic
// =============================================================================

// ==== STATE: Booting =========================================================

// Enable/Configure sensor
// Start/Configure Bluetooth service
// Start/Configure Inference engine?
// Start/Configure MCU ; nvic, interrupts, clocks, etc.
TEST(SequencerAOGroup, GivenBooting_WhenBspInitialised_ThenRequestSensorInitialization)
{
    using namespace cms::test;

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
}

// TODO: handle error from bluetooth

// TODO: handle all succesfful initialization and transition.

// ==== STATE: Enabled =========================================================


// TODO: handle "CONIFGURED" from sensor and trasition accordingly
// TODO: handle "CONFIGURED" from bluetooth and transition accordingly

// =============================================================================
// | Error Handling
// =============================================================================

// BSP init fails
TEST(SequencerAOGroup, GivenBooting_WhenBspInitFails_ThenMoveToErrorStateAndSendSignal)
{
    using namespace cms::test;
    Fake_BSP_InjectError(); // inject before because next step will move it booting state

    startAOAndMoveToBootingState();

    checkRecordedEventSignal(ERROR_BSP_INIT);
}

// TODO: Bluetooth init fails
