///*****************************************************************************
/// WindowAO Tests
///-----------------------------------------------------------------------------
///
/// Expected States:
/// ----------------
///   WindowAO                 
///   ├── idle                 : initialized, but no processing done;
///   │                             -> this assumes inference is off as well
///   ├── accumulating         : actively working to build a window
///   │   └── backpressured    : when inference < sensor data
///   │       ├── retaining    : aggregation, instead of simply dropping data
///   │       └── dropping     : remove/override oldest data
///   └── error                : exists for recoverability/logging.
///   
///*****************************************************************************

// cpputest-for-qpc
#include "cmsTestPublishedEventRecorder.hpp"
#include "cms_cpputest_qf_ctrl.hpp"
#include "cmsQAssertMockSupport.hpp"

// cpputest
#include "CppUTest/TestHarness.h"

#include <array>

#include "events.h"
#include "sensorAO.h"
#include "windowAO.h"
#include "pub_sub_signals.h"

#include "unit_test_utils.hpp"

// Test group
TEST_GROUP(WindowAOGroup) {

    QActive* mUnderTest = nullptr; // Active Object under test

    std::unique_ptr<cms::test::DefaultDummyActiveObject> dummy_sensorAO = nullptr;

    // Storage for the event queue of the AO, holding 10 events max
    std::array<const QEvt*, 10> underTestEventQueueStorage;

    // Records the events
    cms::test::PublishedEventRecorder* mRecorder = nullptr;

    void setup() final {
        using namespace cms::test;
        using cms::test::qf_ctrl::MemPoolConfig;
        using cms::test::qf_ctrl::MemPoolConfigs;

        // Setup fake QP runtime with maximum signals and tick rate
        const MemPoolConfigs memPools = {
            MemPoolConfig{sizeof(uint64_t), 25},
            MemPoolConfig{sizeof(uint64_t) * 5, 10},
        };

        qf_ctrl::Setup(200, 200, memPools);

        // Create Event Recorder ; a cpputest-for-qpc mechanism for recording events
        mRecorder = PublishedEventRecorder::CreatePublishedEventRecorder(
            qf_ctrl::RECORDER_PRIORITY, // priority of the fake qp runtime
            Q_USER_SIG,
            MAX_PUB_SUB_SIG
        );

        setRecorder(mRecorder);
        dummy_sensorAO = setupDummyObject(&g_sensorAO, qf_ctrl::DUMMY_AO_A_PRIORITY);

        WindowAO_ctor();
        mUnderTest = g_windowAO; // this will be out AO under test
        CHECK_TRUE(mUnderTest != nullptr);
        CHECK_TRUE(dummy_sensorAO != nullptr);

        // Clear event queue buffer with nullptr
        underTestEventQueueStorage.fill(nullptr);
    }

    void teardown() final {
        flushDummyAOs();

        mock().checkExpectations();

        WindowAO_dtor();

        mUnderTest = nullptr; // this will be out AO under test
        dummy_sensorAO = nullptr;

        // clears cpputest mock subsystem
        mock().clear();

        // destory cpputest-for-qpc
        cms::test::qf_ctrl::Teardown();

        delete mRecorder;
    }

    /**
     * @brief Drains all recorded events from Dummy Active Objects to prevent memory leaks.
     *
     * Since the SequencerAO posts multiple events from the global pool (Q_NEW),
     * any event not explicitly retrieved via getRecordedEvent() remains allocated
     * in the dummy's internal recorder. This function flushes those queues to
     * ensure all pool memory is returned to the framework before test teardown.
     */
    void flushDummyAOs() {
        if (dummy_sensorAO) {
            while (dummy_sensorAO->isAnyEventRecorded()) {
                (void)dummy_sensorAO->getRecordedEvent(); // Pull and destroy
            }
        }
    }

    void startAOUnderTest() {
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

        CHECK_TRUE(WindowAO_isInState(STATE_IDLE));
    }

    void startAOUnderTestAndMoveToAccumulatingState() {
        using namespace cms::test;
        startAOUnderTest();

        auto* e = Q_NEW(QEvt, START_WINDOWING_SIG);
        qf_ctrl::PublishAndProcess(e, mRecorder);

        CHECK_TRUE(WindowAO_isInState(STATE_ACCUMULATING));
        auto recordedEvent = dummy_sensorAO->getRecordedEvent();
        CHECK_TRUE(recordedEvent != nullptr);
        CHECK_EQUAL(WRITE_LOCATION_SIG, recordedEvent->sig);
    }

};

