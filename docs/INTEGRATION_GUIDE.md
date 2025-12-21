# Integration Guide: Head Tracker Hardware

This guide explains how to integrate actual head tracking hardware with the OSD elements.

## Quick Start

Replace the demo code in [src/core/osd.c](../src/core/osd.c) `osd_hdzero_update()` function:

```c
// Remove this demo code:
static int demo_heading = 0;
static int demo_pitch = 0;
static int demo_counter = 0;

if (demo_counter++ % 5 == 0) {
    demo_heading = (demo_heading + 2) % 360;
    demo_pitch = (int)(15.0 * sin(demo_counter * 0.05));
}

// And replace with your head tracker interface:
int16_t heading = head_tracker_get_heading();
int16_t pitch = head_tracker_get_pitch();

osd_head_tracker_compass_draw(heading);
osd_head_tracker_altitude_draw(pitch);
```

## Example: IMU Sensor Integration

### Option 1: BNO055 9-DOF IMU

```c
// In a new file: src/driver/head_tracker.h

#ifndef HEAD_TRACKER_H
#define HEAD_TRACKER_H

#include <stdint.h>
#include <stdbool.h>

// Initialize head tracker hardware
bool head_tracker_init(void);

// Get current heading (0-359 degrees, 0=North)
int16_t head_tracker_get_heading(void);

// Get current pitch (-90 to +90 degrees)
int16_t head_tracker_get_pitch(void);

// Get current roll (-180 to +180 degrees)  
int16_t head_tracker_get_roll(void);

// Calibrate the sensor
void head_tracker_calibrate(void);

#endif
```

```c
// In a new file: src/driver/head_tracker.c

#include "head_tracker.h"
#include "driver/i2c.h"
#include <math.h>

#define BNO055_ADDR 0x28
#define BNO055_EULER_H_LSB 0x1A
#define BNO055_EULER_R_LSB 0x1C
#define BNO055_EULER_P_LSB 0x1E

static bool initialized = false;

bool head_tracker_init(void) {
    // Initialize BNO055 sensor
    // Set to NDOF mode (9-DOF fusion)
    I2C_Write(BNO055_ADDR, 0x3D, 0x00); // Config mode
    usleep(30000);
    I2C_Write(BNO055_ADDR, 0x3F, 0x00); // Use internal oscillator
    I2C_Write(BNO055_ADDR, 0x3E, 0x00); // Reset
    usleep(650000);
    I2C_Write(BNO055_ADDR, 0x3D, 0x0C); // NDOF mode
    usleep(20000);
    
    initialized = true;
    return true;
}

int16_t head_tracker_get_heading(void) {
    if (!initialized) return 0;
    
    uint8_t lsb = I2C_Read(BNO055_ADDR, BNO055_EULER_H_LSB);
    uint8_t msb = I2C_Read(BNO055_ADDR, BNO055_EULER_H_LSB + 1);
    
    int16_t raw = (msb << 8) | lsb;
    int16_t heading = raw / 16; // Convert to degrees
    
    // Ensure 0-359 range
    while (heading < 0) heading += 360;
    while (heading >= 360) heading -= 360;
    
    return heading;
}

int16_t head_tracker_get_pitch(void) {
    if (!initialized) return 0;
    
    uint8_t lsb = I2C_Read(BNO055_ADDR, BNO055_EULER_P_LSB);
    uint8_t msb = I2C_Read(BNO055_ADDR, BNO055_EULER_P_LSB + 1);
    
    int16_t raw = (msb << 8) | lsb;
    int16_t pitch = raw / 16; // Convert to degrees
    
    // Clamp to valid range
    if (pitch < -90) pitch = -90;
    if (pitch > 90) pitch = 90;
    
    return pitch;
}

int16_t head_tracker_get_roll(void) {
    if (!initialized) return 0;
    
    uint8_t lsb = I2C_Read(BNO055_ADDR, BNO055_EULER_R_LSB);
    uint8_t msb = I2C_Read(BNO055_ADDR, BNO055_EULER_R_LSB + 1);
    
    int16_t raw = (msb << 8) | lsb;
    int16_t roll = raw / 16; // Convert to degrees
    
    return roll;
}

void head_tracker_calibrate(void) {
    // Implement calibration routine
    // Could involve reading calibration status registers
    // and storing calibration offsets
}
```

### Option 2: MPU6050/MPU9250 IMU

```c
// Similar structure, but using MPU registers
// MPU6050: 6-DOF (accel + gyro)
// MPU9250: 9-DOF (accel + gyro + mag)

#define MPU_ADDR 0x68
#define MPU_GYRO_XOUT_H 0x43
#define MPU_ACCEL_XOUT_H 0x3B

// You'll need to implement quaternion/euler angle conversion
// from raw accelerometer and gyroscope data
```

## Integration Steps

### 1. Add to Build System

Update `CMakeLists.txt`:
```cmake
# Add head tracker source
set(SOURCES
    # ... existing sources ...
    src/driver/head_tracker.c
)
```

### 2. Initialize in Main

