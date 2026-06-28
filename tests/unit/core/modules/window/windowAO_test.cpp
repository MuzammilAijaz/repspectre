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

TEST(WindowAOGroup, GivenConstructed_WhenStarted_ThenInitializeWindowStartAndEndIndices)
{
    startAOUnderTest();

    WindowArena* arena = WindowAO_getArena();

    // for every window
    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {

        uint16_t expectedStart = (uint16_t)(i * STRIDE_SAMPLE_COUNT);
        // the end for the last window will loop around
        uint16_t expectedEnd = 
            (uint16_t)((expectedStart + WINDOW_SAMPLE_COUNT - 1) % ARENA_TOTAL_SAMPLES);

        CHECK_EQUAL(expectedStart, arena->windows[i].startIndex);
        CHECK_EQUAL(expectedEnd, arena->windows[i].endIndex);
    }
}

TEST(WindowAOGroup, GivenConstructed_WhenLastWindowInitialized_ThenEndIndexWrapsCorrectly)
{
    WindowAO_ctor();

    WindowArena* arena = WindowAO_getArena();

    WindowBuffer* last = &arena->windows[ARENA_WINDOW_COUNT - 1];
    uint16_t expectedEnd =
        (uint16_t)( (last->startIndex + WINDOW_SAMPLE_COUNT - 1) % ARENA_TOTAL_SAMPLES);

    CHECK_EQUAL(expectedEnd, last->endIndex);
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

TEST(WindowAOGroup, GivenIdle_WhenWindowingRequest_ThenSetAllWindowsAsFree)
{
    startAOUnderTestAndMoveToAccumulatingState();

    WindowArena* arena = WindowAO_getArena();
    for (int i = 0; i < ARENA_WINDOW_COUNT; i++) {
        CHECK_EQUAL(WINDOW_STATE_FREE, arena->windows[i].state);
    }
}

// Sample < Batch < Window < Arena

//===== Arena indexing =========================================================

TEST(WindowAOGroup, GivenAccumulating_WhenSamplesWritten_ThenAdvanceCurrentWindowSampleCountByWIDNOW_SAMPLE_COUNT)
{
    startAOUnderTestAndMoveToAccumulatingState();

    WindowBuffer* curr = WindowAO_getCurrentFillingWindow();
    CHECK_TRUE(curr != nullptr);
    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    WindowArena* arena = WindowAO_getArena();
    CHECK_TRUE(arena->headIndex == WINDOW_SAMPLE_COUNT);
}

TEST(WindowAOGroup, GivenAccumulating_WhenSamplesWritten_ThenSetWindowAsReady)
{
    startAOUnderTestAndMoveToAccumulatingState();

    // we do a double write, because first write always sets WINDOW_STATE_PROCESSING
    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);
    auto* e2 = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e2, mRecorder);

    WindowArena* arena = WindowAO_getArena();
    CHECK_TRUE(arena->windows[1].state == WINDOW_STATE_READY);
}

TEST(WindowAOGroup, GivenAccumulating_WhenSamplesWritten_ThenAdvanceCurrentFillingWindow)
{
    startAOUnderTestAndMoveToAccumulatingState();

    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    // This should be the new window
    WindowBuffer* curr = WindowAO_getCurrentFillingWindow();
    CHECK_TRUE(curr != nullptr);
    CHECK_TRUE(curr->state == WINDOW_STATE_FILLING);
}

TEST(WindowAOGroup, GivenAccumulatingAndFirstSample_WhenSampleWritten_ThenHeadIndexIsIncrementedByWindowSampleCount)
{
    startAOUnderTestAndMoveToAccumulatingState();

    WindowArena* arena = WindowAO_getArena();
    CHECK_EQUAL(arena->headIndex, 0);

    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    CHECK_EQUAL(arena->headIndex, WINDOW_SAMPLE_COUNT);
}

TEST(WindowAOGroup, GivenAccumulatingAndNotFirstSample_WhenSampleWritten_ThenHeadIndexIsIncrementedByStrideSampleCount)
{
    startAOUnderTestAndMoveToAccumulatingState();

    WindowArena* arena = WindowAO_getArena();
    CHECK_EQUAL(arena->headIndex, 0);

    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    auto* e2 = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e2, mRecorder);

    CHECK_EQUAL(arena->headIndex, WINDOW_SAMPLE_COUNT + STRIDE_SAMPLE_COUNT);
}

