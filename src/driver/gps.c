#include "gps.h"
#include "uart.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <log/log.h>

#define GPS_UART_PORT 0  // ttyS0
#define UBX_SYNC1 0xB5
#define UBX_SYNC2 0x62
#define UBX_MAX_PAYLOAD 256

// UBX message classes and IDs
#define UBX_CLASS_NAV 0x01
#define UBX_NAV_POSLLH 0x02  // Position (lat/lon/alt)
#define UBX_NAV_STATUS 0x03  // Fix status
#define UBX_NAV_SOL 0x06     // Navigation solution
#define UBX_NAV_VELNED 0x12  // Velocity

// UBX parser state
typedef enum {
    UBX_STATE_IDLE,
    UBX_STATE_SYNC1,
    UBX_STATE_SYNC2,
    UBX_STATE_CLASS,
    UBX_STATE_ID,
    UBX_STATE_LEN1,
    UBX_STATE_LEN2,
    UBX_STATE_PAYLOAD,
    UBX_STATE_CK_A,
    UBX_STATE_CK_B
} ubx_state_t;

static int gps_fd = -1;
static gps_data_t gps_data = {0};

// UBX parser state
static ubx_state_t ubx_state = UBX_STATE_IDLE;
static uint8_t ubx_class = 0;
static uint8_t ubx_id = 0;
static uint16_t ubx_length = 0;
static uint16_t ubx_payload_pos = 0;
static uint8_t ubx_payload[UBX_MAX_PAYLOAD];
static uint8_t ubx_ck_a = 0;
static uint8_t ubx_ck_b = 0;

// Calculate UBX checksum
static void ubx_checksum(uint8_t byte, uint8_t *ck_a, uint8_t *ck_b) {
    *ck_a += byte;
    *ck_b += *ck_a;
}

// Parse UBX-NAV-POSLLH (0x01 0x02)
static void parse_nav_posllh(const uint8_t *payload, uint16_t length) {
    if (length < 28) return;

    // iTOW (4 bytes) - skip
    // lon (4 bytes) - longitude in degrees * 1e7
    int32_t lon = (int32_t)(payload[4] | (payload[5] << 8) | 
                            (payload[6] << 16) | (payload[7] << 24));
    // lat (4 bytes) - latitude in degrees * 1e7
    int32_t lat = (int32_t)(payload[8] | (payload[9] << 8) | 
                            (payload[10] << 16) | (payload[11] << 24));
    // height (4 bytes) - height above ellipsoid in mm
    int32_t height = (int32_t)(payload[12] | (payload[13] << 8) | 
                               (payload[14] << 16) | (payload[15] << 24));
    // hMSL (4 bytes) - height above MSL in mm
    int32_t hmsl = (int32_t)(payload[16] | (payload[17] << 8) | 
                             (payload[18] << 16) | (payload[19] << 24));

    gps_data.latitude = lat / 1e7;
    gps_data.longitude = lon / 1e7;
    gps_data.altitude = hmsl / 1000.0f;  // mm to meters
}

// Parse UBX-NAV-STATUS (0x01 0x03)
static void parse_nav_status(const uint8_t *payload, uint16_t length) {
    if (length < 16) return;

    // iTOW (4 bytes) - skip
    // gpsFix (1 byte) - 0=no fix, 2=2D, 3=3D
    uint8_t gps_fix = payload[4];
    // flags (1 byte)
    uint8_t flags = payload[5];

    if (gps_fix == 0x03) {
        gps_data.fix = GPS_FIX_3D;
        gps_data.valid = true;
    } else if (gps_fix == 0x02) {
        gps_data.fix = GPS_FIX_2D;
        gps_data.valid = true;
    } else {
        gps_data.fix = GPS_FIX_NONE;
        gps_data.valid = false;
    }
}

// Parse UBX-NAV-SOL (0x01 0x06)
static void parse_nav_sol(const uint8_t *payload, uint16_t length) {
    if (length < 52) return;

    // iTOW (4 bytes) - skip
    // fTOW (4 bytes) - skip
    // week (2 bytes) - skip
    // gpsFix (1 byte) at offset 10
    uint8_t gps_fix = payload[10];
    // numSV (1 byte) at offset 47
    uint8_t num_sv = payload[47];

    gps_data.satellites = num_sv;

    if (gps_fix == 0x03) {
        gps_data.fix = GPS_FIX_3D;
        gps_data.valid = true;
    } else if (gps_fix == 0x02) {
        gps_data.fix = GPS_FIX_2D;
        gps_data.valid = true;
    } else {
        gps_data.fix = GPS_FIX_NONE;
        gps_data.valid = false;
    }
}

// Parse UBX-NAV-VELNED (0x01 0x12)
static void parse_nav_velned(const uint8_t *payload, uint16_t length) {
    if (length < 36) return;

    // iTOW (4 bytes) - skip
    // velN (4 bytes) - north velocity cm/s
    int32_t vel_n = (int32_t)(payload[4] | (payload[5] << 8) | 
                              (payload[6] << 16) | (payload[7] << 24));
    // velE (4 bytes) - east velocity cm/s
    int32_t vel_e = (int32_t)(payload[8] | (payload[9] << 8) | 
                              (payload[10] << 16) | (payload[11] << 24));
    // gSpeed (4 bytes) at offset 20 - ground speed cm/s
    int32_t g_speed = (int32_t)(payload[20] | (payload[21] << 8) | 
                                (payload[22] << 16) | (payload[23] << 24));
    // heading (4 bytes) at offset 24 - heading degrees * 1e5
    int32_t heading = (int32_t)(payload[24] | (payload[25] << 8) | 
                                (payload[26] << 16) | (payload[27] << 24));

    gps_data.speed = g_speed / 100.0f;  // cm/s to m/s
    gps_data.heading = heading / 1e5f;
}

