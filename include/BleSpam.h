#ifndef BLE_SPAM_H
#define BLE_SPAM_H

#include <Arduino.h>
#include <NimBLEDevice.h>

// Forward declaration
struct DeviceType;
struct WatchModel;

/**
 * @class BleSpam
 * @brief A class to perform various BLE spam attacks.
 *
 * This class encapsulates the logic for broadcasting different types of BLE
 * advertisement packets designed to trigger responses on various operating systems.
 * Intended for educational and security research purposes.
 */
class BleSpam {
public:
    /**
     * @enum EBLEPayloadType
     * @brief Defines the types of spam payloads available.
     */
    enum EBLEPayloadType {
        Microsoft,
        SourApple,
        AppleJuice,
        Samsung,
        Google
    };

    /**
     * @brief Construct a new BleSpam object.
     */
    BleSpam();

    /**
     * @brief Destroy the BleSpam object and clean up resources.
     */
    ~BleSpam();
    
    /**
     * @brief Starts a specific BLE spam attack indefinitely.
     * This method is non-blocking and runs the attack in a background task.
     * @param type The type of payload to send.
     * @return true if the attack started successfully, false otherwise.
     */
    bool start(EBLEPayloadType type);

    /**
     * @brief Starts a BLE spam attack that cycles through all payload types indefinitely.
     * This method is non-blocking.
     * @return true if the attack started successfully, false otherwise.
     */
    bool startAll();

    /**
     * @brief Starts a custom BLE spam attack with a specific name indefinitely.
     * This method is non-blocking.
     * @param spamName The custom name to advertise.
     * @return true if the attack started successfully, false otherwise.
     */
    bool startCustom(String spamName);
    
    /**
     * @brief Starts an iBeacon advertisement indefinitely.
     * @param uuid The UUID of the iBeacon.
     * @param major The major value (default: 1).
     * @param minor The minor value (default: 1).
     * @param manufacturerId The manufacturer ID (default: 0x4C00 for Apple).
     * @param txPower The transmission power (default: -59).
     * @return true if the iBeacon started successfully, false otherwise.
     */
    bool startIBeacon(String uuid, uint16_t major = 1, uint16_t minor = 1, uint16_t manufacturerId = 0x4C00, int8_t txPower = -59);


    /**
     * @brief Stops any running BLE spam or iBeacon attack.
     */
    void stop();

    /**
     * @brief Checks if an attack is currently running.
     * @return true if an attack is active, false otherwise.
     */
    bool isRunning();

    /**
     * @brief Broadcasts a specific type of spam packet for a given duration.
     * This is a blocking method.
     * @param type The type of payload to send (e.g., AppleJuice, Microsoft).
     * @param duration_ms The duration in milliseconds to run the spam attack.
     */
    void spam(EBLEPayloadType type, unsigned long duration_ms);

    /**
     * @brief Cycles through and broadcasts all available spam types for a given duration.
     * This is a blocking method.
     * @param duration_ms The total duration in milliseconds to run the attack.
     */
    void spamAll(unsigned long duration_ms);

    /**
     * @brief Broadcasts spam packets with a custom device name for a given duration.
     * This is a blocking method.
     * @param spamName The custom name to advertise.
     * @param duration_ms The duration in milliseconds to run the spam attack.
     */
    void spamCustom(String spamName, unsigned long duration_ms);

private:
    enum AttackMode { NONE, SPAM_TYPE, SPAM_ALL, SPAM_CUSTOM, IBEACON };

    void executeSpam(EBLEPayloadType type);
    void executeCustomSpam(String spamName);
    void executeIBeacon();
    void generateRandomMac(uint8_t* mac);
    const char* generateRandomName();
    BLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType Type);

    // Task management
    static void spamTask(void *param);
    TaskHandle_t _spamTaskHandle;
    volatile bool _isRunning;

    // Parameters for the running task
    volatile AttackMode _currentMode;
    volatile EBLEPayloadType _currentSpamType;
    String _customSpamName;
    
    // Parameters for iBeacon
    String _iBeaconUUID;
    uint16_t _iBeaconMajor;
    uint16_t _iBeaconMinor;
    uint16_t _iBeaconManufacturerId;
    int8_t _iBeaconTxPower;

    BLEAdvertising* pAdvertising;

    static const uint8_t IOS1[];
    static const uint8_t IOS2[];
    static const struct DeviceType android_models[];
    static const struct WatchModel watch_models[];
    static const int android_models_count;
    static const int watch_models_count;
};

// Helper structs
struct DeviceType {
    uint32_t value;
};
struct WatchModel {
    uint8_t value;
};

#endif // BLE_SPAM_H