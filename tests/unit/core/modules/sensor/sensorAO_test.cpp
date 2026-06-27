///*****************************************************************************
/// @brief Tests for SensorAO Active Object responsible for managing sensor
/// initialization, configuration, data and error handling.
///
/// Responsibilities
/// ----------------
/// The SensorAO Active Object handles the lifecycle of the sensor, including:
/// - Initialization on receiving INITIALIZE_MPU_SIG
/// - Data processing triggered by the MPU_FIFO_FULL event, which publishes
/// SAMPLES_WRITTEN_SIG
/// - Transitioning to error state and publishing error events (e.g.,
/// ERROR_SENSOR_I2C_MASTER) if initialization fails
///
/// States
/// ------
/// - **Uninitialized**: Waits for initialization signals
/// - **Initialized**: Processes data events and publishes data ready signals
/// - **Error**: Handles initialization and operational errors
///
//*****************************************************************************

// cpputest-for-qpc
#include "cmsTestPublishedEventRecorder.hpp"
#include "cms_cpputest_qf_ctrl.hpp"
#include "cmsQAssertMockSupport.hpp"

// cpputest
#include "CppUTest/TestHarness.h"

#include <array>

#include "sensor.h"
#include "sensorAO.h"
#include "pub_sub_signals.h"
#include "Fake_Sensor.h"

#include "unit_test_utils.hpp"

// Test group
TEST_GROUP(SensorAOGroup) {

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
        };

        qf_ctrl::Setup(200, 200, memPools);

        // Create Event Recorder ; a cpputest-for-qpc mechanism for recording events
        mRecorder = PublishedEventRecorder::CreatePublishedEventRecorder(
            qf_ctrl::RECORDER_PRIORITY, // priority of the fake qp runtime
            Q_USER_SIG,
            MAX_PUB_SUB_SIG
        );

        setRecorder(mRecorder);

        SensorAO_ctor(&Fake_Sensor_interface);
        mUnderTest = g_sensorAO; // this will be out AO under test
        CHECK_TRUE(mUnderTest != nullptr);

        Fake_Sensor_ctor();

        // Clear event queue buffer with nullptr
        underTestEventQueueStorage.fill(nullptr);
    }

    void teardown() final {
        mock().checkExpectations();

        Fake_Sensor_dtor();
        SensorAO_dtor();

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

        auto event = mRecorder->getRecordedEvent();
        CHECK_TRUE(event != nullptr); // make sure event is called from AO
        CHECK_EQUAL(MPU_UNINITIALIZED_SIG, event->sig);
    }

    void startAOAndMoveToInitializedState(const SensorConfig& config) {
        using namespace cms::test;

        startAOUnderTest();

        auto* e = Q_NEW(SensorAOInitializeMpuRequestEvent, INITIALIZE_MPU_SIG);
        e->config = config;

        qf_ctrl::PublishAndProcess(&e->super, mRecorder);

        mRecorder->getRecordedEvent(); // consume the MPU_INITIALIZED_SIG event
    }

};

static const SensorConfig validConfig = {
    .sample_rate_hz = 200,
    .enable_dmp = true,
    .calibrate_on_init = true,
    .calib_loops = 6,
    .fifo_size = 1000,
};

static const SensorConfig invalidConfig = {
    .sample_rate_hz = SENSOR_MAX_SAMPLE_RATE + 1,
    .enable_dmp = true,
    .calibrate_on_init = true,
    .calib_loops = SENSOR_MAX_CALIB_LOOPS + 1,
    .fifo_size = 1000,
};

TEST(SensorAOGroup, GivenUninitialized_whenInitialized_thenChangeToUninitializedState) {
    startAOUnderTest();
}

// =============================================================================
// | MPU Initialization
// =============================================================================

// INITIALIZE_MPU_SIG signal changes state to initialized when in uninitialized state
TEST(SensorAOGroup, GivenValidConfig_WhenInitializeMpuCalled_ThenEmitsInitializedEventWithSameConfig) {
    using namespace cms::test;

    startAOUnderTest();

    auto* e = Q_NEW(SensorAOInitializeMpuRequestEvent, INITIALIZE_MPU_SIG);
    e->config = validConfig;

    qf_ctrl::PublishAndProcess(&e->super, mRecorder);

    auto event = checkRecordedEventSignal(MPU_INITIALIZED_SIG);

    // The response event is passed with the parameters that were set before.
    auto responseEvt = reinterpret_cast<const SensorAOMpuInitializedResponseEvent*>(event.get());
    CHECK_EQUAL(200, responseEvt->config.sample_rate_hz);
}

// TODO: Should handle Reconfiguration events

// =============================================================================
// | Domain Logic
// =============================================================================

// Should handle The FIFO buffer filling event and send data ready signal for further processing
TEST(SensorAOGroup, GivenInitialized_WhenDataReady_ThenPublishesSensorDataEvent) {
    using namespace cms::test;
    startAOAndMoveToInitializedState(validConfig);

    auto* e = Q_NEW(QEvt, MPU_FIFO_FULL);
    qf_ctrl::PublishAndProcess(e, mRecorder);

    auto recordedEvent = checkRecordedEventSignal(MPU_DATA_READY_SIG);
    auto responseEvt = reinterpret_cast<const MpuBatchEvent*>(recordedEvent.get());

    CHECK_EQUAL(BATCH_SAMPLE_COUNT, responseEvt->batch.count);
}

// TODO: Edge cases for FIFO

// =============================================================================
// | Error Handling
// =============================================================================

// TODO: FIFO size should NOT be set to a value higher than
// SensorAO is moved to error state on any errors during initialization inside Uninitialized state
TEST(SensorAOGroup, GivenInitializing_WhenSensorInitFails_ThenPublishesErrorEvent) {
    using namespace cms::test;

    Fake_Sensor_SetInitResult(ERR_I2C);

    startAOUnderTest();

    auto* e = Q_NEW(SensorAOInitializeMpuRequestEvent, INITIALIZE_MPU_SIG);
    e->config = validConfig;

    qf_ctrl::PublishAndProcess(&e->super, mRecorder);

    checkRecordedEventSignal(ERROR_SENSOR_I2C_MASTER);
}

// TODO: Timeout functionality if sensor doesnt complete operation in time.
// TODO: SensorAO is moved to error state on any errors when inside initialized state
// SensorAO is more can handle i2c errors
// TODO: Fix/Recovery from Error State

// ==== Assertions =============================================================

// Invalid parameters in INITIALIZE_MPU_SIG are handled when appropriately
TEST(SensorAOGroup, GivenInvalidConfig_WhenInitializeMpuCalled_ThenThrowsAssertion) {
    using namespace cms::test;

    // NOTE: assertions cause memory leak detections, this will ignore those.
    qf_ctrl::ChangeMemPoolTeardownOption(qf_ctrl::MemPoolTeardownOption::IGNORE);

    MockExpectQAssert();

    startAOAndMoveToInitializedState(invalidConfig);
}

// TODO: System should not be able to read/request sensor data if SensorAO is in uninitialized state
