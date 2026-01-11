# FreeCameraFramework (FCFW) - Papyrus Developer Reference

---

## Table of Contents

1. [What is FCFW?](#what-is-fcfw)
2. [Introduction](#introduction)
3. [Core Concepts](#core-concepts)
4. [Timeline Management](#timeline-management)
5. [Building Timelines (Camera Paths)](#building-timelines-camera-paths)
6. [Recording Camera Movement](#recording-camera-movement)
7. [Event Handling](#event-handling)
8. [Import/Export](#importexport)

---

## What is FCFW?

FreeCameraFramework is an SKSE plugin that enables cinematic camera control in Skyrim.

---

## Introduction

This document provides a reference for Papyrus developers who want to create cinematic camera sequences in Skyrim using the FreeCameraFramework (FCFW) plugin. Whether you're building machinima, creating scripted cutscenes, or adding dynamic camera movements to your mod, FCFW provides a powerful and flexible API for camera control.

### Quick Start and References

Download the example mod (FCFW_EXAMPLE) from the FCFW page on Nexus, or grab it directly from the FCFW installation (`Documentation/PapyrusExample`). Install it in your Data directory, and activate FCFW_EXAMPLE.esp in your mod list.

Test the FCFW_EXAMPLE features by pressing the following keys:
* Key '1' - Cinematic Orbit Sequence: Smooth approach from current camera position to orbit entry (center is a marker reference above the player), followed by continuous circular orbit around marker (looping). During this stage the user can rotate the camera freely while it is orbiting. Pressing '1' again triggers a smooth return to initial camera position.
* 'Key '2' - Playback Sequence: Plays back camera timeline. Timeline can be defined via recording (via key '3'), or from imported YAML (via key '5').
* Key '3' - Recording Mode: Records camera movement in real-time. Can be exported to a YAML file via key '4', and played back via Key '2'.
* Key '4' - Export Recorded Timeline to YAML File: Exports recorded timeline to a YAML file for external editing and later import/use. Exported timeline file can be found in the Skyrim Data folder (`FCFW_EXAMPLE_TIMELINE.yaml`).
* Key '5' - Import Recorded Timeline from YAML File: Imports timeline from `FCFW_EXAMPLE_TIMELINE.yaml`. Play back imported timeline via Key '2'.

Review `Source/Scripts/FCFW_EXAMPLE_QUESTSCRIPT.psc` to get an understanding on how the various features are implemented using the FCFW API. Look at the proposed exercises in the comment section and start modifying the script / YAML files.

The complete API documentation can be found in the API file (which you need to include in your mod if you want to use FCFW's functions): `Source/Scripts/FCFW_SKSEFunctions.psc`

To understand the timeline YAML file format, check out the timeline example files in `Documentation/TimelineFileExample/*.yaml`:
- FCFW_Timeline_orbitReference.yaml - a looping orbit around a reference
- FCFW_Timeline_trackReferenceAndReturn.yaml - move camera to reference, track it for a while, then return
- FCFW_Timeline_worldPath.yaml - a pre-recorded world-space camera path with wait at end

The complete timeline YAML format description is here: `Documentation/TimelineFileExample/TIMELINE_FORMAT.md`

For troubleshooting, check FCFW logs: `Documents/My Games/Skyrim Special Edition/SKSE/FreeCameraFramework.log`. You will need to set LogLevel = 2 (or smaller) in `SKSE/Plugins/FreeCameraFramework.ini` in order to get all relevent log messages.

FCFW Source: https://github.com/staalo18/FreeCameraFramework

---

### FCFW Key Features

**Multi-Timeline System**
- Create and manage multiple independent camera timelines
- Each timeline is identified by a unique ID
- Only your mod can access timelines it registered
- One timeline active at a time (either recording or playing)

**Flexible Camera Paths**
- **Translation points** control camera position (where it moves)
- **Rotation points** control camera orientation (where it looks)
- Three point types:
  - **World points**: Static coordinates in world space
  - **Reference points**: Follow or look at objects/actors
  - **Camera points**: Capture current camera state (for smooth transitions)

**Interpolation Modes**
- **None**: Jump instantly to point (no interpolation)
- **Linear**: Linear interpolation between points (straight line for translation points, linear change of angles for rotation points)
- **Cubic Hermite**: Smooth curved paths with automatic tangent calculation
- Per-point easing for acceleration/deceleration between points
- Global easing for entire timeline

**Real-Time Recording**
- Record camera movements as you manually control the camera
- Configurable sampling rate (per-frame or time-based)
- Append to existing timelines or start fresh

**Playback Modes**
- **End mode**: Stop when timeline completes
- **Loop mode**: Restart from beginning with smooth transition
- **Wait mode**: Hold at final position until manual stop
- Optional user rotation control during playback

**Event Callbacks**
- Register Forms to receive playback event notifications
- `OnPlaybackStart`: Fired when timeline playback begins
- `OnPlaybackStop`: Fired when timeline stops or completes (end mode)
- `OnPlaybackWait`: Fired when timeline reaches end in wait mode
- Chain timeline sequences using event-driven logic

**File Operations**
- Export timelines to YAML files for editing
- Import pre-made or edited camera timelines
- Share timelines between mods or sessions

---

## Core Concepts

### Timelines

A **timeline** is a container for camera movement data. Each timeline has:
- A unique **timeline ID** (integer > 0)
- **Translation points** defining camera positions over time
- **Rotation points** defining camera orientations over time
- **Playback settings** (mode, loop offset, user rotation, etc.)

**Important**: Translation and rotation tracks are independent. You can have:
- Different numbers of points in each track
- Different time values for translation vs rotation points
- Different interpolation modes per track

### Points

**Points** are keyframes that define camera state at specific times:

**Translation Points** specify where the camera is located:
```papyrus
; World point at coordinates (1000, 2000, 500) at time 0.0
FCFW_SKSEFunctions.AddTranslationPoint(modName, timelineID, 0.0, 1000.0, 2000.0, 500.0)

; Reference point 200 units in front of actor at time 5.0
FCFW_SKSEFunctions.AddTranslationPointAtRef(modName, timelineID, 5.0, actorRef, 0.0, 200.0, 0.0, isOffsetRelative=true)

; Camera point capturing camera position at the start of playback. Camera translates to this position at time 10.0
FCFW_SKSEFunctions.AddTranslationPointAtCamera(modName, timelineID, 10.0)
```

**Rotation Points** specify where the camera is looking:
```papyrus
; Look at specific pitch/yaw at time 0.0 (world coords)
FCFW_SKSEFunctions.AddRotationPoint(modName, timelineID, 0.0, pitch, yaw)

; Look directly at actor at time 5.0
FCFW_SKSEFunctions.AddRotationPointAtRef(modName, timelineID, 5.0, actorRef, 0.0, 0.0)

; Camera point capturing camera rotation at the start of playback. Camera rotates to this angle at time 10.0
FCFW_SKSEFunctions.AddRotationPointAtCamera(modName, timelineID, 10.0)
```

### Time Values

Time is measured in **seconds** from the start of the timeline:
- First point typically starts at `time = 0.0`
- Subsequent points have increasing time values
- Timeline duration is determined by the last point's time value
- Points are automatically sorted by time when added

### Interpolation

**Interpolation** determines how the camera moves between points:

**Interpolation Modes:**
- `0` = **None**: No interpolation, instant jump to next point
- `1` = **Linear**: Linear interpolation between points (straight line for translation points, linear change of angles for rotation points)
- `2` = **Cubic Hermite**: Smooth curved path with automatic tangent calculation (default, recommended for translation points)

**Easing:**
- `easeIn = true`: Gradually accelerate from previous point
- `easeOut = true`: Gradually decelerate approaching this point
- Both control the **incoming** segment (previous → current point)

**Note on Easing**: 
To ensure smooth passage through a point (no slowdown), use:
```papyrus
; Current point: easeOut = false (don't slow down approaching)
FCFW_SKSEFunctions.AddTranslationPoint(..., easeIn=false, easeOut=false, ...)
; Next point: easeIn = false (don't slow down departing previous point)
FCFW_SKSEFunctions.AddTranslationPoint(..., easeIn=false, easeOut=false, ...)
```

### Playback Modes

**Playback mode** controls what happens when the timeline ends:

**0 = End Mode** (default)
- Timeline stops when reaching the last point
- `OnPlaybackStop` event fires
- Camera instantly returns to the playback's starting position and rotation

**1 = Loop Mode**
- Timeline restarts from the beginning
- Optional `loopTimeOffset` creates smooth interpolation period between last and first point
- Continues looping until `FCFW_SKSEFunctions.StopPlayback()` called
- No event fires when a new loop starts

**2 = Wait Mode**
- Timeline reaches end but continues holding final point (ie continues moving along with reference for reference points)
- `OnPlaybackWait` event fires (not `OnPlaybackStop`)
- Useful for chaining timelines via `FCFW_SKSEFunctions.SwitchPlayback()`
- Must manually call `FCFW_SKSEFunctions.StopPlayback()` to exit

### Ownership

**Every timeline is owned by the mod that registered it.** The mod name (ESP/ESL filename) is required for all API calls to validate ownership. Only the owning mod can:
- Add or remove points
- Start/stop recording or playback
- Modify playback settings
- Query timeline data
- Export the timeline

This prevents conflicts when multiple mods use FCFW simultaneously.

## Timeline Management

### Registering Timelines

**Every timeline must be registered before use:**

```papyrus
int timelineID = FCFW_SKSEFunctions.RegisterTimeline(ModName)

if timelineID <= 0
    Debug.Notification("Failed to register timeline")
    return
endif

; Store timelineID for later use
```

**Important Notes:**
- Timeline IDs are NOT preserved between savegames! That means you will need to ensure that your timelines are (re-)registered when a savegame is loaded
- Each mod can register multiple timelines
- Timeline IDs are unique and never reused
- Registration can fail if mod name is invalid

### Managing Multiple Timelines

You can register and use multiple independent timelines:

```papyrus
Scriptname MyMultiTimelineScript extends Quest

string ModName = "MyMod.esp"

; Different timelines for different purposes
int approachTimelineID = -1
int orbitTimelineID = -1
int returnTimelineID = -1

Function InitializeTimelines()
    approachTimelineID = FCFW_SKSEFunctions.RegisterTimeline(ModName)
    orbitTimelineID = FCFW_SKSEFunctions.RegisterTimeline(ModName)
    returnTimelineID = FCFW_SKSEFunctions.RegisterTimeline(ModName)
    
    ; Each timeline can have completely different content
    BuildApproachPath()
    BuildOrbitPath()
    BuildReturnPath()
EndFunction

Function BuildApproachPath()
    ; Build camera path for approaching a location
    FCFW_SKSEFunctions.ClearTimeline(ModName, approachTimelineID)
    ; ... add points ...
EndFunction

Function BuildOrbitPath()
    ; Build circular orbit path
    FCFW_SKSEFunctions.ClearTimeline(ModName, orbitTimelineID)
    ; ... add points ...
EndFunction

Function BuildReturnPath()
    ; Build return to original position
    FCFW_SKSEFunctions.ClearTimeline(ModName, returnTimelineID)
    ; ... add points ...
EndFunction
```

### Clearing Timelines

**To update timeline content**, use `ClearTimeline()` then add new points:

```papyrus
; Clear all points from timeline
FCFW_SKSEFunctions.ClearTimeline(ModName, timelineID)

; Now rebuild with new points
FCFW_SKSEFunctions.AddTranslationPoint(...)
FCFW_SKSEFunctions.AddRotationPoint(...)
```

**Important**: 
- `ClearTimeline()` removes all points but keeps the timeline ID valid
- Use this to update timeline content, not `UnregisterTimeline()` / `RegisterTimeline()` to avoid unneccessary generation of stale timelineIDs.
- Clearing stops any active playback/recording on that timeline

### Unregistering Timelines

**Only unregister when completely done** with a timeline:

```papyrus
; Cleanup on mod unload or plugin shutdown
FCFW_SKSEFunctions.UnregisterTimeline(ModName, timelineID)
```

**This will:**
- Stop any active playback/recording
- Remove all points
- Free resources
- Make the timeline ID invalid for re-use in the same session

---

## Building Timelines (Camera Paths)

### World Points

**World points** use static coordinates that never change:

```papyrus
; Add translation point at world coordinates
FCFW_SKSEFunctions.AddTranslationPoint(
    ModName,
    timelineID,
    time = 0.0,          ; Time in seconds
    posX = 1000.0,       ; World X coordinate
    posY = 2000.0,       ; World Y coordinate
    posZ = 100.0,        ; World Z coordinate
    easeIn = false,      ; No ease-in from previous point
    easeOut = false,     ; No ease-out approaching this point
    interpolationMode = 2 ; Cubic Hermite (smooth curves)
)

; Add rotation point with specific pitch and yaw
FCFW_SKSEFunctions.AddRotationPoint(
    ModName,
    timelineID,
    time = 0.0,
    pitch = 0.1,         ; Pitch in radians
    yaw = 1.57,          ; Yaw in radians (≈90 degrees)
    easeIn = false,
    easeOut = false,
    interpolationMode = 2
)
```

**Use world points when:**
- Camera should move to specific locations
- Positions are known at script time
- No need to follow moving objects

### Reference Points

**Reference points** dynamically track objects or actors:

```papyrus
ObjectReference targetActor = Game.GetPlayer()

; Position 200 units in front of actor (relative to their heading)
FCFW_SKSEFunctions.AddTranslationPointAtRef(
    ModName,
    timelineID,
    time = 5.0,
    reference = targetActor,
    offsetX = 0.0,       ; Right/left offset
    offsetY = 200.0,     ; Forward/back offset
    offsetZ = 50.0,      ; Up/down offset
    isOffsetRelative = true,  ; Rotate offset with actor heading
    easeIn = false,
    easeOut = false,
    interpolationMode = 2
)

; Look directly at the actor
FCFW_SKSEFunctions.AddRotationPointAtRef(
    ModName,
    timelineID,
    time = 5.0,
    reference = targetActor,
    offsetPitch = 0.0,   ; No pitch offset
    offsetYaw = 0.0,     ; No yaw offset (look straight at target)
    isOffsetRelative = false,  ; Offset from camera-to-ref direction
    easeIn = false,
    easeOut = false,
    interpolationMode = 2
)
```

**Offset Modes:**

**Translation with `isOffsetRelative = true`:**
- Offset rotates with reference heading
- `offsetY = 200` means "200 units in the direction actor is facing"
- Useful for relative offsets like "in front of" or "behind" positioning

**Translation with `isOffsetRelative = false`:**
- Offset in world space (doesn't rotate with reference)
- Useful for fixed offsets like "200 units north"

**Rotation with `isOffsetRelative = true`:**
- Camera looks in the direction reference is facing + offset
- `offsetYaw = 0` means "look where actor looks"
- `offsetYaw = 3.14` means "look opposite direction"

**Rotation with `isOffsetRelative = false`:**
- Camera looks at reference from current position + offset
- `offsetYaw = 0` means "look directly at target"
- `offsetPitch = 0.5` means "look slightly above target"

### Camera Points

**Camera points** capture the camera state at playback start, and use this position any time during playback:

```papyrus
; Capture camera position at StartPlayback and move to this position at time 2.0
FCFW_SKSEFunctions.AddTranslationPointAtCamera(
    ModName,
    timelineID,
    time = 2.0,
    easeIn = false,
    easeOut = true,      ; Slow down approaching next point
    interpolationMode = 2
)

; Capture camera rotation at StartPlayback and move to this rotation at time 2.0
FCFW_SKSEFunctions.AddRotationPointAtCamera(
    ModName,
    timelineID,
    time = 2.0,
    easeIn = false,
    easeOut = true,
    interpolationMode = 2
)
```

**Key Behavior:**
- Position/rotation is sampled when `FCFW_SKSEFunctions.StartPlayback()` is called
- Useful to define smooth camera behavior at the start end the end of a timeline playback


### Procedural Path Generation

**Generate paths programmatically** for complex movements. 

Example: Looping orbit around an ObjectRef:

```papyrus
Function BuildCircularPath(ObjectReference center, float radius, int numPoints, float duration)
    FCFW_SKSEFunctions.ClearTimeline(ModName, timelineID)
    
    float timeStep = duration / numPoints
    
    int i = 0
    while i < numPoints
        float angle = (i as float / numPoints as float) * 360.0
        float time = i * timeStep
        
        ; Calculate position on circle
        float offsetX = radius * Math.Cos(angle)
        float offsetY = radius * Math.Sin(angle)
        
        ; Add point relative to center
        FCFW_SKSEFunctions.AddTranslationPointAtRef(
            ModName, timelineID, time,
            center, offsetX, offsetY, 0.0,
            isOffsetRelative = true
        )
        
        i += 1
    endWhile
    
    ; Add rotation to look at center
    ; Only a single rotation point is needed because the camera is always facing the center (isOffsetRelative=false).
    FCFW_SKSEFunctions.AddRotationPointAtRef(
        ModName, timelineID, 0.0,
        center, 0.0, 0.0,
        isOffsetRelative = false
    )
EndFunction
```

## Recording Camera Movement

**Record camera movements in real-time:**

```papyrus
Function StartCameraRecording()
    
    ; Start recording at 1 point per second (default)
    if !FCFW_SKSEFunctions.StartRecording(ModName, timelineID)
        Debug.Notification("Failed to start recording")
        return
    endif
    
    Debug.Notification("Recording started - move the camera!")
EndFunction

Function StopCameraRecording()
    if !FCFW_SKSEFunctions.StopRecording(ModName, timelineID)
        Debug.Notification("Failed to stop recording")
        return
    endif
    
    int numPoints = FCFW_SKSEFunctions.GetTranslationPointCount(ModName, timelineID)
    Debug.Notification("Recording stopped - captured " + numPoints + " points")
EndFunction
```

---

## Event Handling

### Registering for Events

**Register a Form to receive playback events:**

```papyrus
Scriptname MyCameraQuest extends Quest

Event OnInit()
    ; Register THIS quest to receive events
    FCFW_SKSEFunctions.RegisterForTimelineEvents(self as Form)
EndEvent

Event OnPlayerLoadGame()
    ; Re-register after loading (events don't persist)
    FCFW_SKSEFunctions.RegisterForTimelineEvents(self as Form)
EndEvent
```

**Important**: 
- Only Forms can receive events (Quest, ObjectReference, etc.)
- ReferenceAlias cannot receive events directly
- Events must be re-registered after loading a save

### Event Callbacks

**Three events are available:**

```papyrus
Event OnPlaybackStart(int timelineID)
    ; Fired when any timeline starts playing
    Debug.Notification("Timeline " + timelineID + " started")
EndEvent

Event OnPlaybackStop(int timelineID)
    ; Fired when timeline stops (manual stop or end mode completion)
    Debug.Notification("Timeline " + timelineID + " stopped")
EndEvent

Event OnPlaybackWait(int timelineID)
    ; Fired when timeline reaches end in wait mode
    Debug.Notification("Timeline " + timelineID + " waiting at end")
    ; Timeline is holding at final position - call FCFW_SKSEFunctions.StopPlayback when ready
EndEvent
```

---

## Import/Export

### Exporting Timelines

**Save timelines to YAML files:**

```papyrus
string filename = "MyTimeline.yaml"

if FCFW_SKSEFunctions.ExportTimeline(ModName, timelineID, filename)
    Debug.Notification("Timeline exported to " + filename)
else
    Debug.Notification("Export failed!")
endif
```

**File Location:**
- Relative to Skyrim's `Data/` folder
- `"SKSE/Plugins/MyTimeline.yaml"` saves to `Data/SKSE/Plugins/MyTimeline.yaml`

**Exported files contain:**
- All translation and rotation points
- Interpolation and easing settings
- Playback mode configuration
- Reference formIDs (load-order independent)
- TimelineIDs are NOT saved in YAML, as these are allocated dynamically during runtime.

### Importing Timelines

**Load timelines from YAML files:**

NOTE: TimelineIDs are NOT saved in YAML as these are allocated dynamically during runtime. Import requires valid timelineID.

```papyrus
string filename = "MyTimeline.yaml"

; Clear existing points
FCFW_SKSEFunctions.ClearTimeline(ModName, timelineID)

; Import from file
if FCFW_SKSEFunctions.AddTimelineFromFile(ModName, timelineID, filename)
    Debug.Notification("Timeline imported successfully!")
else
    Debug.Notification("Import failed!")
endif

; Optional: Add time offset to all imported points
if FCFW_SKSEFunctions.AddTimelineFromFile(ModName, timelineID, filename, timeOffset = 5.0)
    Debug.Notification("Timeline imported with 5 second offset")
endif
```

**Time Offset:**
- Adds specified seconds to all imported point times
- Useful for appending imported content to existing timeline
- Does not affect relative timing between points

### YAML Format

See `Documentation/TimelineFileExample/TIMELINE_FORMAT.md` for complete YAML format specification.

### Example YAML Files

See `Documentation/TimelineFileExample/` for example timelines:
- `FCFW_Timeline_orbitReference.yaml` - Looping orbit around a reference
- `FCFW_Timeline_trackReferenceAndReturn.yaml` - Track reference then return
- `FCFW_Timeline_worldPath.yaml` - Pre-recorded world-space path

---
