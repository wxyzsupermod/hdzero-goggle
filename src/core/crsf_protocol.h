/**
 * @file crsf_protocol.h
 * @brief CRSF (Crossfire) protocol definitions for ExpressLRS backpack telemetry
 *
 * This file contains the CRSF frame types, structures, and parsing definitions
 * needed to receive telemetry data via ESP-NOW from an ExpressLRS backpack.
 * Based on the ExpressLRS CRSF protocol specification.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// CRSF Frame constants
#define CRSF_SYNC_BYTE                0xC8
#define CRSF_MAX_PACKET_LEN           64
#define CRSF_PAYLOAD_SIZE_MAX         62
#define CRSF_FRAME_NOT_COUNTED_BYTES  2
#define CRSF_FRAME_SIZE(payload_size) ((payload_size) + 2)
#define CRSF_FRAME_CRC_SIZE           1

// CRSF Frame type indices
#define CRSF_TELEMETRY_SYNC_INDEX   0
#define CRSF_TELEMETRY_LENGTH_INDEX 1
#define CRSF_TELEMETRY_TYPE_INDEX   2

/**
 * CRSF Frame Types
 * These define the type of telemetry data in the frame
 */
typedef enum {
    CRSF_FRAMETYPE_GPS = 0x02,
    CRSF_FRAMETYPE_VARIO = 0x07,
    CRSF_FRAMETYPE_BATTERY_SENSOR = 0x08,
    CRSF_FRAMETYPE_BARO_ALTITUDE = 0x09,
    CRSF_FRAMETYPE_AIRSPEED = 0x0A,
    CRSF_FRAMETYPE_HEARTBEAT = 0x0B,
    CRSF_FRAMETYPE_RPM = 0x0C,
    CRSF_FRAMETYPE_TEMP = 0x0D,
    CRSF_FRAMETYPE_CELLS = 0x0E,
    CRSF_FRAMETYPE_LINK_STATISTICS = 0x14,
    CRSF_FRAMETYPE_RC_CHANNELS_PACKED = 0x16,
    CRSF_FRAMETYPE_ATTITUDE = 0x1E,
    CRSF_FRAMETYPE_FLIGHT_MODE = 0x21,
    // Extended Header Frames
    CRSF_FRAMETYPE_DEVICE_PING = 0x28,
    CRSF_FRAMETYPE_DEVICE_INFO = 0x29,
    CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY = 0x2B,
    CRSF_FRAMETYPE_PARAMETER_READ = 0x2C,
    CRSF_FRAMETYPE_PARAMETER_WRITE = 0x2D,
    CRSF_FRAMETYPE_ELRS_STATUS = 0x2E,
    CRSF_FRAMETYPE_COMMAND = 0x32,
    CRSF_FRAMETYPE_MSP_REQ = 0x7A,
    CRSF_FRAMETYPE_MSP_RESP = 0x7B,
    CRSF_FRAMETYPE_MSP_WRITE = 0x7C,
} crsf_frame_type_e;

/**
 * CRSF Address types
 */
typedef enum {
    CRSF_ADDRESS_BROADCAST = 0x00,
    CRSF_ADDRESS_USB = 0x10,
    CRSF_ADDRESS_BLUETOOTH_WIFI = 0x12,
    CRSF_ADDRESS_TBS_CORE_PNP_PRO = 0x80,
    CRSF_ADDRESS_CURRENT_SENSOR = 0xC0,
    CRSF_ADDRESS_GPS = 0xC2,
    CRSF_ADDRESS_TBS_BLACKBOX = 0xC4,
    CRSF_ADDRESS_FLIGHT_CONTROLLER = 0xC8,
    CRSF_ADDRESS_RACE_TAG = 0xCC,
    CRSF_ADDRESS_RADIO_TRANSMITTER = 0xEA,
    CRSF_ADDRESS_CRSF_RECEIVER = 0xEC,
    CRSF_ADDRESS_CRSF_TRANSMITTER = 0xEE,
    CRSF_ADDRESS_ELRS_LUA = 0xEF
} crsf_addr_e;

/**
 * CRSF Header structure
 */
typedef struct __attribute__((packed)) {
    uint8_t sync_byte;
    uint8_t frame_size; // counts size after this byte: payload_size + 2 (type + crc)
    uint8_t type;
} crsf_header_t;

/**
 * CRSF Extended Header structure (for extended frame types 0x28-0x96)
 */
typedef struct __attribute__((packed)) {
    uint8_t device_addr;
    uint8_t frame_size;
    uint8_t type;
    uint8_t dest_addr;
    uint8_t orig_addr;
} crsf_ext_header_t;

/**
 * GPS Sensor Data (CRSF_FRAMETYPE_GPS = 0x02)
 * Total payload size: 15 bytes
 */
