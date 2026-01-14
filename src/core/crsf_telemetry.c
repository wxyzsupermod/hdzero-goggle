/**
 * @file crsf_telemetry.c
 * @brief CRSF Telemetry parser implementation for ExpressLRS backpack
 *
 * Parses CRSF telemetry frames received via ESP-NOW and extracts GPS,
 * battery, attitude, and other data for the antenna tracker feature.
 */

#include "crsf_telemetry.h"

#include <math.h>
#include <pthread.h>
#include <string.h>

#include <log/log.h>

#include "core/ht.h"
#include "core/settings.h"

// Thread safety mutex
static pthread_mutex_t telemetry_mutex = PTHREAD_MUTEX_INITIALIZER;

// Telemetry state
static crsf_telemetry_state_t telemetry_state;
static telemetry_source_t preferred_source = TELEMETRY_SOURCE_BACKPACK_CRSF;

// CRC8 DVB-S2 lookup table
static const uint8_t crc8_dvb_s2_table[256] = {
    0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54,
    0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
    0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06,
    0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
    0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0,
    0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
    0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2,
    0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
    0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9,
    0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
    0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B,
    0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
    0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D,
    0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
    0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F,
    0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
    0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB,
    0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
    0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9,
    0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
    0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F,
    0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
    0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D,
    0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
    0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26,
    0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
    0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74,
    0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
    0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82,
    0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
    0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0,
    0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9};

uint8_t crsf_crc8_dvb_s2(uint8_t crc, uint8_t data) {
    return crc8_dvb_s2_table[crc ^ data];
}

uint8_t crsf_calc_crc(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc = crsf_crc8_dvb_s2(crc, data[i]);
    }
    return crc;
}

bool crsf_validate_frame(const uint8_t *frame, uint8_t len) {
    if (len < 4) {
        return false;
    }

    // Check sync byte
    if (frame[CRSF_TELEMETRY_SYNC_INDEX] != CRSF_SYNC_BYTE &&
        frame[CRSF_TELEMETRY_SYNC_INDEX] != CRSF_ADDRESS_FLIGHT_CONTROLLER &&
        frame[CRSF_TELEMETRY_SYNC_INDEX] != CRSF_ADDRESS_RADIO_TRANSMITTER) {
        return false;
    }

    // Get frame size
    uint8_t frame_size = frame[CRSF_TELEMETRY_LENGTH_INDEX];

    // Validate frame size
    if (frame_size < 2 || frame_size > CRSF_PAYLOAD_SIZE_MAX + 2) {
        return false;
    }

    // Check if we have enough data
    if (len < (uint8_t)(frame_size + CRSF_FRAME_NOT_COUNTED_BYTES)) {
        return false;
    }

    // Calculate CRC (from type byte to end of payload, excluding CRC byte)
    uint8_t calc_crc = crsf_calc_crc(&frame[CRSF_TELEMETRY_TYPE_INDEX], frame_size - 1);
    uint8_t frame_crc = frame[frame_size + 1];

    return calc_crc == frame_crc;
}

static void process_gps_frame(const uint8_t *payload) {
    const crsf_sensor_gps_t *gps = (const crsf_sensor_gps_t *)payload;

    pthread_mutex_lock(&telemetry_mutex);

    // Convert from CRSF format (big-endian, scaled values) to standard units
    telemetry_state.gps.latitude = (double)crsf_be32_to_host_signed(gps->latitude) / 10000000.0;
    telemetry_state.gps.longitude = (double)crsf_be32_to_host_signed(gps->longitude) / 10000000.0;
    telemetry_state.gps.groundspeed = (float)crsf_be16_to_host(gps->groundspeed) / 10.0f;
    telemetry_state.gps.heading = (float)crsf_be16_to_host(gps->gps_heading) / 100.0f;
    telemetry_state.gps.altitude = (float)crsf_be16_to_host(gps->altitude) - 1000.0f; // Remove 1000m offset
    telemetry_state.gps.satellites = gps->satellites_in_use;
    telemetry_state.gps.valid = (gps->satellites_in_use >= 3);
    telemetry_state.gps.last_update = time(NULL);
    telemetry_state.gps.source = TELEMETRY_SOURCE_BACKPACK_CRSF;

    pthread_mutex_unlock(&telemetry_mutex);

    // Update head tracker GPS data
    if (telemetry_state.gps.valid) {
        ht_antenna_tracker_update_gps(
            telemetry_state.gps.latitude,
            telemetry_state.gps.longitude,
            telemetry_state.gps.altitude,
            telemetry_state.gps.valid);
    }

    LOGD("CRSF GPS: lat=%.6f lon=%.6f alt=%.1fm sat=%d",
         telemetry_state.gps.latitude,
         telemetry_state.gps.longitude,
         telemetry_state.gps.altitude,
         telemetry_state.gps.satellites);
}

