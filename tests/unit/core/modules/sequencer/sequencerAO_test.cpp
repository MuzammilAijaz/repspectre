// cpputest-for-qpc
#include "cmsTestPublishedEventRecorder.hpp"
#include "cms_cpputest_qf_ctrl.hpp"
#include "cmsQAssertMockSupport.hpp"

// cpputest
#include "CppUTest/TestHarness.h"

#include "sequencerAO.h"
#include "Fake_BSP.h"

#include "pub_sub_signals.h"
#include <CppUTest/UtestMacros.h>
#include "unit_test_utils.hpp"

// Test group
TEST_GROUP(SequencerAOGroup)
{
    QActive* mUnderTest = nullptr; // Active Object under test

    // Storage for the event queue of the AO, holding 10 events max
    std::array<const QEvt*, 10> underTestEventQueueStorage;

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

    void startAOAndMoveToBootingState(void) {
        using namespace cms::test;

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

// TODO: handle error from bluetooth

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