// TODO: last window

//===== Finding Free Windows ===================================================

TEST(WindowAOGroup, GivenAccumulating_WhenSamplesWritten_ThenWindowAndHeadIndexSynced)
{
    startAOUnderTestAndMoveToAccumulatingState();

    WindowArena* arena = WindowAO_getArena();
    // This starts inference on window 0
    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    // Set windows 1 through 11 to READY (making window 12 FILLING)
    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT - 2; i++) {
        auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
        qf_ctrl::PublishAndProcess(e, mRecorder);
        
        while (dummy_sensorAO->isAnyEventRecorded()) {
            (void)dummy_sensorAO->getRecordedEvent();
        }
    }

    // Now window 0 is PROCESSING, windows 1..11 are READY, window 12 is FILLING.
    // Finish inference on window 0, freeing it and transitioning window 1 to PROCESSING.
    auto* e2 = Q_NEW(QEvt, INFERENCE_DONE_SIG);
    qf_ctrl::PublishAndProcess(e2, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);
    CHECK_EQUAL(WINDOW_STATE_FREE, arena->windows[0].state);

    // Write to window 12 (making it READY). This will lease window 0 (making it FILLING).
    auto* e3 = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e3, mRecorder);
    while (dummy_sensorAO->isAnyEventRecorded()) {
        (void)dummy_sensorAO->getRecordedEvent();
    }

    // Finish inference on window 1, freeing it and transitioning window 2 to PROCESSING.
    auto* e4 = Q_NEW(QEvt, INFERENCE_DONE_SIG);
    qf_ctrl::PublishAndProcess(e4, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    // Write to window 0 (making it READY). This will lease window 1 (making it FILLING).
    auto* e5 = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e5, mRecorder);

    CHECK_EQUAL(WINDOW_STATE_READY, arena->windows[0].state);
}

TEST(WindowAOGroup, GivenAccumulating_WhenSamplesWritten_ThenShouldWriteOnlyToFreeWindows_A)
{
    startAOUnderTestAndMoveToAccumulatingState();

    // only make the last window free
    // ASSUMPTION: ARENA_WINDOW_COUNT = 4
    WindowArena* arena = WindowAO_getArena();
    arena->windows[1].state = WINDOW_STATE_PROCESSING;
    arena->windows[2].state = WINDOW_STATE_PROCESSING;

    // "write" once
    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    // This should write to the last window!
    auto* e2 = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e2, mRecorder);

    // Ensure other states remained untouched
    CHECK_EQUAL(WINDOW_STATE_PROCESSING, arena->windows[1].state);
    CHECK_EQUAL(WINDOW_STATE_PROCESSING, arena->windows[2].state);
    // main assert
    CHECK_EQUAL(WINDOW_STATE_READY, arena->windows[3].state);
}

TEST(WindowAOGroup, GivenAccumulating_WhenSamplesWritten_ThenShouldWriteOnlyToFreeWindows_B)
{
    startAOUnderTestAndMoveToAccumulatingState();

    // ASSUMPTION: ARENA_WINDOW_COUNT = 4
    WindowArena* arena = WindowAO_getArena();
    arena->windows[1].state = WINDOW_STATE_PROCESSING;
    arena->windows[3].state = WINDOW_STATE_PROCESSING;

    // "write" once
    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    auto* e2 = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e2, mRecorder);

    CHECK_EQUAL(WINDOW_STATE_PROCESSING, arena->windows[1].state);
    CHECK_EQUAL(WINDOW_STATE_PROCESSING, arena->windows[3].state);
    // main assert
    CHECK_EQUAL(WINDOW_STATE_READY, arena->windows[2].state);
}