static void process_battery_frame(const uint8_t *payload) {
    const crsf_sensor_battery_t *batt = (const crsf_sensor_battery_t *)payload;

    pthread_mutex_lock(&telemetry_mutex);

    telemetry_state.battery.voltage = (float)crsf_be16_to_host(batt->voltage) / 10.0f;
    telemetry_state.battery.current = (float)crsf_be16_to_host(batt->current) / 10.0f;
    // Extract 24-bit capacity (already in little-endian order in struct)
    telemetry_state.battery.capacity_used = batt->capacity;
    telemetry_state.battery.remaining = batt->remaining;
    telemetry_state.battery.valid = true;
    telemetry_state.battery.last_update = time(NULL);

    pthread_mutex_unlock(&telemetry_mutex);

    LOGD("CRSF Battery: %.2fV %.2fA %d%% remaining",
         telemetry_state.battery.voltage,
         telemetry_state.battery.current,
         telemetry_state.battery.remaining);
}

static void process_attitude_frame(const uint8_t *payload) {
    const crsf_sensor_attitude_t *att = (const crsf_sensor_attitude_t *)payload;

    pthread_mutex_lock(&telemetry_mutex);

    // Convert from radians*10000 to degrees
    const float rad_to_deg = 180.0f / 3.14159265f;
    telemetry_state.attitude.pitch = (float)crsf_be16_to_host_signed(att->pitch) / 10000.0f * rad_to_deg;
    telemetry_state.attitude.roll = (float)crsf_be16_to_host_signed(att->roll) / 10000.0f * rad_to_deg;
    telemetry_state.attitude.yaw = (float)crsf_be16_to_host_signed(att->yaw) / 10000.0f * rad_to_deg;
    telemetry_state.attitude.valid = true;
    telemetry_state.attitude.last_update = time(NULL);

    pthread_mutex_unlock(&telemetry_mutex);

    LOGD("CRSF Attitude: pitch=%.1f roll=%.1f yaw=%.1f",
         telemetry_state.attitude.pitch,
         telemetry_state.attitude.roll,
         telemetry_state.attitude.yaw);
}

static void process_link_statistics_frame(const uint8_t *payload) {
    const crsf_link_statistics_t *link = (const crsf_link_statistics_t *)payload;

    pthread_mutex_lock(&telemetry_mutex);

    telemetry_state.link_stats.rssi_1 = -(int8_t)link->uplink_rssi_1;
    telemetry_state.link_stats.rssi_2 = -(int8_t)link->uplink_rssi_2;
    telemetry_state.link_stats.link_quality = link->uplink_link_quality;
    telemetry_state.link_stats.snr = link->uplink_snr;
    telemetry_state.link_stats.rf_mode = link->rf_mode;
    telemetry_state.link_stats.tx_power = link->uplink_tx_power;
    telemetry_state.link_stats.valid = true;
    telemetry_state.link_stats.last_update = time(NULL);

    pthread_mutex_unlock(&telemetry_mutex);

    LOGD("CRSF Link: RSSI1=%d RSSI2=%d LQ=%d%% SNR=%d",
         telemetry_state.link_stats.rssi_1,
         telemetry_state.link_stats.rssi_2,
         telemetry_state.link_stats.link_quality,
         telemetry_state.link_stats.snr);
}

static void process_flight_mode_frame(const uint8_t *payload, uint8_t payload_len) {
    pthread_mutex_lock(&telemetry_mutex);

    // Copy flight mode string (null-terminated)
    uint8_t copy_len = payload_len < 16 ? payload_len : 16;
    memcpy(telemetry_state.flight_mode, payload, copy_len);
    telemetry_state.flight_mode[copy_len] = '\0';

    // Check for armed state in flight mode string
    // Common armed indicators: "ARM", "ARMED", or flight mode without "DISARM"
    telemetry_state.armed = (strstr(telemetry_state.flight_mode, "ARM") != NULL &&
                             strstr(telemetry_state.flight_mode, "DISARM") == NULL);

    pthread_mutex_unlock(&telemetry_mutex);

    LOGD("CRSF Flight Mode: %s (armed=%d)",
         telemetry_state.flight_mode,
         telemetry_state.armed);
}

