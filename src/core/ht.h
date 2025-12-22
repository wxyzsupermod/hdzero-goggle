#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define DEG_TO_RAD 0.017453295199
#define RAD_TO_DEG 57.29577951308

#define CALIBRATION_BCNT   8 // calibartion loop cnt = (1<<CALIBRATION_BCNT)
#define gyroWeightTiltRoll 0.98

#define MOTION_GYRO_THR    3000
#define MOTION_DUR_1MINUTE 60

#include "bmi270/bmi2_defs.h"

// Antenna tracker GPS data from Betaflight OSD
typedef struct {
    double latitude;  // Current GPS latitude (degrees)
    double longitude; // Current GPS longitude (degrees)
    float altitude;   // Current GPS altitude (meters MSL)
    bool valid;       // GPS fix valid
} ht_gps_data_t;

// Antenna tracker calibration data
typedef struct {
    // Drone origin position (home point)
    double origin_latitude;
    double origin_longitude;
    float origin_altitude; // meters MSL

    // Head tracker angles when pointing at origin during calibration
    float pan_offset;  // degrees
    float tilt_offset; // degrees

    // Calibration state
    bool is_calibrated;
} ht_antenna_tracker_cal_t;

typedef struct {
    struct bmi2_sens_data sensor_data;

    // offset
    int32_t acc_offset[3];
    int32_t gyr_offset[3];

    // Final angles for headtracker
    float tiltAngle;
    float tiltAngleHome;

    float rollAngle;
    float rollAngleHome;

    float panAngle;
    float panAngleHome;

    // Servo settings
    int8_t tiltInverse; // -1= inverted
    int8_t rollInverse;
    int8_t panInverse;

    float tiltFactor; // Gain
    float rollFactor;
    float panFactor;

    // PPM setting
    int16_t htChannels[3]; // 0=Pan, 1=tilt, 2=roll

    // internal state
    uint8_t enable;

    // Antenna tracker calibration
    ht_antenna_tracker_cal_t antenna_tracker;

    // Current GPS data from drone
    ht_gps_data_t gps_data;

} ht_data_t;

void ht_init();
void ht_enable();
void ht_disable();
void ht_detect_motion();
void ht_calibrate();
void ht_set_maxangle(int angle);
void ht_set_alarm_angle();
void ht_set_center_position();
int16_t *ht_get_channels();
float ht_get_pan_angle();
float ht_get_tilt_angle();
void head_alarm_init();

// Antenna tracker functions
void ht_antenna_tracker_calibrate();
void ht_antenna_tracker_update_gps(double latitude, double longitude, float altitude, bool valid);
bool ht_antenna_tracker_is_calibrated();
void ht_antenna_tracker_test_calibrate();
float ht_get_drone_azimuth();
float ht_get_drone_elevation();

#ifdef __cplusplus
}
#endif
