// cpputest-for-qpc
#include "cmsTestPublishedEventRecorder.hpp"
#include "cms_cpputest_qf_ctrl.hpp"

#include "pub_sub_signals.h"
#include <CppUTest/UtestMacros.h>

/* Returns recorded event for further checking of event signal */
cms::QEvtUniquePtr checkRecordedEventSignal(PubSubSignal signal);
void setRecorder(cms::test::PublishedEventRecorder * recorder);