// TODO:
TEST(WindowAOGroup, GivenAccumulating_WhenSamplesWritten_ThenShouldWriteOnlyToFreeWindows_C)
{
    startAOUnderTestAndMoveToAccumulatingState();

    // A realistic scenario: we fill windows until all but one are occupied
    // (inference was only started once, so window[0] is still PROCESSING).
    // We stop one write short of exhausting all free windows to avoid
    // triggering the unimplemented backpressure path (Q_ASSERT).

    // First write: window[0] -> PROCESSING (firstWrite path), window[1] -> FILLING
    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    // Fill windows[1] through windows[ARENA_WINDOW_COUNT - 2], leaving
    // windows[ARENA_WINDOW_COUNT - 1] as FREE.
    // Loop runs ARENA_WINDOW_COUNT - 2 times (not -1) to avoid exhausting all windows.
    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT - 2; i++) {
        auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
        qf_ctrl::PublishAndProcess(e, mRecorder);
        
        // Drain events to prevent event pool exhaustion
        while (dummy_sensorAO->isAnyEventRecorded()) {
            (void)dummy_sensorAO->getRecordedEvent();
        }
    }

    WindowArena* arena = WindowAO_getArena();

    // window[0] was the first window processed; it is still PROCESSING (inference not done)
    CHECK_EQUAL(WINDOW_STATE_PROCESSING, arena->windows[0].state);

    // windows[1] through [ARENA_WINDOW_COUNT - 2] have been filled and are READY
    for (uint16_t i = 1; i <= ARENA_WINDOW_COUNT - 2; i++) {
        CHECK_EQUAL(WINDOW_STATE_READY, arena->windows[i].state);
    }

    // The last window should still be FILLING (we leased it but haven't finished filling it)
    CHECK_EQUAL(WINDOW_STATE_FILLING, arena->windows[ARENA_WINDOW_COUNT - 1].state);
}

// TODO: TEST(WindowAOGroup, GivenAccumulatingAndNoFreeWindow_WhenSamplesWritten_Then???)

//===== Managing Window States =================================================

// TODO: initial window state should be filling
// TODO: @inferenceAO_test.cpp when actively inferencing on the window = processing
// TODO: @inferenceAO_test.cpp when infernecing done change to FREE

//===== Managing Sensor ========================================================

TEST(WindowAOGroup, GivenAccumulating_WhenSampleWritten_ThenLeaseMemoryToSensorAO)
{
    startAOUnderTestAndMoveToAccumulatingState();

    // this should lead to lease
    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    auto recordedEvent = dummy_sensorAO->getRecordedEvent();
    CHECK_TRUE(recordedEvent != nullptr);
    CHECK_EQUAL(WRITE_LOCATION_SIG, recordedEvent->sig);
}

TEST(WindowAOGroup, GivenAccumulatingAndFirstSample_WhenSampleWritten_ThenRequestFullWindowWrite)
{
    startAOUnderTestAndMoveToAccumulatingState();

    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    auto event = dummy_sensorAO->getRecordedEvent();
    CHECK_TRUE(event != nullptr);
    auto evt = reinterpret_cast<const WriteLocationEvent*>(event.get());
    CHECK_EQUAL(WINDOW_SAMPLE_COUNT,  evt->maxSamples);
}

TEST(WindowAOGroup, GivenAccumulatingAndNotFirstSample_WhenSampleWritten_ThenRequestStrideWrite)
{
    startAOUnderTestAndMoveToAccumulatingState();

    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);
    dummy_sensorAO->getRecordedEvent(); // drop

    auto* e2 = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e2, mRecorder);

    auto event = dummy_sensorAO->getRecordedEvent();
    CHECK_TRUE(event != nullptr);
    auto evt = reinterpret_cast<const WriteLocationEvent*>(event.get());
    CHECK_EQUAL(STRIDE_SAMPLE_COUNT,  evt->maxSamples);
}

//===== Managing Inference =====================================================

TEST(WindowAOGroup, GivenAccumulatingAndFirstSampleWritten_WhenSampleWritten_ThenPublish_WINDOW_STATE_READY_WithReadyWindow)
{
    startAOUnderTestAndMoveToAccumulatingState();

    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);

    // verify processing
    WindowArena* arena = WindowAO_getArena();
    WindowBuffer* window = &arena->windows[0]; // should point to the first window
    auto event = checkRecordedEventSignal(WINDOW_READY_SIG);
    auto responseEvt = reinterpret_cast<const WindowReadyEvent*>(event.get());
    CHECK_EQUAL(window->startIndex,  responseEvt->window->startIndex);
    CHECK_EQUAL(WINDOW_STATE_PROCESSING,  responseEvt->window->state);
}

