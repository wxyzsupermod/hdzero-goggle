/**
 * @file crsf_telemetry.h
 * @brief CRSF Telemetry parser for ExpressLRS backpack telemetry via ESP-NOW
 *
 * This module receives and parses CRSF telemetry frames that are forwarded
 * from an ExpressLRS transmitter via ESP-NOW to the goggle's ESP32 backpack.
 * It extracts GPS and other telemetry data for use by the antenna tracker.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "crsf_protocol.h"

/**
 * Telemetry source enumeration
 * Indicates where GPS data is coming from
 */
typedef enum {
    TELEMETRY_SOURCE_NONE = 0,
    TELEMETRY_SOURCE_OSD,          // Parsed from OSD display
    TELEMETRY_SOURCE_BACKPACK_CRSF // From ELRS backpack via ESP-NOW/CRSF
} telemetry_source_t;

/**
 * GPS telemetry data structure
 * Stores the most recent GPS data from any source
 */
typedef struct {
    double latitude;           // degrees
    double longitude;          // degrees
    float altitude;            // meters above mean sea level
    float groundspeed;         // km/h
    float heading;             // degrees (0-360)
    uint8_t satellites;        // number of satellites in use
    bool valid;                // true if GPS has fix
    time_t last_update;        // timestamp of last update
    telemetry_source_t source; // where this data came from
} crsf_gps_data_t;

/**
 * Battery telemetry data structure
 */
typedef struct {
    float voltage;          // volts
    float current;          // amps
    uint32_t capacity_used; // mAh
    uint8_t remaining;      // percentage
    bool valid;
    time_t last_update;
} crsf_battery_data_t;

/**
 * Attitude telemetry data structure
 */
typedef struct {
    float pitch; // degrees
    float roll;  // degrees
    float yaw;   // degrees
    bool valid;
    time_t last_update;
} crsf_attitude_data_t;

/**
 * Link statistics data structure
 */
typedef struct {
    int8_t rssi_1;        // dBm
    int8_t rssi_2;        // dBm
    uint8_t link_quality; // percentage
    int8_t snr;           // dB
    uint8_t rf_mode;
    uint8_t tx_power; // mW enum
    bool valid;
    time_t last_update;
} crsf_link_stats_t;

/**
 * Complete telemetry state
 */
typedef struct {
    crsf_gps_data_t gps;
    crsf_battery_data_t battery;
    crsf_attitude_data_t attitude;
    crsf_link_stats_t link_stats;
    char flight_mode[17]; // null-terminated flight mode string
    bool armed;           // armed state detected from flight mode
} crsf_telemetry_state_t;

/**
 * Initialize the CRSF telemetry parser
 */
void crsf_telemetry_init(void);

/**
 * Process a raw CRSF frame received from ESP-NOW/backpack
 * @param frame Pointer to complete CRSF frame data
 * @param len Length of frame data
 * @return true if frame was successfully parsed
 */
bool crsf_telemetry_process_frame(const uint8_t *frame, uint8_t len);

/**
 * Get current GPS telemetry data
 * @return Pointer to GPS data structure
 */
const crsf_gps_data_t *crsf_telemetry_get_gps(void);

/**
 * Get current battery telemetry data
 * @return Pointer to battery data structure
 */
const crsf_battery_data_t *crsf_telemetry_get_battery(void);

/**
 * Get current attitude telemetry data
 * @return Pointer to attitude data structure
 */
const crsf_attitude_data_t *crsf_telemetry_get_attitude(void);

/**
 * Get current link statistics
 * @return Pointer to link stats structure
 */
const crsf_link_stats_t *crsf_telemetry_get_link_stats(void);

/**
 * Get complete telemetry state
 * @return Pointer to telemetry state structure
 */
const crsf_telemetry_state_t *crsf_telemetry_get_state(void);

/**
 * Check if CRSF telemetry GPS data is available and recent
 * @param max_age_sec Maximum age in seconds for data to be considered valid
 * @return true if GPS data is valid and recent
 */
bool crsf_telemetry_is_gps_valid(uint32_t max_age_sec);

/**
 * Check if drone is armed based on telemetry
 * @return true if armed
 */
bool crsf_telemetry_is_armed(void);

/**
 * Reset all telemetry data
 */
void crsf_telemetry_reset(void);

/**
 * Get the current telemetry source for GPS
 * @return Current telemetry source
 */
telemetry_source_t crsf_telemetry_get_source(void);

/**
 * Set preferred telemetry source priority
 * @param source Preferred source (BACKPACK_CRSF has higher priority by default)
 */
void crsf_telemetry_set_preferred_source(telemetry_source_t source);

#ifdef __cplusplus
}
#endif