In [src/core/main.c](../src/core/main.c):
```c
#include "driver/head_tracker.h"

int main() {
    // ... existing initialization ...
    
    // Initialize head tracker
    if (!head_tracker_init()) {
        LOGE("Failed to initialize head tracker");
    }
    
    // ... rest of main ...
}
```

### 3. Update OSD Function

In [src/core/osd.c](../src/core/osd.c):
```c
#include "driver/head_tracker.h"

void osd_hdzero_update(void) {
    // ... existing code ...
    
    // Replace demo code with real sensor data:
    int16_t heading = head_tracker_get_heading();
    int16_t pitch = head_tracker_get_pitch();
    
    osd_head_tracker_compass_draw(heading);
    osd_head_tracker_altitude_draw(pitch);
}
```

## Advanced Features

### Smoothing Filter

To reduce jitter:
```c
// Simple moving average filter
#define FILTER_SIZE 5

int16_t filter_heading(int16_t new_value) {
    static int16_t buffer[FILTER_SIZE] = {0};
    static int index = 0;
    static int32_t sum = 0;
    
    sum -= buffer[index];
    buffer[index] = new_value;
    sum += new_value;
    index = (index + 1) % FILTER_SIZE;
    
    return sum / FILTER_SIZE;
}
```

### Mounting Orientation Compensation

If sensor is mounted at an angle:
```c
typedef struct {
    int16_t heading_offset;
    int16_t pitch_offset;
    int16_t roll_offset;
} head_tracker_config_t;

// Apply offsets
int16_t compensated_heading = (raw_heading + config.heading_offset) % 360;
int16_t compensated_pitch = raw_pitch + config.pitch_offset;
```

### Calibration Storage

Store calibration in settings:
```c
// In settings.h
typedef struct {
    bool enabled;
    int16_t heading_offset;
    int16_t pitch_offset;
    int16_t roll_offset;
    bool auto_calibrate;
} setting_head_tracker_t;

// Add to main settings struct
typedef struct {
    // ... existing fields ...
    setting_head_tracker_t head_tracker;
} setting_t;
```

## Antenna Tracking Output

To send head position to antenna tracker:

### Option 1: Serial Output (UART)
```c
void head_tracker_send_serial(int16_t heading, int16_t pitch) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "HEAD:%d,PITCH:%d\n", heading, pitch);
    uart_write(buffer, strlen(buffer));
}
```

### Option 2: MSP Protocol
```c
// Use existing MSP infrastructure
void head_tracker_send_msp(int16_t heading, int16_t pitch) {
    msp_msg_t msg;
    msg.cmd = MSP_HEAD_TRACKER; // Define new MSP command
    msg.payload[0] = heading & 0xFF;
    msg.payload[1] = (heading >> 8) & 0xFF;
    msg.payload[2] = pitch & 0xFF;
    msg.payload[3] = (pitch >> 8) & 0xFF;
    msg.len = 4;
    msp_send(&msg);
}
```

### Option 3: WiFi/Network
```c
// Send over UDP
void head_tracker_send_udp(int16_t heading, int16_t pitch) {
    char json[128];
    snprintf(json, sizeof(json), 
             "{\"heading\":%d,\"pitch\":%d}", 
             heading, pitch);
    udp_send(json, strlen(json));
}
```

## Testing Without Hardware

Use test patterns:
```c
// Simulate head movement patterns
typedef enum {
    TEST_PATTERN_STATIONARY,
    TEST_PATTERN_PAN,
    TEST_PATTERN_TILT,
    TEST_PATTERN_CIRCLE,
    TEST_PATTERN_FIGURE_8
} test_pattern_t;

void head_tracker_test_pattern(test_pattern_t pattern) {
    static int counter = 0;
    counter++;
    
    switch(pattern) {
        case TEST_PATTERN_PAN:
            heading = (counter * 2) % 360;
            pitch = 0;
            break;
        case TEST_PATTERN_TILT:
            heading = 0;
            pitch = 45 * sin(counter * 0.05);
            break;
        // ... etc
    }
}
```

## Performance Considerations

- **Update Rate**: Aim for 20-50 Hz sensor reads
- **I2C Speed**: Use 400kHz fast mode if possible
- **CPU Load**: IMU fusion typically uses <5% CPU
- **Latency**: Keep total latency under 50ms for responsive tracking

## Troubleshooting

Common issues:

1. **Erratic readings**: Check sensor mounting, ensure firm attachment
2. **Drift**: Implement magnetometer calibration
3. **Lag**: Reduce filtering, increase sample rate
4. **Noise**: Add low-pass filter, check I2C wiring
5. **Offset**: Run calibration routine with level reference

## Resources

- [BNO055 Datasheet](https://www.bosch-sensortec.com/products/smart-sensors/bno055/)
- [MPU9250 Datasheet](https://invensense.tdk.com/products/motion-tracking/9-axis/mpu-9250/)
- [Madgwick Filter](https://x-io.co.uk/open-source-imu-and-ahrs-algorithms/)
- [Complementary Filter Tutorial](http://www.pieter-jan.com/node/11)