TEST(WindowAOGroup, GivenAccumulating_WhenInferenceDone_ThenPublish_WINDOW_READY_SIG_WithAValidReadyWindow)
{
    startAOUnderTestAndMoveToAccumulatingState();

    // This should turn the first window into processing state
    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    // second one to have another ready window
    auto* e3 = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e3, mRecorder);

    // prompt for a new ready window; should return 2nd window now
    auto* e2 = Q_NEW(QEvt, INFERENCE_DONE_SIG);
    qf_ctrl::PublishAndProcess(e2, mRecorder);

    // this should be the window posted by the WindowAO
    WindowArena* arena = WindowAO_getArena();
    WindowBuffer* window = &arena->windows[1];

    auto event = checkRecordedEventSignal(WINDOW_READY_SIG);
    auto responseEvt = reinterpret_cast<const WindowReadyEvent*>(event.get());
    CHECK_EQUAL(window->startIndex,  responseEvt->window->startIndex);
    CHECK_EQUAL(WINDOW_STATE_PROCESSING,  responseEvt->window->state);
}

TEST(WindowAOGroup, GivenAccumulating_WhenInferenceDone_ThenChangeTheWindowToFree)
{
    startAOUnderTestAndMoveToAccumulatingState();

    auto* e = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    // second one to have another ready window
    auto* e3 = Q_NEW(QEvt, SAMPLES_WRITTEN_SIG);
    qf_ctrl::PublishAndProcess(e3, mRecorder);

    auto* e2 = Q_NEW(QEvt, INFERENCE_DONE_SIG);
    qf_ctrl::PublishAndProcess(e2, mRecorder);
    checkRecordedEventSignal(WINDOW_READY_SIG);

    WindowArena* arena = WindowAO_getArena();
    WindowBuffer* window = &arena->windows[0];

    // first window should be free after the inference done sig
    CHECK_EQUAL(WINDOW_STATE_FREE, window->state);
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

//==============================================================================
// | Helpers
//==============================================================================
// WARN: these tests were generated by AI AFTER implementation. Not reviewed yet.
// TODO: review.

//===== findFreeWindow =========================================================

TEST(WindowAOGroup, GivenAllWindowsFree_WhenFindFreeWindow_ThenReturnNextWindow)
{
    WindowArena arena {};
    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        arena.windows[i].state = WINDOW_STATE_FREE;
    }

    WindowBuffer* current = &arena.windows[0];

    WindowBuffer* next = findFreeWindow(&arena, current);

    CHECK_TRUE(next != NULL);
    CHECK_TRUE(next == &arena.windows[1]);
}

TEST(WindowAOGroup, GivenNextWindowNotFree_WhenFindFreeWindow_ThenSkipToNextFreeWindow)
{
    WindowArena arena {};

    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        arena.windows[i].state = WINDOW_STATE_PROCESSING;
    }

    arena.windows[2].state = WINDOW_STATE_FREE;

    WindowBuffer* current = &arena.windows[0];

    WindowBuffer* next = findFreeWindow(&arena, current);

    CHECK_TRUE(next != NULL);
    CHECK_TRUE(next == &arena.windows[2]);
}

TEST(WindowAOGroup, GivenFreeWindowAfterWrapAround_WhenFindFreeWindow_ThenReturnWrappedWindow)
{
    WindowArena arena {};

    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        arena.windows[i].state = WINDOW_STATE_READY;
    }

    arena.windows[0].state = WINDOW_STATE_FREE;

    WindowBuffer* current = &arena.windows[ARENA_WINDOW_COUNT - 1];

    WindowBuffer* next = findFreeWindow(&arena, current);

    CHECK_TRUE(next != NULL);
    CHECK_TRUE(next == &arena.windows[0]);
}

