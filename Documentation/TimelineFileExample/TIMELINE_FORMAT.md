# FreeCameraFramework Timeline YAML Format Reference

Timeline file format is YAML.

**Quick Start:** Export a timeline using `ExportTimeline()` to get an example YAML file you can modify.

---

## File Structure

```yaml
# FreeCameraFramework Timeline (YAML format)
formatVersion: 1

playbackMode: end
loopTimeOffset: 0.0
globalEaseIn: false
globalEaseOut: false
showMenusDuringPlayback: false
allowUserRotation: false
followGround: true
minHeightAboveGround: 0
useDegrees: true

translationPoints:
  - time: 0.0
    type: world
    position: [x, y, z]
    interpolationMode: cubicHermite
    easeIn: false
    easeOut: false

rotationPoints:
  - time: 0.0
    type: world
    rotation: [pitch, roll, yaw]
    interpolationMode: cubicHermite
    easeIn: false
    easeOut: false

fovPoints:
  - time: 0.0
    fov: 80.0
    interpolationMode: cubicHermite
    easeIn: false
    easeOut: false
```

---

## Global Parameters

All global parameters are optional except `formatVersion`.

### `formatVersion`
- **Type:** integer
- **Required:** Yes
- **Current Version:** 1
- **Description:** YAML format version for backward compatibility

### `playbackMode`
- **Type:** string
- **Default:** `end`
- **Valid Values:**
  - `end` - Stop playback when timeline completes
  - `loop` - Restart from beginning when timeline completes
  - `wait` - Stay at final position until manual `StopPlayback()` call
- **Description:** Controls timeline behavior at completion

### `loopTimeOffset`
- **Type:** float
- **Default:** `0.0`
- **Units:** Seconds
- **Description:** time offset between the last and first point in the loop. Only used when `playbackMode: loop`.

### `globalEaseIn`
- **Type:** boolean
- **Default:** `false`
- **Description:** Apply smooth acceleration at the beginning of the entire timeline, independent of per-point easing

### `globalEaseOut`
- **Type:** boolean
- **Default:** `false`
- **Description:** Apply smooth deceleration at the end of the entire timeline, independent of per-point easing

### `showMenusDuringPlayback`
- **Type:** boolean
- **Default:** `false`
- **Description:** Keep game UI (HUD, menus) visible during playback. Set `false` for cinematic shots.

### `allowUserRotation`
- **Type:** boolean
- **Default:** `false`
- **Description:** Allow user to manually control camera rotation during playback

### `followGround`
- **Type:** boolean
- **Default:** `true`
- **Description:** Automatically adjust camera height to stay above ground level during playback. Prevents camera from going underground.

### `minHeightAboveGround`
- **Type:** float
- **Default:** `0.0`
- **Units:** Game units (Skyrim units)
- **Description:** Minimum height above ground level when `followGround` is enabled. Camera will be raised if below this threshold.

### `useDegrees`
- **Type:** boolean
- **Default:** `true` (when exporting)
- **Description:** Rotation angle units. `true` = degrees, `false` = radians. Engine always uses radians internally; conversion happens during import/export.

---

## Point Parameters (Common)

All point types (translation and rotation) support these parameters:

### `time`
- **Type:** float
- **Required:** Yes
- **Units:** Seconds
- **Description:** Keyframe timestamp. Points are sorted by time automatically.

### `type`
- **Type:** string
- **Required:** Yes
- **Valid Values:**
  - `world` - Static coordinates in world space
  - `reference` - Dynamic position/rotation relative to a reference object
  - `camera` - Relative to camera position/rotation at playback start
- **Description:** Determines how the point's value is calculated

### `interpolationMode`
- **Type:** string
- **Default:** `cubicHermite`
- **Valid Values:**
  - `none` - Instant jump to this point (no interpolation)
  - `linear` - Linear interpolation at constant velocity
  - `cubicHermite` - Smooth cubic Hermite spline with velocity continuity
- **Description:** Interpolation method from this point to the next

### `easeIn`
- **Type:** boolean
- **Default:** `false`
- **Description:** Smooth acceleration at the **start** of the segment arriving at this point (from previous point). Camera starts slow and gradually speeds up. Has **no effect on first point** (no incoming segment),unless in loop mode, where it affects the virtual loop segment (last point → first point).

