# OSD Test Page Usage (Emulator Only)

## Overview
The OSD Test page is an emulator-only feature that allows you to test the head tracker OSD elements without video signal or actual hardware.

## Accessing the Test Page

1. **Build and run the emulator**:
   ```bash
   cd /workspaces/hdzero-goggle/build_emu
   ./HDZGOGGLE
   ```

2. **Navigate to the OSD Test page**:
   - Press `W`/`S` to navigate the main menu
   - Find "OSD Test" in the menu
   - Press `D` to enter the page

## Controls

### Basic Navigation
- **W**: Scroll menu up
- **S**: Scroll menu down
- **D**: Select/Click
- **A**: Back/Right button

### Head Tracker Controls (when on OSD Test page)

#### Arrow Keys (Large Adjustments)
- **LEFT Arrow**: Decrease heading by 5°
- **RIGHT Arrow**: Increase heading by 5°
- **UP Arrow**: Increase pitch by 5°
- **DOWN Arrow**: Decrease pitch by 5°

#### IJKL Keys (Fine Adjustments)
- **J**: Decrease heading by 1°
- **L**: Increase heading by 1°
- **I**: Increase pitch by 1°
- **K**: Decrease pitch by 1°

#### Number Keys (Cardinal Directions)
- **1**: North (0°, 0°)
- **2**: Northeast (45°, 0°)
- **3**: East (90°, 0°)
- **4**: Southeast (135°, 0°)
- **5**: South (180°, 0°)
- **6**: Southwest (225°, 0°)
- **7**: West (270°, 0°)
- **8**: Northwest (315°, 0°)

#### Special Keys
- **R**: Reset to 0° heading, 0° pitch
- **D** (Click): Toggle OSD element visibility

## What You'll See

### On-Screen Display
- **Current values** displayed in green text:
  - Heading: 0-359°
  - Pitch: -90° to +90°

- **Instructions** showing all available keyboard controls

### OSD Elements
- **Horizontal Compass** (top of screen):
  - Shows scrolling degree scale
  - Red center indicator shows current heading
  - White tick marks every 10°
  - Labels every 30°

- **Vertical Altitude Indicator** (right side):
  - Shows scrolling degree scale
  - Red horizontal line shows current pitch
  - White tick marks every 5°
  - Labels every 15°

## Testing Scenarios

### Test 1: Pan (Heading)
1. Press **1** to start at North
2. Use **RIGHT arrow** or **L** to rotate clockwise
3. Observe compass scrolling smoothly
4. Watch degree labels update

### Test 2: Tilt (Pitch)
1. Press **R** to reset
2. Use **UP arrow** or **I** to look up
3. Use **DOWN arrow** or **K** to look down
4. Observe altitude indicator scrolling
5. Note pitch limits at ±90°

### Test 3: Combined Movement
1. Press **1** for North
2. Use **RIGHT arrow** to turn East (90°)
3. Use **UP arrow** to look up (+30°)
4. Observe both elements moving independently

### Test 4: Cardinal Directions
1. Press **1** through **8** in sequence
2. Watch compass jump to each cardinal direction
3. Verify correct degree values

## Troubleshooting

### OSD Elements Not Visible
- Press **D** to toggle visibility
- Check that you're on the OSD Test page
- Elements are enabled by default when entering the page

### Keys Not Responding
- Make sure SDL window has focus
- Verify you're on the OSD Test page (not main menu)
- Try pressing **ESC** and re-entering the page

### Display Issues
- OSD elements render at 20 FPS (50ms update interval)
- Some lag is normal in the emulator
- Ensure you're running the emulator build (not hardware build)

## Notes

- OSD elements overlay on a black background (simulating no video signal)
- Elements maintain their configured positions from OSD settings
- This page is **ONLY available in EMULATOR_BUILD** mode
- Exiting the page preserves the last heading/pitch values
- Use this to test positioning before deploying to hardware

## Next Steps

Once you're satisfied with the OSD element design:
1. Adjust positions via OSD Element Position settings
2. Integrate actual head tracker hardware
3. Replace demo code in `osd_hdzero_update()` with real sensor data
4. Test on actual device

## Code Location

- Page implementation: `src/ui/page_osd_test.c`
- Keyboard handler: `src/core/input_device.c`
- Drawing functions: `src/core/osd.c`
