///*****************************************************************************
/// System logging / tracing (using QSpy)
///-----------------------------------------------------------------------------
///
/// How Logging works with QSpy:
/// ----------------------------
///  - MCU: Sends compact binary data (level + tag + message).
///  - HOST: host-side decoder (i.e. QSpy running on host) handle formatting
///          into human-readable text.
///
/// Usage:
/// ------
///  1. #include "repspectre_log.h".
///
///  2. Optional: Define a module tag:  static const char *TAG = "my_module";
///
///  3. Register QSpy dictionaries with QS_USR_DICTIONARY(...);
///       use *_DICTIONARY() format if exposing api for module (to
///       register module dictionaries)
///
///  4. Log:  LOGE(TAG, "Failed to read register 0x%02X", reg);
///     -> Sends:  level(u8) | tag(str) | message(str)
///     -> Host formats the rest.
///
///*****************************************************************************

#ifndef REPSPECTRE_LOG_H_
#define REPSPECTRE_LOG_H_

#include "qpc.h"

//===== Record Ids =============================================================

/**
 * Each ID has its own dictionary entry, allowing the host-side QSpy
 * decoder to distinguish and interpret records from different modules.
 *
 * @see qs.h for user record ids
 */
enum RepspectreQsRecordIds {
    RS_QS_HIL_TEST               = QS_USER0 ,

    /* Hal */
    RS_LOG_DRIVER_LOGS           = QS_USER1 ,
    RS_LOG_SENSOR_HAL                       ,

    /* Driver */
    RS_LOG_MPU6050               = QS_USER2 ,
    RS_LOG_BT_ESP_DRV                       ,

    /* Active Objects */
    RS_LOG_AO_WINDOW             = QS_USER3 ,
    RS_LOG_AO_SENSOR                        ,

    RS_QS_LOG                               ,
    RS_LOG_MAX                              ,
};

/* Convenience alias – use this anywhere instead of magic numbers */
#define REPSPECTRE_LOG_RECORD_ID  RS_QS_LOG

//===== Log Levels =============================================================

typedef enum {
    REPSPECTRE_LOG_LEVEL_NONE    = 0,
    REPSPECTRE_LOG_LEVEL_ERROR   = 1,
    REPSPECTRE_LOG_LEVEL_WARN    = 2,
    REPSPECTRE_LOG_LEVEL_INFO    = 3,
    REPSPECTRE_LOG_LEVEL_DEBUG   = 4,
    REPSPECTRE_LOG_LEVEL_VERBOSE = 5,
} RepSpectreLogLevel;

/* Compile-time filter: set per translation unit before including this header */
#ifndef REPSPECTRE_LOG_LEVEL
    #define REPSPECTRE_LOG_LEVEL REPSPECTRE_LOG_LEVEL_INFO
#endif

//===== Logging Macro ==========================================================

#ifdef Q_SPY
    #define REPSPECTRE_LOG(level_, record_id_, msg_) \
        do { \
            if ((int)(level_) <= (int)(REPSPECTRE_LOG_LEVEL)) { \
                QS_BEGIN_ID((record_id_), 0U) \
                    QS_U8(1U, (uint8_t)(level_)); \
                    QS_STR(msg_); \
                QS_END() \
            } \
        } while (0)
#else
    #define REPSPECTRE_LOG(level_, record_id_, msg_) ((void)0)
#endif

//==============================================================================

static inline void RepspectreLog_registerDriverQsRecords(void)
{
#ifdef Q_SPY
    QS_USR_DICTIONARY(RS_LOG_SENSOR_HAL);
    QS_USR_DICTIONARY(RS_LOG_AO_WINDOW);
    QS_USR_DICTIONARY(RS_LOG_AO_SENSOR);
#endif
}

/* Convenience wrappers – message must be a string literal or char[] */

#define RS_LOGE(record_id_, msg_) REPSPECTRE_LOG(REPSPECTRE_LOG_LEVEL_ERROR,   (record_id_), (msg_))
#define RS_LOGW(record_id_, msg_) REPSPECTRE_LOG(REPSPECTRE_LOG_LEVEL_WARN,    (record_id_), (msg_))
#define RS_LOGI(record_id_, msg_) REPSPECTRE_LOG(REPSPECTRE_LOG_LEVEL_INFO,    (record_id_), (msg_))
#define RS_LOGD(record_id_, msg_) REPSPECTRE_LOG(REPSPECTRE_LOG_LEVEL_DEBUG,   (record_id_), (msg_))
#define RS_LOGV(record_id_, msg_) REPSPECTRE_LOG(REPSPECTRE_LOG_LEVEL_VERBOSE, (record_id_), (msg_))

#endif