### `easeOut`
- **Type:** boolean
- **Default:** `false`
- **Description:** Smooth deceleration at the **end** of the segment arriving at this point (from previous point). Camera gradually slows down before reaching this point. Has **no effect on first point** (no incoming segment), unless in loop mode, where it affects the virtual loop segment (last point → first point).

**Easing combinations for a segment:**
- Both `false`: Continuous velocity when approaching the point 
- `easeIn=true, easeOut=false`: Accelerate into point, maintain speed
- `easeIn=false, easeOut=true`: Maintain speed, decelerate before point
- Both `true`: Smooth S-curve (ease-in-out) - starts slow, speeds up, slows down

**Note:** Each point controls only its **incoming** segment (previous → current). If you want smooth transition through a point, you'll need to set easeOut=false for this point, AND easeIn=false for the next point.

---

## Translation Points

Translation points define camera position over time. Array: `translationPoints`

### World Type

Static world coordinates.

**Required Fields:**
- `time`
- `type: world`
- `position: [x, y, z]` - World coordinates (floats)

**Example:**
```yaml
- time: 0.0
  type: world
  position: [5247.3, -3891.2, 1024.5]
  interpolationMode: cubicHermite
  easeIn: false
  easeOut: false
```

### Reference Type

Position relative to a reference object (NPC, marker, etc.).

**Required Fields:**
- `time`
- `type: reference`
- `reference:` - Object with these sub-fields:
  - `editorID: "string"` - Load-order independent reference ID (optional, but recommended)
  - `plugin: "string"` - Plugin filename
  - `formID: 0xHEX` - FormID in hexadecimal notation
- `offset: [x, y, z]` - Offset from reference position

**Optional Fields:**
- `isOffsetRelative: boolean` (default: `false`)
  - `false` - Offset in world-space (Y axis = north)
  - `true` - Offset in reference's local space (rotates with reference heading)
- `bodyPart: string` (default: `none`)
  - Valid values: `none`, `head`, `torso`
  - When set to a body part, offset is relative to that body part's position and rotation (for actors/characters)
  - Only affects references that are actors; ignored for other reference types

**Reference Resolution Priority:**
1. If `editorID` present, look up by EditorID (load-order independent)
2. If EditorID fails or missing, use `plugin` + `formID` combination
3. If resolution fails, uses offset as absolute world position

**Example (EditorID):**
```yaml
- time: 0.0
  type: reference
  reference:
    editorID: "PlayerRef"
  offset: [0.0, -200.0, 100.0]  # 200 units behind, 100 up
  isOffsetRelative: false
  interpolationMode: cubicHermite
```

**Example (Plugin + FormID):**
```yaml
- time: 2.0
  type: reference
  reference:
    plugin: "Skyrim.esm"
    formID: 0x14      # PlayerRef
  offset: [100.0, 0.0, 50.0]
  isOffsetRelative: true
  bodyPart: head    # Track player's head position
  interpolationMode: linear
```

### Camera Type

Relative to camera position at playback start.

**Required Fields:**
- `time`
- `type: camera`
- `offset: [x, y, z]` - Offset from camera position at `StartPlayback()` call

**Behavior:** Camera position is captured once when playback starts. Offset is added to create absolute world coordinates. After baking, behaves as static world point during playback.

**Example:**
```yaml
- time: 0.0
  type: camera
  offset: [0.0, 0.0, 0.0]       # Start at camera position
- time: 5.0
  type: camera
  offset: [100.0, 0.0, 50.0]    # Move 100 forward, 50 up from start
```

---

## Rotation Points

Rotation points define camera aim direction over time. Array: `rotationPoints`

**Angle Format:**
- `pitch` - Vertical angle (positive = up, negative = down)
- `roll` - Camera tilt/banking (positive = clockwise rotation, negative = counter-clockwise)
- `yaw` - Horizontal angle (0° = north, 90° = east, 180° = south, 270° = west)
- Units determined by `useDegrees` setting

### World Type

Static pitch/roll/yaw angles in world space.

**Required Fields:**
- `time`
- `type: world`
- `rotation: [pitch, roll, yaw]` - Angles in degrees or radians

**Example:**
```yaml
- time: 0.0
  type: world
  rotation: [15.0, 0.0, 90.0]     # Looking east, tilted 15° up, no roll
  interpolationMode: cubicHermite
```

### Reference Type

Rotation pointing at or relative to reference.

**Required Fields:**
- `time`
- `type: reference`
- `reference:` - Same format as translation reference (editorID or plugin+formID)
- `offset: [pitch, roll, yaw]` - Angle offset