TEST(WindowAOGroup, GivenCurrentWindowIsOnlyFreeWindow_WhenFindFreeWindow_ThenReturnNull)
{
    WindowArena arena {};

    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        arena.windows[i].state = WINDOW_STATE_READY;
    }

    arena.windows[0].state = WINDOW_STATE_FREE;

    WindowBuffer* current = &arena.windows[0];

    WindowBuffer* next = findFreeWindow(&arena, current);

    CHECK_TRUE(next == NULL);
}

TEST(WindowAOGroup, GivenNoFreeWindows_WhenFindFreeWindow_ThenReturnNull)
{
    WindowArena arena {};

    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        arena.windows[i].state = WINDOW_STATE_READY;
    }

    WindowBuffer* current = &arena.windows[0];

    WindowBuffer* next = findFreeWindow(&arena, current);

    CHECK_TRUE(next == NULL);
}

TEST(WindowAOGroup, GivenMultipleFreeWindows_WhenFindFreeWindow_ThenReturnClosestWindow)
{
    WindowArena arena {};

    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        arena.windows[i].state = WINDOW_STATE_PROCESSING;
    }

    arena.windows[2].state = WINDOW_STATE_FREE;
    arena.windows[3].state = WINDOW_STATE_FREE;

    WindowBuffer* current = &arena.windows[0];

    WindowBuffer* next = findFreeWindow(&arena, current);

    CHECK_TRUE(next != NULL);
    CHECK_TRUE(next == &arena.windows[2]);
}

TEST(WindowAOGroup, GivenCurrentAtMiddle_WhenFindFreeWindow_ThenSearchStartsAfterCurrent)
{
    WindowArena arena {};

    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        arena.windows[i].state = WINDOW_STATE_READY;
    }

    arena.windows[0].state = WINDOW_STATE_FREE;

    WindowBuffer* current = &arena.windows[2];

    WindowBuffer* next = findFreeWindow(&arena, current);

    CHECK_TRUE(next != NULL);
    CHECK_TRUE(next == &arena.windows[0]);
}

//===== findReadyWindow ========================================================

TEST(WindowAOGroup, GivenNextWindowReady_WhenFindReadyWindow_ThenReturnNextWindow)
{
    WindowArena arena {};

    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        arena.windows[i].state = WINDOW_STATE_FREE;
    }

    arena.windows[1].state = WINDOW_STATE_READY;

    WindowBuffer* current = &arena.windows[0];

    WindowBuffer* next = findReadyWindow(&arena, current);

    CHECK_TRUE(next != NULL);
    CHECK_TRUE(next == &arena.windows[1]);
}

TEST(WindowAOGroup, GivenReadyWindowAfterSeveralWindows_WhenFindReadyWindow_ThenSkipNonReadyWindows)
{
    WindowArena arena {};

    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        arena.windows[i].state = WINDOW_STATE_PROCESSING;
    }

    arena.windows[3].state = WINDOW_STATE_READY;

    WindowBuffer* current = &arena.windows[0];

    WindowBuffer* next = findReadyWindow(&arena, current);

    CHECK_TRUE(next != NULL);
    CHECK_TRUE(next == &arena.windows[3]);
}

TEST(WindowAOGroup, GivenReadyWindowAfterWrapAround_WhenFindReadyWindow_ThenReturnWrappedWindow)
{
    WindowArena arena {};

    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        arena.windows[i].state = WINDOW_STATE_FREE;
    }

    arena.windows[0].state = WINDOW_STATE_READY;

    WindowBuffer* current = &arena.windows[ARENA_WINDOW_COUNT - 1];

    WindowBuffer* next = findReadyWindow(&arena, current);

    CHECK_TRUE(next != NULL);
    CHECK_TRUE(next == &arena.windows[0]);
}

TEST(WindowAOGroup, GivenCurrentWindowIsOnlyReadyWindow_WhenFindReadyWindow_ThenReturnNull)
{
    WindowArena arena {};

    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        arena.windows[i].state = WINDOW_STATE_FREE;
    }

    arena.windows[1].state = WINDOW_STATE_READY;

    WindowBuffer* current = &arena.windows[1];

    WindowBuffer* next = findReadyWindow(&arena, current);

    CHECK_TRUE(next == NULL);
}

