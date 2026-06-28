// cpputest-for-qpc
#include "cmsTestPublishedEventRecorder.hpp"
#include "cms_cpputest_qf_ctrl.hpp"
#include "cmsQAssertMockSupport.hpp"

// cpputest
#include "CppUTest/TestHarness.h"

#include <array>

#include "motionInferenceAO.h"
#include "pub_sub_signals.h"

#include "unit_test_utils.hpp"

// Test group
TEST_GROUP(MotionInferenceAOGroup) {

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

        MotionInferenceAO_ctor();
        mUnderTest = g_motionInferenceAO; // this will be out AO under test
        CHECK_TRUE(mUnderTest != nullptr);

        // Clear event queue buffer with nullptr
        underTestEventQueueStorage.fill(nullptr);
    }

    void teardown() final {
        mock().checkExpectations();

        MotionInferenceAO_dtor();

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

        CHECK_TRUE(MotionInferenceAO_isInState(STATE_INACTIVE));
    }

};

using namespace cms::test;

TEST(MotionInferenceAOGroup, WhenConstructed_ThenEntersInactiveState)
{
    startAOUnderTest();
}

//==============================================================================
// | Idle
//==============================================================================