**Optional Fields:**
- `isOffsetRelative: boolean` (default: `false`)
  - `false` - Look-at mode: Calculate direction to reference, apply offset to aim
  - `true` - Heading mode: Match reference's facing direction, apply offset

**Example (Look-at):**
```yaml
- time: 0.0
  type: reference
  reference:
    editorID: "PlayerRef"
  offset: [10.0, 0.0, 0.0]      # Look at player, 10° above center, no roll or yaw offset
  isOffsetRelative: false
  interpolationMode: linear
```

**Example (Heading):**
```yaml
- time: 2.0
  type: reference
  reference:
    editorID: "PlayerRef"
  offset: [0.0, 0.0, 45.0]      # Face same direction as player, rotated 45° right, no pitch or roll offset
  isOffsetRelative: true
  interpolationMode: cubicHermite
```

### Camera Type

Rotation relative to camera aim at playback start.

**Required Fields:**
- `time`
- `type: camera`
- `offset: [pitch, roll, yaw]` - Angle offset from camera rotation at `StartPlayback()` call

**Behavior:** Camera rotation is captured once when playback starts. Offset is added to create absolute pitch/roll/yaw. After baking, behaves as static world rotation during playback.

**Example:**
```yaml
- time: 0.0
  type: camera
  offset: [0.0, 0.0, 0.0]       # Start at current camera rotation
- time: 3.0
  type: camera
  offset: [0.0, 0.0, 90.0]      # Rotate 90° right from start, no pitch or roll change
```

---

## FOV Points

FOV (Field of View) points define camera zoom level over time. Array: `fovPoints`

**FOV Values:**
- Valid range: 1-160 degrees
- Default: 80 degrees
- Lower values = zoomed in (telephoto effect)
- Higher values = zoomed out (wide angle effect)
- Values outside 1-160 range are clamped to 80 with warning logged

**Required Fields:**
- `time` - Time in seconds
- `fov` - Field of view angle in degrees

**Optional Fields:**
- `interpolationMode` - Interpolation between this and next point (default: `cubicHermite`)
  - `none` - Jump to FOV value instantly
  - `linear` - Linear interpolation
  - `cubicHermite` - Smooth curved transition
- `easeIn` - Smooth acceleration from previous point (default: `false`)
- `easeOut` - Smooth deceleration to this point (default: `false`)

**Note:** FOV is always in world space. Unlike translation/rotation, there is no reference tracking or camera-relative mode.

**Example (Zoom sequence):**
```yaml
fovPoints:
  - time: 0.0
    fov: 80.0                    # Start at default FOV
    interpolationMode: cubicHermite
    easeIn: false
    easeOut: false
    
  - time: 3.0
    fov: 30.0                    # Zoom in (telephoto)
    interpolationMode: cubicHermite
    easeIn: true
    easeOut: true
    
  - time: 6.0
    fov: 120.0                   # Zoom out (wide angle)
    interpolationMode: linear
    easeIn: false
    easeOut: false
```

---

## Complete Example

```yaml
# FreeCameraFramework Timeline (YAML format)
formatVersion: 1

playbackMode: loop
loopTimeOffset: 0.5
globalEaseIn: true
globalEaseOut: true
showMenusDuringPlayback: false
allowUserRotation: false
useDegrees: true

translationPoints:
  - time: 0.0
    type: reference
    reference:
      editorID: "MyTargetMarker"
    offset: [500.0, 0.0, 100.0]
    isOffsetRelative: true
    interpolationMode: cubicHermite
    easeIn: false
    easeOut: false
    
  - time: 4.0
    type: reference
    reference:
      editorID: "MyTargetMarker"
    offset: [-500.0, 0.0, 100.0]
    isOffsetRelative: true
    interpolationMode: cubicHermite
    easeIn: false
    easeOut: false

rotationPoints:
  - time: 0.0
    type: reference
    reference:
      editorID: "MyTargetMarker"
    offset: [0.0, 0.0]
    isOffsetRelative: false
    interpolationMode: linear
    easeIn: false
    easeOut: false

fovPoints:
  - time: 0.0
    fov: 80.0
    interpolationMode: cubicHermite
    easeIn: false
    easeOut: false
    
  - time: 2.0
    fov: 60.0
    interpolationMode: cubicHermite
    easeIn: true
    easeOut: true
```

---