void crsf_telemetry_init(void) {
    pthread_mutex_lock(&telemetry_mutex);
    memset(&telemetry_state, 0, sizeof(telemetry_state));
    preferred_source = TELEMETRY_SOURCE_BACKPACK_CRSF;
    pthread_mutex_unlock(&telemetry_mutex);

    LOGI("CRSF Telemetry parser initialized");
}

bool crsf_telemetry_process_frame(const uint8_t *frame, uint8_t len) {
    LOGD("crsf_telemetry_process_frame: len=%d, sync=0x%02X len_byte=%d", len, frame[0], frame[1]);
    
    if (!crsf_validate_frame(frame, len)) {
        LOGW("CRSF: Invalid frame received - validation failed");
        return false;
    }

    LOGD("CRSF: Frame validation passed");
    uint8_t frame_type = frame[CRSF_TELEMETRY_TYPE_INDEX];
    uint8_t frame_size = frame[CRSF_TELEMETRY_LENGTH_INDEX];
    const uint8_t *payload = &frame[3];   // Payload starts after sync, length, type
    uint8_t payload_len = frame_size - 2; // Subtract type and CRC bytes
    
    LOGD("CRSF: Type=0x%02X, Size=%d, PayloadLen=%d", frame_type, frame_size, payload_len);

    switch (frame_type) {
    case CRSF_FRAMETYPE_GPS:
        if (payload_len >= sizeof(crsf_sensor_gps_t)) {
            process_gps_frame(payload);
        }
        break;

    case CRSF_FRAMETYPE_BATTERY_SENSOR:
        if (payload_len >= sizeof(crsf_sensor_battery_t)) {
            process_battery_frame(payload);
        }
        break;

    case CRSF_FRAMETYPE_ATTITUDE:
        if (payload_len >= sizeof(crsf_sensor_attitude_t)) {
            process_attitude_frame(payload);
        }
        break;

    case CRSF_FRAMETYPE_LINK_STATISTICS:
        if (payload_len >= sizeof(crsf_link_statistics_t)) {
            process_link_statistics_frame(payload);
        }
        break;

    case CRSF_FRAMETYPE_FLIGHT_MODE:
        process_flight_mode_frame(payload, payload_len);
        break;

    default:
        // Ignore unsupported frame types
        LOGD("CRSF: Ignoring frame type 0x%02X", frame_type);
        return false;
    }

    return true;
}

const crsf_gps_data_t *crsf_telemetry_get_gps(void) {
    return &telemetry_state.gps;
}

const crsf_battery_data_t *crsf_telemetry_get_battery(void) {
    return &telemetry_state.battery;
}

const crsf_attitude_data_t *crsf_telemetry_get_attitude(void) {
    return &telemetry_state.attitude;
}

const crsf_link_stats_t *crsf_telemetry_get_link_stats(void) {
    return &telemetry_state.link_stats;
}

const crsf_telemetry_state_t *crsf_telemetry_get_state(void) {
    return &telemetry_state;
}

bool crsf_telemetry_is_gps_valid(uint32_t max_age_sec) {
    pthread_mutex_lock(&telemetry_mutex);
    bool valid = telemetry_state.gps.valid &&
                 (time(NULL) - telemetry_state.gps.last_update) < max_age_sec;
    pthread_mutex_unlock(&telemetry_mutex);
    return valid;
}

bool crsf_telemetry_is_armed(void) {
    pthread_mutex_lock(&telemetry_mutex);
    bool armed = telemetry_state.armed;
    pthread_mutex_unlock(&telemetry_mutex);
    return armed;
}

void crsf_telemetry_reset(void) {
    pthread_mutex_lock(&telemetry_mutex);
    memset(&telemetry_state, 0, sizeof(telemetry_state));
    pthread_mutex_unlock(&telemetry_mutex);
    LOGI("CRSF Telemetry state reset");
}

telemetry_source_t crsf_telemetry_get_source(void) {
    return telemetry_state.gps.source;
}

void crsf_telemetry_set_preferred_source(telemetry_source_t source) {
    preferred_source = source;
}