using namespace cms::test;

//==============================================================================
// | Idle
//==============================================================================

TEST(WindowAOGroup, GivenConstructed_WhenStarted_ThenEntersIdleState)
{
    startAOUnderTest();
}

//==============================================================================
// | Accumulating
//==============================================================================

TEST(WindowAOGroup, GivenIdle_WhenWindowingRequest_ThenMoveToAccumulatingStateAndLeaseInitialWindow)
{
    startAOUnderTest();

    auto* e = Q_NEW(QEvt, START_WINDOWING_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);

    CHECK_TRUE(WindowAO_isInState(STATE_ACCUMULATING));
    auto recordedEvent = dummy_sensorAO->getRecordedEvent();
    CHECK_TRUE(recordedEvent != nullptr);
    CHECK_EQUAL(WRITE_LOCATION_SIG, recordedEvent->sig);
}

// Sample < Batch < Window < Arena

//===== Arena indexing =========================================================

// TODO: TEST(WindowAOGroup, GivenAccumulating_WhenSamplesWritten_ThenAdvanceCurrentWindowWriteIndex)
// TODO: TEST(WindowAOGroup, GivenAccumulating_WhenWindowBecomesFull_ThenMarkWindowReady)
// TODO: TEST(WindowAOGroup, GivenReadyWindow_WhenAnotherFreeWindowExists_ThenAdvanceToNextWindow)

//===== Managing Window States =================================================

// TODO: initial window state should be filling
// TODO: @inferenceAO_test.cpp when actively inferencing on the window = processing
// TODO: @inferenceAO_test.cpp when infernecing done change to FREE
// TODO: TEST(WindowAOGroup, GivenCurrentWindowFull_WhenFreeWindowExists_ThenLeaseIt)

// TODO: TEST(WindowAOGroup, GivenWindowReady_WhenInferenceCompletes_ThenMarkWindowFree)

//===== Communication ==========================================================

// REFACTOR: 
TEST(WindowAOGroup, GivenAccumulating_WhenExactWindowSizeIsReached_ThenPublishWindowReady)
{
    startAOUnderTestAndMoveToAccumulatingState();

    // fill a whole window

    // TODO: checkRecordedEventSignal(WINDOW_READY_SIG);
}

//===== Overflown arena ========================================================

// TODO: TEST(WindowAOGroup, GivenNoFreeWindows_WhenSamplesArrive_ThenDropOrRetainPerPolicy)

//===== Overlap and Stride =====================================================

// TODO:TEST(WindowAOGroup, GivenWindowReady_WhenStrideConfigured_ThenNextWindowStartsAtStrideOffset)

// TODO: the second infernece run should be given stride + window_size as the window

//==============================================================================
// | Backpressured
//==============================================================================
// given no FREE windows; apply appropriate/selected strategy

// TODO: TEST(WindowAOGroup, GivenNoFreeWindows_WhenWindowCompletes_ThenEnterBackpressuredState)
// TODO: GivenBackpressured_WhenInferenceIsSlow_ThenApplyConfiguredPolicy

//===== Retaining ==============================================================

// TODO: GivenRetaining_WhenNewSamplesArrive_ThenPreserveSharedHistory

//===== Dropping ===============================================================

// TODO: GivenDropping_WhenOverflowOccurs_ThenDiscardAsConfigured

//==============================================================================
// | Error
//==============================================================================

// TODO: GivenError_WhenFaultOccurs_ThenExposeRecoverabilitySignal
