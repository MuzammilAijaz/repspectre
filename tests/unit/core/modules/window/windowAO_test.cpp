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

#include "sensorAO.h"
#include "windowAO.h"
#include "pub_sub_signals.h"

#include "unit_test_utils.hpp"

// Test group
TEST_GROUP(WindowAOGroup) {

    QActive* mUnderTest = nullptr; // Active Object under test

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
            MemPoolConfig{sizeof(MpuBatchEvent), 4},
        };

        qf_ctrl::Setup(200, 200, memPools);

        // Create Event Recorder ; a cpputest-for-qpc mechanism for recording events
        mRecorder = PublishedEventRecorder::CreatePublishedEventRecorder(
            qf_ctrl::RECORDER_PRIORITY, // priority of the fake qp runtime
            Q_USER_SIG,
            MAX_PUB_SUB_SIG
        );

        setRecorder(mRecorder);

        WindowAO_ctor();
        mUnderTest = g_windowAO; // this will be out AO under test
        CHECK_TRUE(mUnderTest != nullptr);

        // Clear event queue buffer with nullptr
        underTestEventQueueStorage.fill(nullptr);
    }

    void teardown() final {
        mock().checkExpectations();

        WindowAO_dtor();

        mUnderTest = nullptr; // this will be out AO under test

        // clears cpputest mock subsystem
        mock().clear();

        // destory cpputest-for-qpc
        cms::test::qf_ctrl::Teardown();

        delete mRecorder;
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
    }

    void sendMpuDataReadySignalWithEmptyBatch() {
        using namespace cms::test;
        SensorBatch batch = {};
        for (uint32_t j = 0; j < BATCH_SAMPLE_COUNT; ++j) {
            Axis3f a{0.0f, 0.0f, 0.0f};
            batch.samples[j] = {a, a, a, j + 1};
        }
        auto* e = Q_NEW(MpuBatchEvent, MPU_DATA_READY_SIG);
        e->batch = batch;
        qf_ctrl::PublishAndProcess(&e->super, mRecorder);
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

TEST(WindowAOGroup, GivenIdle_WhenWindowingRequest_ThenMoveToAccumulatingState)
{
    startAOUnderTest();

    auto* e = Q_NEW(QEvt, START_WINDOWING_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);

    CHECK_TRUE(WindowAO_isInState(STATE_ACCUMULATING));
}

// Sample < Batch < Window < Arena

//===== Arena indexing =========================================================

TEST(WindowAOGroup, GivenAccumulating_WhenOneSensorBatchArrives_ThenAppendBatchIntoArena)
{
    startAOUnderTestAndMoveToAccumulatingState();

    SensorBatch batch = {};
    for (uint32_t i = 0; i < BATCH_SAMPLE_COUNT; ++i) {
        Axis3f a{0.0f, 0.0f, 0.0f};
        batch.samples[i] = {a, a, a, i + 1};
    }

    auto* e = Q_NEW(MpuBatchEvent, MPU_DATA_READY_SIG);
    e->batch = batch;

    qf_ctrl::PublishAndProcess(&e->super, mRecorder);

    CHECK_EQUAL(BATCH_SAMPLE_COUNT, WindowAO_accumulatedSampleCount());
}

TEST(WindowAOGroup, GivenAccumulating_WhenMultipleBatchesArrive_ThenAccumulateUntilWindowSize)
{
    startAOUnderTestAndMoveToAccumulatingState();

    // fill a whole window
    for (uint32_t i = 0; i < WINDOW_BATCH_COUNT; i++) {
        sendMpuDataReadySignalWithEmptyBatch();
    }
    checkRecordedEventSignal(WINDOW_READY_SIG); // side-affect

    // a single window would be full
    CHECK_EQUAL(WINDOW_BATCH_COUNT * BATCH_SAMPLE_COUNT, WindowAO_accumulatedSampleCount());
}

TEST(WindowAOGroup, GivenAccumulating_WhenBatchOverflowsWindow_ThenKeepAppendingToNextWindow)
{
    startAOUnderTestAndMoveToAccumulatingState();

    // fill a whole window + 1 extra
    for (uint32_t i = 0; i < WINDOW_BATCH_COUNT + 1; i++) {
        sendMpuDataReadySignalWithEmptyBatch();
    }
    checkRecordedEventSignal(WINDOW_READY_SIG); // side-affect

    // a single window would be full + 1
    CHECK_EQUAL( (WINDOW_BATCH_COUNT + 1) * BATCH_SAMPLE_COUNT, WindowAO_accumulatedSampleCount());
    CHECK_EQUAL(1, WindowAO_getCurrentWindowIndex());
}

//===== Managing Window States =================================================

// TODO: initial state should be filling
// TODO: @inferenceAO_test.cpp when actively inferencing on the window = processing
// TODO: @inferenceAO_test.cpp when infernecing done change to FREE
// TODO: when done with window, make it ready (for inference)
// TODO: when moving to another window, check if its free before writing to it. ; its free
// TODO: when moving to another window, check if its free before writing to it. ; its not free

//===== Communication ==========================================================

TEST(WindowAOGroup, GivenAccumulating_WhenExactWindowSizeIsReached_ThenPublishWindowReady)
{
    startAOUnderTestAndMoveToAccumulatingState();

    // fill a whole window
    for (uint32_t i = 0; i < WINDOW_BATCH_COUNT; i++) {
        sendMpuDataReadySignalWithEmptyBatch();
    }

    auto e = checkRecordedEventSignal(WINDOW_READY_SIG);
    (void)reinterpret_cast<const WindowReadyEvent*>(e.get());
}

//===== Overflown arena ========================================================

// TODO: TEST(WindowAOGroup, GivenAccumulating_WhenBatchOverflowsArena_Then???)

//===== Overlap and Stride =====================================================

// TODO: GivenAccumulating_WhenNextBatchArrives_ThenAdvanceByConfiguredStride

// TODO: the second infernece run should be given stride + window_size as the window

//==============================================================================
// | Backpressured
//==============================================================================
// given no FREE windows; apply appropriate/selected strategy

// TODO: GivenBackpressured_WhenInferenceIsSlow_ThenApplyConfiguredPolicy

//===== Retaining ==============================================================

// TODO: GivenRetaining_WhenNewSamplesArrive_ThenPreserveSharedHistory

//===== Dropping ===============================================================

// TODO: GivenDropping_WhenOverflowOccurs_ThenDiscardAsConfigured

//==============================================================================
// | Error
//==============================================================================

// TODO: GivenError_WhenFaultOccurs_ThenExposeRecoverabilitySignal
