# Antenna Tracker Output Mode Implementation

## Overview
The HDZero goggles can output CPPM signals for controlling a ground-based antenna tracker through the head tracker 3.5mm jack. This mode uses an external GPS module connected to the goggles to determine tracker position, and receives drone GPS via OSD telemetry. A simple one-step calibration captures the initial heading when pointing at the drone on takeoff.

## Hardware Setup

### GPS Module Connection
Connect a **u-blox GPS module** to **UART0 (ttyS0)** on the goggles:
- **Protocol**: UBX binary (u-blox proprietary)
- **Baud rate**: 9600 (u-blox default)
- **Format**: 8N1 (8 data bits, no parity, 1 stop bit)
- **Supported messages**: UBX-NAV-POSLLH, UBX-NAV-STATUS, UBX-NAV-SOL, UBX-NAV-VELNED
- **Recommended modules**: 
  - NEO-6M / NEO-7M / NEO-8M / NEO-M8N (u-blox)
  - Any u-blox GPS module with default UBX output

**Wiring**:
- GPS TX → Goggle UART0 RX (ttyS0)
- GPS RX → Not connected (one-way communication)
- GPS VCC → 3.3V or 5V (depending on module)
- GPS GND → GND

### Goggles Output
- **Port**: 3.5mm mono jack on goggles (labeled "HEAD TRACKER")
- **Signal**: CPPM (Compound PPM) - single multiplexed pulse train
- **Format**: 50Hz (20ms frame), 2ms sync pulse, 3 channels (pan/tilt/roll)
- **Channel Ranges**: 1000-2000µs per channel

### CPPM Decoder (RP2040)
The CPPM signal from the goggles must be decoded to individual PWM outputs for servos. Use a Raspberry Pi Pico (RP2040) with the following Arduino code:

```cpp
// CPPM input on GPIO 2, PWM outputs on GPIO 3, 4, 5
const int CPPM_PIN = 2;
const int PWM_PINS[] = {3, 4, 5};
const int NUM_CHANNELS = 3;

volatile unsigned long pulseStart = 0;
volatile unsigned long pulseWidths[NUM_CHANNELS] = {1500, 1500, 1500};
volatile int currentChannel = -1;

void setup() {
  pinMode(CPPM_PIN, INPUT);
  for (int i = 0; i < NUM_CHANNELS; i++) {
    pinMode(PWM_PINS[i], OUTPUT);
  }
  attachInterrupt(digitalPinToInterrupt(CPPM_PIN), cppmISR, CHANGE);
}

void cppmISR() {
  unsigned long now = micros();
  if (digitalRead(CPPM_PIN) == HIGH) {
    pulseStart = now;
  } else {
    unsigned long width = now - pulseStart;
    if (width > 2500) {
      currentChannel = 0;  // Sync pulse detected
    } else if (currentChannel >= 0 && currentChannel < NUM_CHANNELS) {
      pulseWidths[currentChannel] = constrain(width, 1000, 2000);
      currentChannel++;
    }
  }
}

void loop() {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    digitalWrite(PWM_PINS[i], HIGH);
    delayMicroseconds(pulseWidths[i]);
    digitalWrite(PWM_PINS[i], LOW);
    delayMicroseconds(20000 / NUM_CHANNELS - pulseWidths[i]);
  }
}
```

### Servo Connections
Connect servosexternal GPS module on UART0
   - Requires drone GPS from OSD telemetry
   - Requires one-time calibration (point at drone on takeoff)
   - Pan = azimuth to drone (relative to calibrated headingh, -180° to +180°)
- **GPIO 4 (Channel 1)**: Tilt servo (elevation, 0° to 90°)
- **GPIO 5 (Channel 2)**: Roll servo (centered in antenna mode)

## Software Configuration

### Output Mode Selection
The goggles now have two output modes selectable in the Head Tracker menu:

1. **Head Track Mode** (default): Outputs IMU-based head orientation angles
   - Uses BMI270 accelerometer/gyroscope
   - Pan/Tilt/Roll reflect actual head position
   - No GPS required

2. **Ant Track Mode**: Outputs GPS-calculated antenna tracker angles
   - Requires GPS calibration (2-arm process)
   - Pan = azimuth to drone (-180° to +180°)
   - Tilt = elevation to drone (0° to 90°)
   - Roll = centered (1500µs)

### GPS Calibration (One-Time Setup)
The antenna tracker needs to know where the tracker is located and which direction is "forward". This is accomplished with a simple one-step calibration:

**Calibration Procedure**:
1. Connect GPS module to goggles UART0 - wait for GPS fix (status will show satellite count)
2. Place goggles/tracker at your operating position
3. Ensure drone is powered on with GPS fix (OSD shows GPS coordinates)
4. Point goggles directly at the drone's takeoff position
5. Select **"Calibrate"** in the Head Tracker menu
6. GPS status will change to **"GPS: Calibrated"**

**What happens during calibration**:
- Goggle GPS position is recorded (from external GPS module)
- Drone GPS position is recorded (from OSD telemetry)
- Current head pan/tilt angles are stored as the heading reference
- The direction you're looking becomes the reference heading (0°)

**Important**: The goggles must be pointed at the drone during calibration. This establishes the heading reference without needing a magnetometer/compass.

### Menu Navigation
1. Go to **Settings > Head Tracker**
2. Set **Tracking** to **On**
3. Set **Output Mode** to **Ant Track**
4. Wait for GPS fix (status shows "GPS: Ready (X sats)" when both goggles and drone have GPS)
5. Point goggles at drone and select **Calibrate**
6. Connect RP2040 decoder to 3.5mm jack
7. Power on servos and verify tracking

## Code Changes

### New Files

