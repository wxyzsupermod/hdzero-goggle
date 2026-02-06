#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// GPS fix types
typedef enum {
    GPS_FIX_NONE = 0,
    GPS_FIX_2D = 1,
    GPS_FIX_3D = 2,
} gps_fix_type_t;

// GPS data structure
typedef struct {
    double latitude;      // Degrees
    double longitude;     // Degrees
    float altitude;       // Meters above sea level
    float speed;          // Speed in m/s
    float heading;        // Heading in degrees (0-360)
    uint8_t satellites;   // Number of satellites
    gps_fix_type_t fix;   // Fix type
    bool valid;           // Data is valid
} gps_data_t;

// Initialize u-blox GPS on UART0 (ttyS0) at 9600 baud
int gps_init();

// Close GPS UART
void gps_close();

// Update GPS data (call periodically, parses UBX binary messages)
void gps_update();

// Get current GPS data
gps_data_t gps_get_data();

// Check if GPS has valid fix
bool gps_has_fix();

#ifdef __cplusplus
}
#endif
