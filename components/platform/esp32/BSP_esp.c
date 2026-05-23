#include "BSP_esp.h"

#include "qpc.h"

#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"

#include "qsafe.h"
#include "soc/soc_caps.h" // for SOC_BT_SUPPORTED

#include "i2c_config_esp32.h"
#include "i2c.h"

Q_DEFINE_THIS_MODULE("BSPEsp32")

// for btStarted()
# if defined(ESP_PLATFORM) && defined(CONFIG_ENABLE_ARDUINO_DEPENDS)
#  include "esp32-hal-bt.h"
#  if __has_include("esp32-hal-bt-mem.h")
#   include "esp32-hal-bt-mem.h"
#  endif
# endif

static const char *TAG = "BSP_ESP";

// ==== Override ===============================================================

static void BSP_configureI2cBus_stub(void) {
    setSensorBusDef(&esp32SensorBusDef);
    Q_ASSERT(sensorsBus.def != NULL);
    i2cdrvInit(&sensorsBus);
}

static int BSP_init_adapter(void) {

    /* ONLY REQUIRED WHEN USED WITH ARDUINO FRAMEWORK
     *
     * CONFIG_ENABLE_ARDUINO_DEPENDS is a configuration flag used in ESP-IDF project builds.
     * When set to =y, it enables the Arduino framework to be used as a component within a
     * standard, C-based ESP-IDF project.
     */
#if defined(CONFIG_ENABLE_ARDUINO_DEPENDS) && SOC_BT_SUPPORTED
    // make sure the linker includes esp32-hal-bt.c so Arduino init doesn't release BLE memory.
    // WARN: THIS SHOULD BE DONE BEFORE FLASH INIT!!
    btStarted();
#endif

    /* CRITICAL RESOURCE SETUP: Initialize NVS Flash required for Wi-Fi/BT PHY calibration */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret != ESP_OK) {
        // ESP_LOGE(TAG, "Failed to initialize hardware NVS sector: %s", esp_err_to_name(ret));
        return 0; // Return 0 on failure matching firmware conventions
    }

    // ESP_LOGI(TAG, "Global ESP32 hardware platform initialization complete (NVS initialized).");
    return 1; // Return 1 on success matching your Arduino implementation
}

BspInterface espBspInterface = {
    .BSP_init = BSP_init_adapter,
    .BSP_configureI2cBus = BSP_configureI2cBus_stub,
};
