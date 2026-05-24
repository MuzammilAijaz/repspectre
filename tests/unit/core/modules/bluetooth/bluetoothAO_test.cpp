// cpputest-for-qpc
#include "cmsTestPublishedEventRecorder.hpp"
#include "cms_cpputest_qf_ctrl.hpp"
#include "cmsQAssertMockSupport.hpp"

// cpputest
#include "CppUTest/TestHarness.h"

#include "bluetooth.h"
#include "bluetoothAO.h"
#include "pub_sub_signals.h"
#include "Mock_Bluetooth.hpp"

#include "unit_test_utils.hpp"

// Test group
TEST_GROUP(BluetoothAOGroup) {

    QActive* mUnderTest = nullptr; // Active Object under test

    // Storage for the event queue of the AO, holding 10 events max
    std::array<const QEvt*, 10> underTestEventQueueStorage;

    // Records the events
    cms::test::PublishedEventRecorder* mRecorder = nullptr;

    void setup() final {
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

        BluetoothAO_ctor(&Mock_Bluetooth_interface);
        mUnderTest = g_bluetoothAO; // this will be out AO under test
        CHECK_TRUE(mUnderTest != nullptr);

        Mock_Bluetooth_ctor();

        // Clear event queue buffer with nullptr
        underTestEventQueueStorage.fill(nullptr);
    }

    void teardown() final {
        mock().checkExpectations();

        Mock_Bluetooth_dtor();
        BluetoothAO_dtor();

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
    }

    void startAOAndMoveToInitializedState(const BluetoothConfig& config) {
        using namespace cms::test;

        startAOUnderTest();

        // initialize bluetooth with valid config
        mock().expectOneCall("bluetooth_init")
            .withParameter("device_name", config.device_name)
            .withParameter("mtu", config.mtu)
            .andReturnValue(true); // Assuming initialization succeeds
        mock().expectOneCall("bluetooth_setup_profile")
            .andReturnValue(true);
        auto* e = Q_NEW(BluetoothAOInitializeRequestEvent, INITIALIZE_BLUETOOTH_SIG);
        e->config = config;
        qf_ctrl::PublishAndProcess(&e->super, mRecorder);
        checkRecordedEventSignal(BLUETOOTH_INITIALIZED_SIG);
    }

};

BluetoothConfig validConfig = {
    .device_name = "test",
    .mtu = 500,
};

TEST(BluetoothAOGroup, CppUTest_Smoke_test) {
    startAOUnderTest();
}

// =============================================================================
// | Bluetooth Initialization
// =============================================================================

TEST(BluetoothAOGroup, GivenValidBluetoothConfig_WhenInitializeBluetoothCalled_ThenMoveToInitializedState) {
    using namespace cms::test;

    mock().expectOneCall("bluetooth_init")
        .withParameter("device_name", validConfig.device_name)
        .withParameter("mtu", validConfig.mtu)
        .andReturnValue(true); // Assuming initialization succeeds

    mock().expectOneCall("bluetooth_setup_profile")
        .andReturnValue(true);

    startAOUnderTest();

    auto* e = Q_NEW(BluetoothAOInitializeRequestEvent, INITIALIZE_BLUETOOTH_SIG);
    e->config = validConfig;

    qf_ctrl::PublishAndProcess(&e->super, mRecorder);

    auto event = checkRecordedEventSignal(BLUETOOTH_INITIALIZED_SIG);
}

TEST(BluetoothAOGroup, GivenInitFailed_WhenInitializeBluetoothCalled_ThenMoveToErrorState) {
    using namespace cms::test;

    mock().expectOneCall("bluetooth_init")
        .withParameter("device_name", validConfig.device_name)
        .withParameter("mtu", validConfig.mtu)
        .andReturnValue(false);
    // does not call the setup_profile

    startAOUnderTest();

    auto* e = Q_NEW(BluetoothAOInitializeRequestEvent, INITIALIZE_BLUETOOTH_SIG);
    e->config = validConfig;

    qf_ctrl::PublishAndProcess(&e->super, mRecorder);

    checkRecordedEventSignal(ERROR_BLUETOOTH_INIT);
}
// =============================================================================
// | Domain Logic
// =============================================================================

TEST(BluetoothAOGroup, GivenInitialized_WhenAdvertisementStarted_ThenMoveToAdvertisingState) {
    using namespace cms::test;

    startAOAndMoveToInitializedState(validConfig);

    // start advertisement
    mock().expectOneCall("bluetooth_start_advertising")
        .andReturnValue(true);
    auto* e1 = Q_NEW(QEvt, START_ADVERTISEMENT_SIG);
    qf_ctrl::PublishAndProcess(e1, mRecorder);
}

//: bluetooth advertisment fails
TEST(BluetoothAOGroup, GivenInitialized_WhenAdvertisementStartFailed_ThenMoveToErrorState) {
    using namespace cms::test;

    startAOAndMoveToInitializedState(validConfig);

    mock().expectOneCall("bluetooth_start_advertising")
        .andReturnValue(false);
    auto* e1 = Q_NEW(QEvt, START_ADVERTISEMENT_SIG);
    qf_ctrl::PublishAndProcess(e1, mRecorder);
    checkRecordedEventSignal(ERROR_BLUETOOTH_ADV);
}

// TODO: stop advertismeent and move to conencted state on connection establishment with max/min 1 client

// =============================================================================
// | Failure Handling
// =============================================================================