typedef struct __attribute__((packed)) {
    int32_t latitude;     // degree / 10,000,000 (Big-Endian)
    int32_t longitude;    // degree / 10,000,000 (Big-Endian)
    uint16_t groundspeed; // km/h / 10 (Big-Endian)
    uint16_t gps_heading; // degree / 100 (Big-Endian)
    uint16_t altitude;    // meter + 1000m offset (Big-Endian)
    uint8_t satellites_in_use;
} crsf_sensor_gps_t;

/**
 * Battery Sensor Data (CRSF_FRAMETYPE_BATTERY_SENSOR = 0x08)
 */
typedef struct __attribute__((packed)) {
    uint16_t voltage;       // mV * 100 (Big-Endian)
    uint16_t current;       // mA * 100 (Big-Endian)
    uint32_t capacity : 24; // mAh
    uint8_t remaining;      // %
} crsf_sensor_battery_t;

/**
 * Barometer/Vario Data (CRSF_FRAMETYPE_BARO_ALTITUDE = 0x09)
 */
typedef struct __attribute__((packed)) {
    uint16_t altitude;   // Altitude in decimeters + 10000dm, or meters if high bit set (Big-Endian)
    int16_t verticalspd; // Vertical speed in cm/s (Big-Endian)
} crsf_sensor_baro_vario_t;

/**
 * Vario Data (CRSF_FRAMETYPE_VARIO = 0x07)
 */
typedef struct __attribute__((packed)) {
    int16_t verticalspd; // Vertical speed in cm/s (Big-Endian)
} crsf_sensor_vario_t;

/**
 * Attitude Data (CRSF_FRAMETYPE_ATTITUDE = 0x1E)
 */
typedef struct __attribute__((packed)) {
    int16_t pitch; // radians * 10000 (Big-Endian)
    int16_t roll;  // radians * 10000 (Big-Endian)
    int16_t yaw;   // radians * 10000 (Big-Endian)
} crsf_sensor_attitude_t;

/**
 * Link Statistics (CRSF_FRAMETYPE_LINK_STATISTICS = 0x14)
 */
typedef struct __attribute__((packed)) {
    uint8_t uplink_rssi_1;       // dBm * -1
    uint8_t uplink_rssi_2;       // dBm * -1
    uint8_t uplink_link_quality; // %
    int8_t uplink_snr;           // dB
    uint8_t active_antenna;
    uint8_t rf_mode;
    uint8_t uplink_tx_power;       // mW enum
    uint8_t downlink_rssi;         // dBm * -1
    uint8_t downlink_link_quality; // %
    int8_t downlink_snr;           // dB
} crsf_link_statistics_t;

/**
 * Flight Mode (CRSF_FRAMETYPE_FLIGHT_MODE = 0x21)
 */
typedef struct __attribute__((packed)) {
    char flight_mode[16];
} crsf_flight_mode_t;

/**
 * CRC8 DVB-S2 polynomial used by CRSF
 */
uint8_t crsf_crc8_dvb_s2(uint8_t crc, uint8_t data);

/**
 * Calculate CRC for a CRSF frame
 * @param data Pointer to frame data starting at type byte
 * @param len Length of data to CRC (frame_size - 1, excludes CRC byte itself)
 * @return CRC8 value
 */
uint8_t crsf_calc_crc(const uint8_t *data, uint8_t len);

/**
 * Validate a CRSF frame
 * @param frame Pointer to complete CRSF frame starting at sync byte
 * @param len Total length of frame data
 * @return true if frame is valid
 */
bool crsf_validate_frame(const uint8_t *frame, uint8_t len);

/**
 * Convert big-endian 16-bit value to host byte order
 */
static inline uint16_t crsf_be16_to_host(uint16_t val) {
    return ((val >> 8) & 0xFF) | ((val & 0xFF) << 8);
}

/**
 * Convert big-endian 32-bit value to host byte order
 */
static inline uint32_t crsf_be32_to_host(uint32_t val) {
    return ((val >> 24) & 0xFF) |
           ((val >> 8) & 0xFF00) |
           ((val & 0xFF00) << 8) |
           ((val & 0xFF) << 24);
}

/**
 * Convert big-endian signed 32-bit value to host byte order
 */
static inline int32_t crsf_be32_to_host_signed(int32_t val) {
    return (int32_t)crsf_be32_to_host((uint32_t)val);
}

/**
 * Convert big-endian signed 16-bit value to host byte order
 */
static inline int16_t crsf_be16_to_host_signed(int16_t val) {
    return (int16_t)crsf_be16_to_host((uint16_t)val);
}

#ifdef __cplusplus
}
#endif