// Process complete UBX message
static void process_ubx_message() {
    switch (ubx_class) {
        case UBX_CLASS_NAV:
            switch (ubx_id) {
                case UBX_NAV_POSLLH:
                    parse_nav_posllh(ubx_payload, ubx_length);
                    break;
                case UBX_NAV_STATUS:
                    parse_nav_status(ubx_payload, ubx_length);
                    break;
                case UBX_NAV_SOL:
                    parse_nav_sol(ubx_payload, ubx_length);
                    break;
                case UBX_NAV_VELNED:
                    parse_nav_velned(ubx_payload, ubx_length);
                    break;
            }
            break;
    }
}

// Process incoming GPS data byte-by-byte
static void process_gps_data(uint8_t byte) {
    switch (ubx_state) {
        case UBX_STATE_IDLE:
            if (byte == UBX_SYNC1) {
                ubx_state = UBX_STATE_SYNC1;
            }
            break;

        case UBX_STATE_SYNC1:
            if (byte == UBX_SYNC2) {
                ubx_state = UBX_STATE_CLASS;
                ubx_ck_a = 0;
                ubx_ck_b = 0;
            } else {
                ubx_state = UBX_STATE_IDLE;
            }
            break;

        case UBX_STATE_CLASS:
            ubx_class = byte;
            ubx_checksum(byte, &ubx_ck_a, &ubx_ck_b);
            ubx_state = UBX_STATE_ID;
            break;

        case UBX_STATE_ID:
            ubx_id = byte;
            ubx_checksum(byte, &ubx_ck_a, &ubx_ck_b);
            ubx_state = UBX_STATE_LEN1;
            break;

        case UBX_STATE_LEN1:
            ubx_length = byte;
            ubx_checksum(byte, &ubx_ck_a, &ubx_ck_b);
            ubx_state = UBX_STATE_LEN2;
            break;

        case UBX_STATE_LEN2:
            ubx_length |= (byte << 8);
            ubx_checksum(byte, &ubx_ck_a, &ubx_ck_b);
            ubx_payload_pos = 0;
            if (ubx_length > 0 && ubx_length < UBX_MAX_PAYLOAD) {
                ubx_state = UBX_STATE_PAYLOAD;
            } else if (ubx_length == 0) {
                ubx_state = UBX_STATE_CK_A;
            } else {
                ubx_state = UBX_STATE_IDLE;  // Invalid length
            }
            break;

        case UBX_STATE_PAYLOAD:
            ubx_payload[ubx_payload_pos++] = byte;
            ubx_checksum(byte, &ubx_ck_a, &ubx_ck_b);
            if (ubx_payload_pos >= ubx_length) {
                ubx_state = UBX_STATE_CK_A;
            }
            break;

        case UBX_STATE_CK_A:
            if (byte == ubx_ck_a) {
                ubx_state = UBX_STATE_CK_B;
            } else {
                ubx_state = UBX_STATE_IDLE;  // Checksum error
            }
            break;

        case UBX_STATE_CK_B:
            if (byte == ubx_ck_b) {
                // Valid message - process it
                process_ubx_message();
            }
            ubx_state = UBX_STATE_IDLE;
            break;

        default:
            ubx_state = UBX_STATE_IDLE;
            break;
    }
}

int gps_init() {
    LOGI("Initializing u-blox GPS on UART0 (ttyS0) at 9600 baud");
    
    gps_fd = uart_open(GPS_UART_PORT);
    if (gps_fd < 0) {
        LOGE("Failed to open GPS UART");
        return -1;
    }

    // Configure for 9600 baud, 8N1 (u-blox default)
    if (uart_set_opt(gps_fd, 9600, 8, 'N', 1) < 0) {
        LOGE("Failed to configure GPS UART");
        uart_close(gps_fd);
        gps_fd = -1;
        return -1;
    }

    // Initialize GPS data structure
    memset(&gps_data, 0, sizeof(gps_data));
    ubx_state = UBX_STATE_IDLE;
    ubx_payload_pos = 0;

    LOGI("u-blox GPS initialized successfully");
    return 0;
}

void gps_close() {
    if (gps_fd >= 0) {
        uart_close(gps_fd);
        gps_fd = -1;
    }
}

void gps_update() {
    if (gps_fd < 0) {
        return;
    }

    uint8_t byread = 0;
    
    // Read up to 256 bytes per update cycle (UBX messages can be larger than NMEA)
    while (bytes_read < 256 && uart_read_byte(gps_fd, &byte) == 1) {
        process_gps_data(byte);
        bytes_reada(byte);
        bytes_available++;
    }
}

gps_data_t gps_get_data() {
    return gps_data;
}

bool gps_has_fix() {
    return gps_data.valid && gps_data.fix != GPS_FIX_NONE;
}
