#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h> // for size_t
#include <stdbool.h>

typedef struct {
    const char* device_name; // null-terminated string owned by caller
    int mtu;
} BluetoothConfig;

typedef struct {
    bool (*init)(BluetoothConfig config);
    bool (*setup_profile)(void);
    bool (*start_advertising)(void);
    bool (*stop_advertising)(void);
    bool (*is_advertising)(void);
    bool (*set_preferred_mtu)(uint16_t mtu);
    bool (*set_value)(uint32_t param);
    bool (*notify)(uint32_t param);
    void (*get_address)(char* out_str, size_t max_len);
    void (*run_callback_test)(void);
} BluetoothInterface;

#ifdef __cplusplus
}
#endif

#endif