TEST(WindowAOGroup, GivenNoReadyWindows_WhenFindReadyWindow_ThenReturnNull)
{
    WindowArena arena {};

    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        arena.windows[i].state = WINDOW_STATE_FREE;
    }

    WindowBuffer* current = &arena.windows[0];

    WindowBuffer* next = findReadyWindow(&arena, current);

    CHECK_TRUE(next == NULL);
}

TEST(WindowAOGroup, GivenMultipleReadyWindows_WhenFindReadyWindow_ThenReturnClosestReadyWindow)
{
    WindowArena arena {};

    for (uint16_t i = 0; i < ARENA_WINDOW_COUNT; i++) {
        arena.windows[i].state = WINDOW_STATE_FREE;
    }

    arena.windows[2].state = WINDOW_STATE_READY;
    arena.windows[3].state = WINDOW_STATE_READY;

    WindowBuffer* current = &arena.windows[0];

    WindowBuffer* next = findReadyWindow(&arena, current);

    CHECK_TRUE(next != NULL);
    CHECK_TRUE(next == &arena.windows[2]);
}

//===== getNextHeadIndexOfWindow ===============================================

TEST(WindowAOGroup, GivenWindowStartIndex_WhenGetNextHeadIndexOfWindow_ThenReturnOverlapWritePosition)
{
    uint16_t startIndex = 0;

    uint16_t result = getNextHeadIndexOfWindow(startIndex);

    CHECK_EQUAL(
            WINDOW_SAMPLE_COUNT - STRIDE_SAMPLE_COUNT,
            result
            );
}

TEST(WindowAOGroup, GivenNonZeroStartIndex_WhenGetNextHeadIndexOfWindow_ThenOffsetFromWindowStart)
{
    uint16_t startIndex = 123;

    uint16_t result = getNextHeadIndexOfWindow(startIndex);

    CHECK_EQUAL(
            (startIndex + (WINDOW_SAMPLE_COUNT - STRIDE_SAMPLE_COUNT))
            % ARENA_TOTAL_SAMPLES,
            result
            );
}

TEST(WindowAOGroup, GivenHeadIndexNearArenaEnd_WhenGetNextHeadIndexOfWindow_ThenWrapAroundArena)
{
    uint16_t overlap =
        WINDOW_SAMPLE_COUNT - STRIDE_SAMPLE_COUNT;

    uint16_t startIndex =
        (ARENA_TOTAL_SAMPLES - overlap) + 5;

    uint16_t result =
        getNextHeadIndexOfWindow(startIndex);

    CHECK_EQUAL(5, result);
}

TEST(WindowAOGroup, GivenMultipleSequentialWindows_WhenGetNextHeadIndexOfWindow_ThenProduceConsistentStrideOffsets)
{
    uint16_t start0 = 0;
    uint16_t start1 = STRIDE_SAMPLE_COUNT;
    uint16_t start2 = STRIDE_SAMPLE_COUNT * 2;

    uint16_t head0 = getNextHeadIndexOfWindow(start0);
    uint16_t head1 = getNextHeadIndexOfWindow(start1);
    uint16_t head2 = getNextHeadIndexOfWindow(start2);

    CHECK_EQUAL(
            STRIDE_SAMPLE_COUNT,
            (uint16_t)((head1 - head0 + ARENA_TOTAL_SAMPLES)
                % ARENA_TOTAL_SAMPLES)
            );

    CHECK_EQUAL(
            STRIDE_SAMPLE_COUNT,
            (uint16_t)((head2 - head1 + ARENA_TOTAL_SAMPLES)
                % ARENA_TOTAL_SAMPLES)
            );
}

TEST(WindowAOGroup, GivenWindowStartIndexAtArenaBoundary_WhenGetNextHeadIndexOfWindow_ThenReturnWrappedHeadIndex)
{
    uint16_t startIndex = ARENA_TOTAL_SAMPLES - 1;

    uint16_t result =
        getNextHeadIndexOfWindow(startIndex);

    CHECK_EQUAL(
            (uint16_t)(
                (startIndex +
                 (WINDOW_SAMPLE_COUNT - STRIDE_SAMPLE_COUNT))
                % ARENA_TOTAL_SAMPLES),
            result
            );
}
