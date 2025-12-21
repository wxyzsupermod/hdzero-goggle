# Visual Reference: Head Tracker OSD Elements

## Horizontal Compass (Top)

```
                    ┌─────────────────────────────────────┐
                    │         Head Tracker Compass        │
                    ├─────────────────────────────────────┤
                    │                                     │
Tick marks:         ├─┬─┬─┬───┬─┬─┬───┬─┬─┬───┬─┬─┬───┬──┤
                    │ │ │ │   │ │ │   │ │ │   │ │ │   │  │
Degrees:           270  280  290  300  310  320  330  340
                                     ▼
                              Center Indicator (Red)
                           (Shows current heading)
```

**Features:**
- Horizontal scrolling scale
- Small ticks every 10°
- Large ticks with labels every 30°
- Red downward triangle at center
- Currently visible range: ±90° from center
- Background: Transparent

**Example Headings:**
- 0° = North
- 90° = East
- 180° = South  
- 270° = West

---

## Vertical Altitude/Pitch Indicator (Right Side)

```
                          ┌────┬───┐
                          │    │+45│
                          ├────┼───┤
                          │    │   │
                          ├────┼───┤
                          │    │+30│
                          ├────┼───┤
                          │    │   │
                          ├────┼───┤
                          │    │+15│
                          ├────┼───┤
                          │    │   │
                          ├════╪═══┤  ← Center Line (Red)
                          │    │ 0 │    (Current pitch)
                          ├════╪═══┤
                          │    │   │
                          ├────┼───┤
                          │    │-15│
                          ├────┼───┤
                          │    │   │
                          ├────┼───┤
                          │    │-30│
                          ├────┼───┤
                          │    │   │
                          ├────┼───┤
                          │    │-45│
                          └────┴───┘
```

**Features:**
- Vertical scrolling scale
- Small ticks every 5°
- Large ticks with labels every 15°
- Red horizontal line at center
- Currently visible range: ±45° from center
- Background: Transparent

**Pitch Interpretation:**
- Positive (+): Looking up
- 0°: Level/horizon
- Negative (-): Looking down

---

## Full Screen Layout Example

```
╔════════════════════════════════════════════════════════════════════════╗
║                                                                        ║
║    ┌─────── Head Tracker Compass ───────┐                             ║
║    ├─┬─┬─┬───┬─┬─┬───┬─┬─┬───┬─┬─┬───┬──┤                             ║
║   270  280  290  300  310  320  330  340                              ║
║                     ▼                                                  ║
║                                                                        ║
║                                                                        ║
║                                                    ┌────┬───┐          ║
║                                                    │    │+45│          ║
║        Video Feed Area                             ├────┼───┤          ║
║                                                    │    │+30│          ║
║        (1280x720 or 1920x1080)                     ├────┼───┤          ║
║                                                    │    │+15│          ║
║                                                    ├════╪═══┤          ║
║                                                    │    │ 0 │ Altitude ║
║                                                    ├════╪═══┤          ║
║                                                    │    │-15│          ║
║                                                    ├────┼───┤          ║
║                                                    │    │-30│          ║
║                                                    ├────┼───┤          ║
║                                                    │    │-45│          ║
║                                                    └────┴───┘          ║
╚════════════════════════════════════════════════════════════════════════╝
```

---

## Color Scheme

| Element | Color | Purpose |
|---------|-------|---------|
| Scale lines | White (255,255,255) | Tick marks and borders |
| Degree labels | White (255,255,255) | Text showing degrees |
| Center indicators | Red (255,0,0) | Current position markers |
| Background | Transparent | Video pass-through |

---

## Size Specifications

### 720p Mode
- **Compass**: 300 x 40 pixels
- **Altitude**: 40 x 200 pixels

### 1080p Mode (FHD)
- **Compass**: 450 x 60 pixels (1.5x scale)
- **Altitude**: 60 x 300 pixels (1.5x scale)

---

## Dynamic Behavior

### Compass Animation
- As heading changes, the scale scrolls horizontally
- Center indicator remains fixed
- Degree labels update continuously
- Smooth scrolling effect

### Altitude Animation  
- As pitch changes, the scale scrolls vertically
- Center line remains fixed
- Degree labels update continuously
- Smooth scrolling effect

---

## Use Case Example: Antenna Tracking

```
User looking at:              Goggle displays:
┌─────────────────┐          ┌─────────────────┐
│                 │          │  Compass: 045°  │ (NE)
│   Aircraft at   │    →     │  Pitch: +25°    │ (Looking up)
│   NE, elevated  │          │                 │
└─────────────────┘          └─────────────────┘

This tells the antenna tracking system:
- Point antenna 045° (Northeast)
- Elevate antenna +25° above horizon
```

---

## Implementation Notes

- Both elements update in real-time (currently ~20 FPS with demo)
- Elements are hidden when disabled in settings
- Position is fully adjustable via OSD configuration menu
- No performance impact when disabled (elements not rendered)
- Compatible with all video sources (HDMI, AV, HDZero)
