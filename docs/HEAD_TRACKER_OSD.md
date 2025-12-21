# Head Tracker OSD Elements

## Overview
This implementation adds two new OSD elements to the HDZero Goggle firmware for head position tracking visualization. These elements will eventually be integrated with head tracking hardware to display real-time head position for antenna tracking systems.

## New OSD Elements

### 1. Horizontal Compass (Head Tracker Compass)
- **Location**: Top of screen (configurable)
- **Purpose**: Displays azimuth/heading information (0-359 degrees)
- **Visual Design**: 
  - Horizontal scrolling compass scale
  - Tick marks every 10 degrees
  - Major tick marks with degree labels every 30 degrees
  - Red center indicator showing current heading
  - Covers ±90 degrees of view centered on current heading

### 2. Vertical Altitude/Pitch Indicator (Head Tracker Altitude)
- **Location**: Right side of screen (configurable)
- **Purpose**: Displays pitch/elevation information (-90 to +90 degrees)
- **Visual Design**:
  - Vertical scrolling scale
  - Tick marks every 5 degrees
  - Major tick marks with degree labels every 15 degrees
  - Red horizontal line showing current pitch
  - Covers ±45 degrees of view centered on current pitch

## Implementation Details

### Files Modified

1. **src/core/settings.h**
   - Added `OSD_GOGGLE_HEAD_TRACKER_COMPASS` enum
   - Added `OSD_GOGGLE_HEAD_TRACKER_ALTITUDE` enum

2. **src/core/settings.c**
   - Added default positions for both elements
   - Added settings load/save for both elements

3. **src/core/osd.h**
   - Added canvas objects to `osd_hdzero_t` struct
   - Added function declarations for drawing functions

4. **src/core/osd.c**
   - Implemented `osd_head_tracker_compass_draw(int16_t heading_deg)`
   - Implemented `osd_head_tracker_altitude_draw(int16_t pitch_deg)`
   - Added initialization in `embedded_osd_init()`
   - Added position updates in `osd_update_element_positions()`
   - Added demo animation in `osd_hdzero_update()`

5. **src/ui/ui_osd_element_pos.c**
   - Added "Head Tracker Compass" to OSD element list
   - Added "Head Tracker Altitude" to OSD element list

## Default Positions

- **Compass**: X=490, Y=10 (centered near top)
- **Altitude**: X=1230, Y=210 (right side, vertically centered)

Both elements are **disabled by default** and must be enabled through the OSD settings menu.

## Configuration

Users can configure these elements through the OSD Element Position settings:
1. Navigate to OSD settings
2. Select "Head Tracker Compass" or "Head Tracker Altitude"
3. Toggle show/hide
4. Adjust X/Y position as desired

## Current Implementation Status

✅ OSD element structure defined  
✅ Drawing functions implemented  
✅ UI configuration added  
✅ Settings persistence implemented  
✅ Demo animation added (for testing)  

⏳ **To be implemented**:
- Integration with actual head tracker hardware
- Replace demo animation with real sensor data
- Add head tracker configuration page
- Implement calibration routines

## Integration with Head Tracker Hardware

To integrate with actual head tracking hardware, replace the demo code in `osd_hdzero_update()`:

```c
// Current demo code (to be replaced):
static int demo_heading = 0;
static int demo_pitch = 0;
static int demo_counter = 0;

if (demo_counter++ % 5 == 0) {
    demo_heading = (demo_heading + 2) % 360;
    demo_pitch = (int)(15.0 * sin(demo_counter * 0.05));
}

osd_head_tracker_compass_draw(demo_heading);
osd_head_tracker_altitude_draw(demo_pitch);
```

Replace with actual head tracker data:
```c
// Example integration with head tracker:
int16_t heading = head_tracker_get_heading();  // 0-359 degrees
int16_t pitch = head_tracker_get_pitch();      // -90 to +90 degrees

osd_head_tracker_compass_draw(heading);
osd_head_tracker_altitude_draw(pitch);
```

## Technical Notes

### Canvas Rendering
- Both elements use LVGL canvas objects for custom drawing
- Compass: 300x40 pixels (720p) or 450x60 pixels (1080p)
- Altitude: 40x200 pixels (720p) or 60x300 pixels (1080p)
- Elements are hidden by default (LV_OBJ_FLAG_HIDDEN)
- Only visible when enabled in settings

### Performance Considerations
- Drawing is performed every frame in `osd_hdzero_update()`
- Canvas buffers are static to avoid memory allocation overhead
- Line drawing uses LVGL's optimized draw functions
- Consider adding update rate limiting if needed for performance

### Color Scheme
- White (0xFFFFFF): Scale lines and text
- Red (0xFF0000): Center indicators
- Transparent background: Allows video to show through

## Testing

To test the OSD elements:
1. Build the firmware
2. Flash to device
3. Enable OSD elements in settings
4. Observe animated demo showing compass rotation and pitch oscillation
5. Use OSD element position editor to adjust placement

## Future Enhancements

Potential improvements:
- Add cardinal direction markers (N, E, S, W) to compass
- Add attitude indicator integration
- Add roll indication
- Add ground station position markers
- Add visual warning when head position is out of bounds
- Add smoothing/filtering for head tracker data
- Add compass rose visualization option