#### `/src/driver/gps.h` and `/src/driver/gps.c`
Complete u-blox UBX binary protocol parser for UART0:
- Supports UBX-NAV-POSLLH (position), UBX-NAV-STATUS (fix status), UBX-NAV-SOL (solution), UBX-NAV-VELNED (velocity)
- Binary message parsing with checksum verification
- Extracts latitude, longitude, altitude (MSL), speed, heading, satellites, fix type
- Provides `gps_data_t` structure with current GPS state
- Functions:
  - `gps_init()`: Opens UART0 at 9600 baud for UBX protocol
  - `gps_update()`: Call periodically to parse incoming UBX messages
  - `gps_get_data()`: Returns current GPS position
  - `gps_has_fix()`: Check if valid GPS fix available

### Files Modified

#### 1. `/src/core/ht.c`
- Added `#include "driver/gps.h"`
- Modified `ht_init()`: Initializes GPS module on UART0
- Modified `timer_callback_imu()`: Calls `gps_update()` periodically
- Modified `ht_antenna_tracker_calibrate()`:
  - Reads local GPS position from GPS module
  - Reads drone GPS position from OSD telemetry
  - Stores pan/tilt angles as heading reference
  - Logs calibration details

#### 2. `/src/ui/page_headtracker.c`
- Added `#include "driver/gps.h"`
- Modified `page_headtracker_update_gps_status()`:
  - Shows local GPS satellite count
  - Indicates which GPS sources are ready
  - Shows "Calibrated" when complete

## Signal Flow

### Head Track Mode
```
BMI270 IMU → Quaternion → Euler Angles → calculate_orientation() 
→ htChannels[0..2] → Set_HT_dat() → FPGA registers 0x72-0x77 
→ CPPM generator → 3.5mm jack → RP2040 decoder → 3x PWM → Servos
```

### Ant Track Mode
```
Drone GPS (OSD) → ht_get_drone_azimuth/elevation() → calculate_orientation()
→ htChannels[0..2] → Set_HT_dat() → FPGA registers 0x72-0x77
→ CPPM generator → 3.5mm jack → RP2040 decoder → 3x PWM → Servos
```

## FPGA CPPM Generation
The existing FPGA code generates CPPM from the register values:
- **Registers 0x72-0x73**: Channel 0 (Pan) - 16-bit value
- **Registers 0x74-0x75**: Channel 1 (Tilt) - 16-bit value  
- **Registers 0x76-0x77**: Channel 2 (Roll) - 16-bit value

Frame structure:
1. 2000µs sync pulse (low)
2. Channel 0 pulse (1000-2000µs)
3. Channel 1 pulse (1000-2000µs)
4. Channel 2 pulse (1000-2000µs)
5. Repeat at 50Hz (20ms period)

## Troubleshooting

### No Local GPS Fix
- Check GPS module wiring (TX → UART0 RX)
- Verify GPS module has power (3.3V or 5V)
- Move to open area with clear sky view
- Wait 30-60 seconds for cold start
- Check GPS LED (should blink when searching, solid when locked)

### GPS Shows "Waiting for drone"
- Ensure drone is powered on and armed
- Verify OSD telemetry is enabled in goggles
- Check that drone FC has GPS module connected
- Drone needs GPS fix (check OSD for GPS icon/coordinates)

### Calibration Fails
- Verify both local GPS and drone GPS show valid data
- Ensure you're pointing goggles at drone during calibration
- Try reset calibration and recalibrate
- Check that drone position appears reasonable in logs

### Servos Not Moving
- Check 3.5mm jack connection (tip = signal, sleeve = ground)
- Verify RP2040 decoder is powered and programmed
- Test servo power supply (separate from RP2040 logic power)
GPS Module (UART0) → NMEA parser → Local GPS position
Drone OSD → GPS telemetry → Drone GPS position
→ Calculate azimuth/elevation → calculate_orientation()
→ htChannels[0..2] → Set_HT_dat() → FPGA registers 0x72-0x77
→ CPPM generator → 3.5mm jack → RP2040 decoder → 3x PWM → Servos
```

**Heading Calculation**:
- During calibration: Pan angle when pointing at drone is stored as reference
- During tracking: GPS bearing to drone - calibration reference = servo pan angle
- This compensates for lack of magnetometer/compassecalibrate while pointing more precisely at drone
- VeMagnetometer/Compass Support
Adding a magnetometer would eliminate the need for pointing calibration:
- Absolute heading reference without calibration
- No need to point at drone during setup
- More flexible tracker placement

### Dual GPS Tracking (Drone → Goggles)
For tracking FROM the drone back TO the goggles (landing assistance):
- Use same GPS hardware (already installed)
- Calculate reverse bearing (goggles → drone becomes drone → goggles)
- Helpful for return-to-home or precision landing

### Drone-Mounted Tracking (Future)
For tracking FROM the drone back TO the goggles (landing assistance):
- Requires GPS on goggles + GPS on drone
- Same CPPM output, different angle calculation
- Azimuth = atan2(home_lat - drone_lat, home_lon - drone_lon)
- Need compass/magnetometer for absolute heading reference

## Testing
Build and flash the firmware:
```bash
cd /workspaces/hdzero-goggle
make -C build all
# Flash to goggles via SD card or UART
```

Test procedure:
1. Flash firmware to goggles
2. Program RP2040 with CPPM decoder code
3. Connect servos to RP2040 outputs
4. Power everything on
5. Navigate to Head Tracker menu
6. Enable tracking and set mode to "Ant Track"
7. Calibrate GPS
8. Verify servos point toward drone position

## References
- [Head Tracker OSD Documentation](HEAD_TRACKER_OSD.md)
- [Implementation Summary](IMPLEMENTATION_SUMMARY.md)
- CPPM decoder: Arduino code above
- Servo control: Standard 50Hz PWM (1000-2000µs)
