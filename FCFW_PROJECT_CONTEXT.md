# **FreeCameraFramework (FCFW) - Project Context Document**
*Complete technical reference for architectural understanding and refactoring*

---

## **Project Overview**

### **What is FCFW?**
FreeCameraFramework (FCFW) is an SKSE plugin for Skyrim Anniversary Edition that enables cinematic camera path creation and playback. It allows users to record smooth camera movements in Skyrim's free camera mode, edit them with interpolation and easing, and play them back for creating machinima, screenshots, or dynamic scene presentations.

**Core Features:**
- **Multi-Timeline System**: Manage unlimited independent timelines with unique IDs per consumer
  - Each mod/plugin can register and own multiple timelines simultaneously
  - Ownership validation ensures only timeline creators can modify their content
  - Exclusive playback/recording: only one timeline active at any time
- **Timeline-Based Camera Control**: Create camera paths using keyframe-based timelines with translation (position) and rotation points
- **Three Point Types**:
  - **World Points**: Static coordinates in world space
  - **Reference Points**: Dynamic tracking of game objects (NPCs, items) with offset support
  - **Camera Points**: Capture current camera position/rotation during recording
- **Flexible Recording**: Real-time camera path recording with 1-second sampling intervals
- **Advanced Interpolation**: Support for linear and cubic Hermite interpolation with per-point and global easing
- **Import/Export**: Save/load camera paths as `.yaml` timeline files with load-order independent reference tracking
- **Event Callback System**: Notify consumers when timeline playback starts/stops/completes
  - **SKSE Messaging Interface**: C++ plugins receive events via SKSE messaging system
  - **Papyrus Events**: Scripts receive `OnPlaybackStart`, `OnPlaybackStop`, and `OnPlaybackWait` events with timeline ID
- **Three API Surfaces**:
  - **Papyrus Scripts**: Full scripting integration with mod name + timeline ID parameters
  - **C++ Mod API**: Native plugin-to-plugin communication with SKSE plugin handle validation
- **Point Query API**: Query translation and rotation points for external visualization

**Target Users:**
- Skyrim machinima creators needing cinematic camera control
- Modders building scripted scenes with dynamic camera movements
- Screenshot artists creating complex camera animations
- Developers extending FCFW functionality through the C++ API

---

## **Developer Quick Start**

### **Project Architecture Summary**
FCFW follows a **singleton manager pattern** with **multi-timeline storage** and **template-based timeline engines**. The core design separates concerns into:
1. **API Layer**: Two entry points (Papyrus, C++ Mod API) → all funnel into `TimelineManager`
2. **Orchestration Layer**: `TimelineManager` singleton manages timeline map and coordinates recording/playback state
   - Stores `std::unordered_map<size_t, TimelineState>` for multi-timeline support
   - Tracks ownership via SKSE plugin handles
   - Enforces exclusive playback/recording (only one active timeline at a time)
3. **Timeline Layer**: `Timeline` class pairs translation and rotation tracks with metadata
4. **Engine Layer**: `TimelineTrack<T>` template handles interpolation and playback mechanics
5. **Storage Layer**: `CameraPath<T>` template manages point collections and file I/O

**Key Design Patterns:**
- **Singleton**: `TimelineManager` (thread-safe Meyer's singleton)
- **Multi-Timeline Storage**: `std::unordered_map<size_t, TimelineState>` with atomic ID generation
  - Thread-safe access via `std::mutex m_timelineMutex`
  - Each `TimelineState` contains: `Timeline`, ownership info, recording/playback state
- **Ownership Validation**: Two-tier system for external API vs internal operations
  - **External API (C++, Papyrus):** `GetTimeline(timelineID, pluginHandle)` validates ownership for ALL modification and query operations
    - Returns nullptr if timeline not found OR not owned by calling plugin
    - Security model: Prevents plugins from accessing/modifying each other's timelines
  - **Internal Operations (Hooks, Update Loop):** Bypass ownership validation
    - Hooks use overloaded query methods: `IsPlaybackRunning(timelineID)`, `IsUserRotationAllowed(timelineID)` (no pluginHandle parameter)
    - Update loop uses direct map access: `m_timelines.find(m_activeTimelineID)`
    - TimelineManager helpers access state members directly: `state->m_isPlaybackRunning`, `state->m_allowUserRotation`
    - Rationale: Internal FCFW code must check ANY active timeline regardless of owner (for input blocking during playback from external plugins)
- **Exclusive Active Timeline**: `m_activeTimelineID` tracks single active timeline (recording OR playback)
- **Paired Tracks**: `Timeline` class coordinates independent `TimelineTrack<TranslationPath>` and `TimelineTrack<RotationPath>`
- **Three-Tier Encapsulation**: 
  - `Timeline` hides `TimelineTrack` implementation from `TimelineManager`
  - `TimelineTrack` hides `Path` implementation via wrapper methods (GetPath() private)
  - `Path` fully encapsulated - only accessible through `TimelineTrack` wrappers
- **Template Specialization**: `TimelineTrack<T>` provides type-safe interpolation for position and rotation data
- **Track Independence**: Translation and rotation tracks can have different point counts and keyframe times
- **Dependency Injection**: All interpolation/easing math lives in external `_ts_SKSEFunctions` workspace
- **Hook-Based Updates**: Skyrim's main loop (`MainUpdateHook`) drives all timeline updates

**File Structure:**
```
src/
  ├─ plugin.cpp           # SKSE entry point + Papyrus API bindings (20+ functions)
  ├─ TimelineManager.cpp  # Recording/playback orchestration (singleton)
  ├─ Timeline.cpp         # Timeline class implementations (paired track coordination)
  ├─ CameraPath.cpp       # Point storage + YAML import/export
  ├─ Hooks.cpp            # Game loop interception (MainUpdateHook)
  └─ FCFW_Utils.cpp       # Hermite interpolation + file parsing + YAML enum converters

include/
  ├─ TimelineTrack.h      # TimelineTrack<T> template (declaration + implementation)
  ├─ Timeline.h           # Timeline class declaration (non-template wrapper)
  ├─ CameraPath.h         # Point storage templates (TranslationPath, RotationPath)
  ├─ CameraTypes.h        # Core enums (InterpolationMode, PointType, PlaybackMode)
  ├─ TimelineManager.h    # Main orchestrator interface
  └─ FCFW_API.h           # C++ Mod API + event messaging interface definition
```

**Data Flow:**
```
User Input (Papyrus/ModAPI)
  ↓
TimelineManager (validate timeline ID + ownership, orchestrate)
  ↓
GetTimeline(timelineID, pluginHandle) → TimelineState*
  ↓
TimelineState->m_timeline.AddTranslationPoint() / AddRotationPoint()
  ↓
TimelineTrack<PathType>::AddPoint() (internal: stores in m_path via private GetPath())
  ↓
MainUpdateHook (every frame)
  ↓
TimelineManager::Update()
  ├─ RecordTimeline(TimelineState*) → sample camera @ 1Hz → Timeline::AddPoint()
  ├─ PlayTimeline(TimelineState*) → Timeline::UpdatePlayback(deltaTime) 
  │                 → Timeline::GetTranslation/Rotation(time)
  │                 → TimelineTrack::GetPointAtTime(t)
  │                 → (internal: m_path access for interpolation)
  └─ Point queries available via GetTranslationPoint/GetRotationPoint API
  ↓
Apply to RE::FreeCameraState (position/rotation)
```

**Encapsulation Boundaries:**
- **TimelineManager** → sees only Timeline public API
- **Timeline** → sees TimelineTrack public API (GetPath() hidden)
- **TimelineTrack** → directly accesses m_path member (Path fully encapsulated)

**Common Tasks:**
- **Adding New API Functions**: Bind in `plugin.cpp` (Papyrus), implement in `TimelineManager`, expose in `FCFW_API.h` (Mod API), add to `ModAPI.h` overrides
- **Changing Interpolation**: Modify `TimelineTrack<T>::GetPointAtTime()` in `Timeline.inl` (calls interpolation helpers)
- **File Format Changes**: Update `CameraPath::ExportPath()` and `AddPathFromFile()` in `CameraPath.cpp`
- **New Point Types**: Extend `PointType` enum in `CameraTypes.h`, add cases in `TranslationPoint::GetPoint()` / `RotationPoint::GetPoint()` methods
- **Querying Timeline Data**: Use `GetTranslationPointCount()`/`GetRotationPointCount()` to get point counts, then `GetTranslationPoint()`/`GetRotationPoint()` (C++) or individual coordinate functions (Papyrus) to iterate through points by index

**Build Dependencies:**
- **CommonLibSSE-NG**: SKSE plugin framework (set `COMMONLIBSSE_PATH` env var)
- **_ts_SKSEFunctions**: Utility library (must be sibling directory) - provides interpolation/easing functions
- **vcpkg**: Dependency manager (fmt, spdlog, yaml-cpp)
- **CMake 3.29+**: Build system with MSVC presets

**Testing Entry Points:**
1. Launch Skyrim AE with SKSE
2. Open console: `tfc` (enable free camera)
3. From Papyrus: Call `FCFW_RegisterTimeline("YourMod.esp")` to get a timeline ID
4. Call `FCFW_StartRecording("YourMod.esp", timelineID)` to begin recording
5. Move camera, then call `FCFW_StopRecording("YourMod.esp", timelineID)` to stop
6. Exit free camera (`tfc`), then call `FCFW_StartPlayback("YourMod.esp", timelineID, ...)` to play timeline
7. Check logs: `Documents/My Games/Skyrim Special Edition/SKSE/FreeCameraFramework.log`

**Common Pitfalls & Debugging:**
- **C++ API Returns 0 or -1**: Check that `RegisterTimeline()` succeeded before using returned ID. ID of 0 indicates registration failure (check logs for errors).
- **Crash on API Call**: All API functions validate timeline ID and ownership before proceeding. If crashing, check for use-after-move bugs or null pointer dereferences in wrapper code.
- **Timeline Operations Fail Silently**: Verify ownership - only the plugin that registered a timeline can access it. ALL operations (queries, playback control, modification) require ownership validation and will fail if the timeline doesn't belong to the calling plugin.
- **Missing Log Output**: Ensure `FreeCameraFramework.ini` has correct `LogLevel` (0-6, default 3=info). Use-after-move bugs can cause logging to crash before return values propagate.

---

## **Quick Reference**

### **Architecture at a Glance:**
```
Entry Points (2 surfaces):
  ├─ Papyrus API (plugin.cpp) → FCFW_SKSEFunctions script
  │   └─ Parameters: string modName, int timelineID, ...
  └─ Mod API (ModAPI.h) → FCFW_API::IVFCFW1 interface
      └─ Parameters: size_t timelineID, SKSE::PluginHandle, ...

Core Engine:
  ├─ TimelineManager (singleton) → Multi-timeline orchestration
  │   ├─ std::unordered_map<size_t, TimelineState> m_timelines
  │   ├─ std::atomic<size_t> m_nextTimelineID (auto-increment)
  │   ├─ size_t m_activeTimelineID (exclusive playback/recording)
  │   └─ Ownership validation via SKSE::PluginHandle
  │
  └─ TimelineState (per-timeline storage)
      ├─ size_t m_id (unique identifier)
      ├─ Timeline m_timeline (paired tracks)
      │   ├─ TimelineTrack<TranslationPath> → Position keyframes
      │   └─ TimelineTrack<RotationPath> → Rotation keyframes
      ├─ SKSE::PluginHandle m_ownerHandle (ownership tracking)
      ├─ std::string m_ownerName (for logging)
      ├─ bool m_isRecording (per-timeline recording state)
      └─ bool m_isPlaybackRunning (per-timeline playback state)

Update Loop (Hooks.cpp):
  MainUpdateHook → TimelineManager::Update() every frame
    ├─ PlayTimeline(activeState) → Apply interpolated camera state
    └─ RecordTimeline(activeState) → Sample camera every 1 second
```

### **Key Data Structures:**
- **TimelineState:** Per-timeline container {Timeline, ownerPluginHandle, recording state, playback state}
- **Transition:** `{time, mode, easeIn, easeOut}` - Keyframe metadata
- **PointType:** `kWorld` (static), `kReference` (dynamic), `kCamera` (baked)
- **InterpolationMode:** `kNone`, `kLinear`, `kCubicHermite`
- **PlaybackMode:** `kEnd` (stop), `kLoop` (wrap with offset), `kWait` (stay at final position)

### **Critical Patterns:**
- **Timeline Registration:** Call `RegisterTimeline(pluginHandle)` to get unique timeline ID (required first step)
- **Ownership Validation:** `GetTimeline(timelineID, pluginHandle)` validates before all operations
- **Exclusive Active Timeline:** Only ONE timeline can record/play at a time (m_activeTimelineID)
- **Point Construction:** Always pass `Transition` object (not raw time)
- **Type Conversion:** `int` → `InterpolationMode` at API boundaries
- **User Rotation:** Per-timeline accumulated offset (`state->rotationOffset`)
- **kCamera Baking:** `UpdateCameraPoints(state)` at `StartPlayback(timelineID, ...)`

---

## **1. Project Overview**
- **Purpose**: Skyrim Special Edition SKSE plugin for cinematographic camera control
- **Core Feature**: Multi-timeline camera movement system with keyframe interpolation
- **Language**: C++ (SKSE CommonLibSSE-NG framework)
- **Architecture**: Multi-timeline system with unlimited independent timelines per mod/plugin
  - Ownership validation via SKSE plugin handles
  - Timeline ID + mod name/plugin handle required for all API calls
  - Exclusive playback/recording enforcement (one active timeline at a time)
  - Full API support: Papyrus scripts and C++ plugins

---

## **2. Entry Points & API Surface**

### **2.1 Plugin Initialization (`plugin.cpp`)**
```
SKSEPlugin_Load()
├─> _ts_SKSEFunctions::InitializeLogging() [log level from INI]
├─> Register SKSE message listener → MessageHandler()
├─> Register Papyrus functions → FCFW::Interface::FCFWFunctions()
└─> Install Hooks → Hooks::Install()

MessageHandler() [SKSE lifecycle events]
├─> kDataLoaded/kPostLoad/kPostPostLoad: APIs::RequestAPIs()
└─> kPostLoadGame/kNewGame: APIs::RequestAPIs()

RequestPluginAPI(InterfaceVersion) [Mod API entry]
└─> Returns FCFWInterface singleton for V1
```

**Key Constants:**
- Plugin version: Encoded as `major * 10000 + minor * 100 + patch`
- Log levels: 0-6 (spdlog), defaults to 3 (info) if invalid
- INI path: `SKSE/Plugins/FreeCameraFramework.ini`

---

### **2.2 Papyrus API (`plugin.cpp` - FCFW::Interface namespace)**
**Registration:** All functions bound to `FCFW_SKSEFunctions` Papyrus script

**Multi-Timeline Management:**
| Papyrus Function | Return | Parameters | Notes |
|-----------------|--------|------------|-------|
| `FCFW_RegisterTimeline` | int | modName | Returns new timeline ID for your mod |
| `FCFW_UnregisterTimeline` | bool | modName, timelineID | Remove timeline (requires ownership) |

**Event Callback Registration:**
| Papyrus Function | Return | Parameters | Notes |
|-----------------|--------|------------|-------|
| `FCFW_RegisterForTimelineEvents` | void | form | Register a form to receive playback events |
| `FCFW_UnregisterForTimelineEvents` | void | form | Unregister a form from receiving events |

**Camera Utility Functions:**
| Papyrus Function | Return | Parameters | Notes |
|-----------------|--------|------------|-------|
| `FCFW_GetCameraPosX` | float | - | Get current camera X position (world coordinates) |
| `FCFW_GetCameraPosY` | float | - | Get current camera Y position (world coordinates) |
| `FCFW_GetCameraPosZ` | float | - | Get current camera Z position (world coordinates) |
| `FCFW_GetCameraPitch` | float | - | Get current camera pitch in radians |
| `FCFW_GetCameraYaw` | float | - | Get current camera yaw in radians |

**Timeline Building Functions:**
| Papyrus Function | Return | Parameters | Notes |
|-----------------|--------|------------|-------|
| `FCFW_AddTranslationPointAtCamera` | int | **modName, timelineID**, time, easeIn, easeOut, interpolationMode | Captures camera position |
| `FCFW_AddTranslationPoint` | int | **modName, timelineID**, time, posX, posY, posZ, easeIn, easeOut, interpolationMode | Absolute world position (Papyrus accepts individual coordinates) |
| `FCFW_AddTranslationPointAtRef` | int | **modName, timelineID**, time, ref, offsetX/Y/Z, isOffsetRelative, easeIn, easeOut, interpolationMode | Ref-based position |
| `FCFW_AddRotationPointAtCamera` | int | **modName, timelineID**, time, easeIn, easeOut, interpolationMode | Captures camera rotation |
| `FCFW_AddRotationPoint` | int | **modName, timelineID**, time, pitch, yaw, easeIn, easeOut, interpolationMode | Absolute world rotation (Papyrus accepts individual pitch/yaw) |
| `FCFW_AddRotationPointAtRef` | int | **modName, timelineID**, time, ref, offsetPitch/Yaw, isOffsetRelative, easeIn, easeOut, interpolationMode | Ref-based rotation |
| `FCFW_RemoveTranslationPoint` | bool | **modName, timelineID**, index | Remove by index (requires ownership) |
| `FCFW_RemoveRotationPoint` | bool | **modName, timelineID**, index | Remove by index (requires ownership) |
| `FCFW_ClearTimeline` | bool | **modName, timelineID** | Clear all points (requires ownership) |
| `FCFW_GetTranslationPointCount` | int | **modName, timelineID** | Query point count (-1 if timeline not found) |
| `FCFW_GetRotationPointCount` | int | **modName, timelineID** | Query point count (-1 if timeline not found) |
| `FCFW_GetTranslationPointX` | float | **modName, timelineID**, index | Get X coordinate of translation point. Returns 0.0 on error. |
| `FCFW_GetTranslationPointY` | float | **modName, timelineID**, index | Get Y coordinate of translation point. Returns 0.0 on error. |
| `FCFW_GetTranslationPointZ` | float | **modName, timelineID**, index | Get Z coordinate of translation point. Returns 0.0 on error. |
| `FCFW_GetRotationPointPitch` | float | **modName, timelineID**, index | Get pitch (radians) of rotation point. Returns 0.0 on error. |
| `FCFW_GetRotationPointYaw` | float | **modName, timelineID**, index | Get yaw (radians) of rotation point. Returns 0.0 on error. |

**Recording Functions:**
| Papyrus Function | Return | Parameters | Notes |
|-----------------|--------|------------|-------|
| `FCFW_StartRecording` | bool | **modName, timelineID** | Begin capturing (requires ownership) |
| `FCFW_StopRecording` | bool | **modName, timelineID** | Stop capturing (requires ownership) |

**Playback Functions:**
| Papyrus Function | Return | Parameters | Notes |
|-----------------|--------|------------|-------|
| `FCFW_StartPlayback` | bool | **modName, timelineID**, speed, globalEaseIn, globalEaseOut, useDuration, duration | Begin timeline playback (validates ownership) |
| `FCFW_StopPlayback` | bool | **modName, timelineID** | Stop playback (validates ownership) |
| `FCFW_SwitchPlayback` | bool | **modName, fromTimelineID, toTimelineID** | Glitch-free timeline switch (validates ownership of both source and target timelines) |
| `FCFW_PausePlayback` | bool | **modName, timelineID** | Pause playback (validates ownership) |
| `FCFW_ResumePlayback` | bool | **modName, timelineID** | Resume from pause (validates ownership) |
| `FCFW_IsPlaybackPaused` | bool | **modName, timelineID** | Query pause state (validates ownership) |
| `FCFW_IsPlaybackRunning` | bool | **modName, timelineID** | Query playback state (validates ownership) |
| `FCFW_IsRecording` | bool | **modName, timelineID** | Query recording state (validates ownership) |
| `FCFW_GetActiveTimelineID` | int | - | Get ID of currently active timeline (0 if none) |
| `FCFW_AllowUserRotation` | void | **modName, timelineID**, allow | Enable/disable user camera control (validates ownership) |
| `FCFW_IsUserRotationAllowed` | bool | **modName, timelineID** | Query user rotation state (validates ownership) |
| `FCFW_SetPlaybackMode` | bool | **modName, timelineID**, playbackMode, loopTimeOffset | Set playback mode (0=kEnd, 1=kLoop, 2=kWait) and loop time offset - requires ownership |

**Import/Export Functions:**
| Papyrus Function | Return | Parameters | Notes |
|-----------------|--------|------------|-------|
| `FCFW_AddTimelineFromFile` | bool | **modName, timelineID**, filePath, timeOffset | Import YAML file (requires ownership) |
| `FCFW_ExportTimeline` | bool | **modName, timelineID**, filePath | Export to YAML file (validates ownership) |

**Parameter Notes:**
- **modName**: Your mod's ESP/ESL filename (e.g., `"MyMod.esp"`), case-sensitive. Required for ALL timeline operations to validate that the calling plugin has access to the specified timeline.
- **timelineID**: Integer ID returned by `RegisterTimeline()`, must be > 0
- **playbackMode**: Integer value for PlaybackMode enum (0=kEnd, 1=kLoop, 2=kWait) in API calls; strings ("end", "loop", "wait") in YAML files
- **loopTimeOffset**: Time offset in seconds when looping back (only used in kLoop mode, default: 0.0)
- **index**: 0-based index for point queries. Functions return 0.0 if index is out of range or timeline not found.
- **Ownership Validation**: Applied universally to all timeline API calls. The pluginHandle/modName parameter is required as the FIRST parameter and is always validated - operations fail if the timeline doesn't exist or doesn't belong to the calling plugin.
- **Return Values**: `-1` for query functions on error, `false` for boolean functions on error

**Type Conversion Layer:**
- **InterpolationMode**: Papyrus passes `int` (0=None, 1=Linear, 2=CubicHermite), converted via `ToInterpolationMode()` in `CameraTypes.h`
- **Mod Name to Handle**: `ModNameToHandle(modName)` searches loaded files for matching ESP/ESL, returns `file->compileIndex` as plugin handle
- **Return Values**: TimelineManager returns `int` for point indices, `bool` for operations

**Event System:**
Papyrus scripts can register forms to receive timeline playback events. The form **must extend Form** (Quest, ObjectReference, etc.) and have the event handler functions defined.

**IMPORTANT:** Only Forms can receive events. ReferenceAlias and other Alias types cannot receive events because they don't extend Form. If you need event handling in an alias script, use a Quest script instead and reference the player via `Game.GetPlayer()`.

Registered forms receive these event callbacks:
- `OnPlaybackStart(int timelineID)` - Fired when any timeline starts playing
- `OnPlaybackStop(int timelineID)` - Fired when any timeline stops playing (manual stop or kEnd mode completion)
- `OnPlaybackWait(int timelineID)` - Fired when timeline reaches end in kWait mode (stays at final position)

**Example Usage:**
```papyrus
Scriptname MyQuest extends Quest

Event OnInit()
    ; Register THIS quest (which extends Form) to receive events
    FCFW_SKSEFunctions.FCFW_RegisterForTimelineEvents(self as Form)
EndEvent

Function SetupTimeline()
    ; First, register a new timeline to get its ID
    int timelineID = FCFW_SKSEFunctions.FCFW_RegisterTimeline("MyMod.esp")
    
    ; Add some points...
    FCFW_SKSEFunctions.FCFW_AddTranslationPoint("MyMod.esp", timelineID, 0.0, 100.0, 200.0, 300.0, false, false, 2)
    FCFW_SKSEFunctions.FCFW_AddTranslationPoint("MyMod.esp", timelineID, 5.0, 400.0, 500.0, 600.0, false, false, 2)
    
    ; Set playback mode to kWait (2) - timeline will stay at final position
    FCFW_SKSEFunctions.FCFW_SetPlaybackMode("MyMod.esp", timelineID, 2)
    
    ; Start playback
    FCFW_SKSEFunctions.FCFW_StartPlayback("MyMod.esp", timelineID, 1.0, false, false, false, 0.0)
EndFunction

Function Cleanup()
    ; When done with timeline, unregister it to free resources
    FCFW_SKSEFunctions.FCFW_UnregisterTimeline("MyMod.esp", timelineID)
EndFunction

Event OnPlaybackStart(int timelineID)
    Debug.Notification("Timeline " + timelineID + " started")
EndEvent

Event OnPlaybackStop(int timelineID)
    Debug.Notification("Timeline " + timelineID + " stopped")
EndEvent

Event OnPlaybackWait(int timelineID)
    Debug.Notification("Timeline " + timelineID + " completed (kWait mode)")
    ; Timeline is now waiting at final position - call FCFW_StopPlayback when ready
EndEvent
```

---

### **2.3 Mod API (`ModAPI.h`, `FCFW_API.h`)**
**Purpose:** Binary interface for other SKSE plugins to call FCFW functions

**Access Pattern:**
```cpp
auto api = reinterpret_cast<FCFW_API::IVFCFW1*>(
    RequestPluginAPI(FCFW_API::InterfaceVersion::V1)
);
```

**SKSE Messaging Interface:**
C++ plugins can receive timeline playback events via the SKSE messaging system:

**Event Types (FCFW_API.h):**
```cpp
enum class FCFWMessage : uint32_t {
    kPlaybackStart = 0,
    kPlaybackStop = 1,
    kPlaybackWait = 2  // kWait mode: reached end, staying at final position
};

struct FCFWTimelineEventData {
    size_t timelineID;
};
```

**Example Usage:**
```cpp
void MessageHandler(SKSE::MessagingInterface::Message* msg) {
    if (msg->sender && strcmp(msg->sender, FCFW_API::FCFWPluginName) == 0) {
        switch (msg->type) {
            case static_cast<uint32_t>(FCFW_API::FCFWMessage::kPlaybackStart): {
                auto* data = static_cast<FCFW_API::FCFWTimelineEventData*>(msg->data);
                // Handle timeline data->timelineID started
                break;
            }
            case static_cast<uint32_t>(FCFW_API::FCFWMessage::kPlaybackStop): {
                auto* data = static_cast<FCFW_API::FCFWTimelineEventData*>(msg->data);
                // Handle timeline data->timelineID stopped
                break;
            }
        }
    }
}

// In SKSEPlugin_Load():
SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
```

**Interface: `IVFCFW1`** (pure virtual, defined in `FCFW_API.h`)
- **Thread Safety:** `GetFCFWThreadId()` returns TID for thread validation
- **Version Check:** `GetFCFWPluginVersion()` returns encoded version
- **Timeline Management:**
  - `RegisterTimeline(pluginHandle)` - Returns new timeline ID (size_t, >0) for your plugin
  - `UnregisterTimeline(timelineID, pluginHandle)` - Frees timeline resources (stops active operations first)
- **Multi-Timeline API:** All functions require `size_t timelineID` and `SKSE::PluginHandle` parameters
  - External plugins pass their own plugin handle (via `SKSE::GetPluginHandle()` in their code)
  - Timeline manipulation requires ownership validation (Register/Unregister/Add/Remove/Clear/SetPlaybackMode)
- **Point Addition Functions:** Accept point structs directly (C++ API only)
  - `AddTranslationPoint(pluginHandle, timelineID, time, const RE::NiPoint3& position, ...)` - Pass position as struct
  - `AddRotationPoint(pluginHandle, timelineID, time, const RE::BSTPoint2<float>& rotation, ...)` - Pass rotation as struct (x=pitch, y=yaw)
  - Note: Papyrus API continues to accept individual coordinates (posX/Y/Z or pitch/yaw) for script convenience
- **Point Query Functions:** Returns point data directly with zero sentinel values on error (matches Papyrus pattern)
  - `RE::NiPoint3 GetTranslationPoint(pluginHandle, timelineID, index)` - Returns (0,0,0) on error
  - `RE::BSTPoint2<float> GetRotationPoint(pluginHandle, timelineID, index)` - Returns (0,0) on error
  - Consistent with Papyrus API: error conditions return zero values, check `GetTranslationPointCount()` first to validate index
- **Signatures:** Similar to Papyrus but with native C++ types (`size_t`, `SKSE::PluginHandle` instead of `string`, `int`)
  - All functions marked `const noexcept`
  - `SetPlaybackMode(pluginHandle, timelineID, playbackMode, loopTimeOffset = 0.0f)` - playbackMode as int (0=kEnd, 1=kLoop, 2=kWait), loopTimeOffset defaults to 0.0f
  - `int interpolationMode` converted via `ToInterpolationMode()` at API boundary
  - All functions delegate to `TimelineManager::GetSingleton()`

**Example: Building a Camera Path (C++ API):**
```cpp
auto api = reinterpret_cast<FCFW_API::IVFCFW1*>(
    FCFW_API::RequestPluginAPI(FCFW_API::InterfaceVersion::V1)
);

size_t timelineID = api->RegisterTimeline(SKSE::GetPluginHandle());

// Add translation points using RE::NiPoint3
RE::NiPoint3 pos1(100.0f, 200.0f, 300.0f);
RE::NiPoint3 pos2(400.0f, 500.0f, 600.0f);
api->AddTranslationPoint(SKSE::GetPluginHandle(), timelineID, 0.0f, pos1, false, false, 2);
api->AddTranslationPoint(SKSE::GetPluginHandle(), timelineID, 5.0f, pos2, false, false, 2);

// Add rotation points using RE::BSTPoint2<float> (x=pitch, y=yaw in radians)
RE::BSTPoint2<float> rot1(0.0f, 1.57f);  // Looking east
RE::BSTPoint2<float> rot2(0.5f, 3.14f);  // Looking south with slight pitch
api->AddRotationPoint(SKSE::GetPluginHandle(), timelineID, 0.0f, rot1, false, false, 2);
api->AddRotationPoint(SKSE::GetPluginHandle(), timelineID, 5.0f, rot2, false, false, 2);

// Add reference-based point with offset struct
RE::TESObjectREFR* target = RE::TESForm::LookupByID<RE::TESObjectREFR>(0x14);
RE::NiPoint3 offset(50.0f, 0.0f, 100.0f);  // 50 units right, 100 units up
api->AddTranslationPointAtRef(SKSE::GetPluginHandle(), timelineID, 10.0f, target, offset, false, false, false, 2);

// Add rotation point looking at reference with pitch/yaw offset
RE::BSTPoint2<float> rotOffset(0.1f, 0.0f);  // Slight pitch up, looking directly at target
api->AddRotationPointAtRef(SKSE::GetPluginHandle(), timelineID, 10.0f, target, rotOffset, false, false, false, 2);

// Start playback
api->StartPlayback(SKSE::GetPluginHandle(), timelineID, 1.0f);
```

**Implementation: `FCFWInterface`** (in `ModAPI.h`, implemented in `Messaging` namespace)
- Singleton pattern: `GetSingleton()` returns static instance
- Private constructor/destructor
- Stores `apiTID` member for thread tracking
- All methods marked `const noexcept override`

---

### **2.5 Hooks (`Hooks.h/cpp`)**
**Purpose:** Intercept Skyrim game loop and input handling

**Installed Hooks:**
1. **MainUpdateHook** (RELOCATION_ID 35565/36564)
   - Calls `TimelineManager::Update()` every frame
   - Uses trampoline pattern with `write_call<5>()`

2. **LookHook** (VTable hooks on `RE::LookHandler`)
   - **ProcessThumbstick** (vfunc 0x2): Gamepad camera rotation
   - **ProcessMouseMove** (vfunc 0x3): Mouse camera rotation
   - **Multi-Timeline Behavior:**
     * Gets active timeline ID: `activeID = GetActiveTimelineID()`
     * Blocks input ONLY if: `activeID != 0 && IsPlaybackRunning(activeID) && !IsUserRotationAllowed(activeID)`
     * Uses overloaded methods WITHOUT pluginHandle (checks ANY active timeline, not just FCFW-owned)
     * Critical: Must work for timelines owned by external plugins (e.g., plugin A plays timeline, hooks must block input)
   - Sets `SetUserTurning(true)` when input detected during playback

3. **MovementHook** (VTable hooks on `RE::MovementHandler`)
   - **ProcessThumbstick** (vfunc 0x2): Gamepad movement
   - **ProcessButton** (vfunc 0x4): WASD movement keys
   - **Multi-Timeline Behavior:**
     * Gets active timeline ID: `activeID = GetActiveTimelineID()`
     * Blocks forward/back/strafe input ONLY if: `activeID != 0 && IsPlaybackRunning(activeID)`
     * Uses overloaded method WITHOUT pluginHandle (checks ANY active timeline)
     * Critical: Allows movement during recording, blocks only during playback (regardless of timeline owner)
   - Checks against `userEvents->forward/back/strafeLeft/strafeRight`

**Hook Philosophy:**
- Preserve original behavior via `_OriginalFunction` pattern
- **Cross-Plugin Support:** Hooks use `IsPlaybackRunning(timelineID)` / `IsUserRotationAllowed(timelineID)` overloads WITHOUT pluginHandle
  - Rationale: External plugins can start playback, hooks must block input for ANY active timeline
  - Design: Overloaded methods bypass ownership validation (internal operations)
  - Example: Plugin A owns timeline ID 5 and starts playback → hooks check `IsPlaybackRunning(5)` and block input
- Only intercept when `IsPlaybackRunning(activeID)` and game not paused
- Recording vs Playback: `m_activeTimelineID` is set for BOTH, use `IsPlaybackRunning()` to distinguish
- Track user interaction state (`SetUserTurning(activeID, true)`) for recording

---

## **3. Type System & Conventions**

### **3.1 Core Enums (CameraTypes.h)**

**InterpolationMode:**
```cpp
enum class InterpolationMode {
    kNone = 0,
    kLinear = 1,
    kCubicHermite = 2
};
```
- **Converter Function:** `ToInterpolationMode(int)` in `CameraTypes.h`
- **Valid Values:** 0-2 (validator bounds check)
- **Usage:** API layers pass `int`, convert before calling TimelineManager

**PointType:**
```cpp
enum struct PointType {
    kWorld = 0,      // Static world point
    kReference = 1,  // Dynamic reference-based point
    kCamera = 2      // Static camera-based point (initialized at StartPlayback)
};
```
- **Converter Function:** `ToPointType(int)` in `CameraTypes.h`
- **Usage:** Determines how points calculate their world position during playback

**PlaybackMode:**
```cpp
enum class PlaybackMode : int {
    kEnd = 0,   // Stop at end of timeline (default)
    kLoop = 1,  // Restart from beginning when timeline completes
    kWait = 2   // Stay at final position indefinitely (requires manual StopPlayback)
};
```
- **Stored in INI:** Saved/loaded with timeline files
- **Applied to both tracks:** Timeline class synchronizes mode across translation and rotation
- **kWait Behavior:** Timeline reaches end, dispatches kPlaybackWait event, then continues playing at final frame position (allowing dynamic reference tracking). User must call StopPlayback() to end playback.

### **3.2 Naming Conventions**
    - **Member Variables:** `m_` prefix (e.g., `m_translationTrack`, `m_isPlaying`)
- **Function Parameters:** `a_` prefix (e.g., `a_time`, `a_reference`, `a_point`)
- **Logger:** Uses `log::info`, `log::warn`, `log::error` (CommonLib pattern)
- **Error Handling:** Return error codes (negative `int`) or `false`, log errors to file

### **3.3 Singleton Pattern**
- **TimelineManager**, **FCFWInterface**
- All use `GetSingleton()` returning static instance reference

---

## **4. Dependencies & Build System**

### **4.1 External Libraries (vcpkg)**
- **CommonLibSSE-NG**: SKSE plugin framework
- **spdlog**: Logging
- **fmt**: String formatting
- **simpleini**: INI file parsing
- **rapidcsv**: CSV parsing (unused?)
- **DirectXMath**, **DirectXTK**: Math utilities

### **4.2 Build Configuration**
- **CMake + Ninja**: Build system
- **MSVC compiler**: cl.exe
- **Precompiled Header:** `PCH.h`
- **Output:** `.dll` plugin loaded by SKSE

---

## **5. Core Architecture: TimelineManager & Timeline System**

### **5.1 TimelineManager Overview**
**Role:** Orchestrates multi-timeline recording, playback, and timeline manipulation  
**Pattern:** Singleton with map-based timeline storage (unlimited independent timelines)

**Member Variables:**
```cpp
// Multi-Timeline Storage (Phase 4 Complete)
std::unordered_map<size_t, TimelineState> m_timelines;  // Timeline ID → state
std::atomic<size_t> m_nextTimelineID{ 1 };              // Auto-incrementing ID generator
size_t m_activeTimelineID{ 0 };                         // Currently active timeline (0 = none)
mutable std::recursive_mutex m_timelineMutex;           // Thread-safe access (recursive for reentrant safety)

// Recording (shared across all timelines)
float m_recordingInterval{ 1.0f };                      // Sample rate (1 point per second)

// Playback (global state shared across all timelines)
bool m_isShowingMenus{ true };                          // Pre-playback UI state (captured before starting playback)
bool m_userTurning{ false };                            // User camera control flag
RE::NiPoint2 m_lastFreeRotation;                        // Third-person camera rotation snapshot

// Event System (global)
std::vector<RE::TESForm*> m_eventReceivers;             // Forms registered for Papyrus events
```

**TimelineState Structure** (per-timeline storage):
```cpp
struct TimelineState {
    // ===== IDENTITY & OWNERSHIP (immutable after creation) =====
    size_t m_id;                           // Timeline unique identifier
    SKSE::PluginHandle m_ownerHandle;      // Plugin that owns this timeline
    std::string m_ownerName;               // Plugin name (for logging)
    
    // ===== TIMELINE DATA & STATIC CONFIGURATION (persisted in YAML) =====
    Timeline m_timeline;                   // Paired translation + rotation tracks
    bool m_globalEaseIn{ false };          // Apply easing to timeline start (user preference)
    bool m_globalEaseOut{ false };         // Apply easing to timeline end (user preference)
    bool m_showMenusDuringPlayback{ false }; // UI visibility during playback (user preference)
    bool m_allowUserRotation{ false };     // Allow user camera control during playback (user preference)
    
    // ===== RECORDING STATE (runtime, reset on StopRecording) =====
    bool m_isRecording{ false };           // Currently capturing camera
    float m_currentRecordingTime{ 0.0f };  // Elapsed time during recording
    float m_lastRecordedPointTime{ 0.0f }; // Last sample timestamp
    
    // ===== PLAYBACK STATE (runtime, reset on StopPlayback) =====
    bool m_isPlaybackRunning{ false };     // Active playback
    float m_playbackSpeed{ 1.0f };         // Computed time multiplier (runtime only, NOT persisted)
    float m_playbackDuration{ 0.0f };      // Computed total duration (runtime only, NOT persisted)
    bool m_isCompletedAndWaiting{ false }; // kWait mode completion flag (runtime only)
    RE::BSTPoint2<float> m_rotationOffset{ 0.0f, 0.0f }; // Accumulated user rotation (runtime only)
};
```

**State Organization Notes:**
- **Identity & Ownership:** Set once at RegisterTimeline(), never changes
- **Static Configuration:** User preferences persisted in YAML files (exported/imported)
- **Recording State:** Runtime variables, cleared when StopRecording() is called
- **Playback State:** Runtime variables, cleared when StopPlayback() is called
  - `m_playbackSpeed` and `m_playbackDuration` are **computed dynamically** in StartPlayback() based on parameters, NOT persisted

**Architecture Changes:**
- **Before Phase 4:** Single `Timeline m_timeline`, all state in TimelineManager
- **After Phase 4 (✅ Complete - January 2, 2026):** Map of `TimelineState` objects, each containing timeline + per-timeline state
- **Old Code Removed:** All single-timeline functions and obsolete member variables cleaned up
- **Ownership:** Every timeline has `ownerPluginHandle`, validated on all operations
- **Active Timeline:** Only ONE timeline can be recording/playing at a time (enforced by `m_activeTimelineID`)
- **Helper Pattern:** All public API functions call `GetTimeline(timelineID, pluginHandle)` to validate ownership + existence before operations
- **Internal Query Methods (✅ Complete - January 5, 2026):** Overloaded methods for internal FCFW operations
  - `IsPlaybackRunning(size_t timelineID)` - No ownership check, used by hooks to check ANY active timeline
  - `IsUserRotationAllowed(size_t timelineID)` - No ownership check, used by hooks for input blocking
  - External API variants still require pluginHandle: `IsPlaybackRunning(SKSE::PluginHandle, size_t)`
  - Rationale: Hooks must block input during playback regardless of which plugin owns the timeline
- **Event System (✅ Complete - January 2, 2026):** Dual notification system for timeline playback events
  - SKSE Messaging: Broadcasts to all C++ plugins via `SKSE::GetMessagingInterface()->Dispatch()`
  - Papyrus Events: Queues events to registered forms via `SKSE::GetTaskInterface()->AddTask()`
- **Global vs Per-Timeline State:**
  - **Global (shared):** `m_recordingInterval`, `m_isShowingMenus`, `m_userTurning`, `m_lastFreeRotation`, `m_eventReceivers`
  - **Per-Timeline (in TimelineState):**
    - **Static config (persisted):** `m_globalEaseIn`, `m_globalEaseOut`, `m_showMenusDuringPlayback`, `m_allowUserRotation`
    - **Runtime only (NOT persisted):** `m_isRecording`, `m_currentRecordingTime`, `m_lastRecordedPointTime`, `m_isPlaybackRunning`, `m_playbackSpeed`, `m_playbackDuration`, `m_isCompletedAndWaiting`, `m_rotationOffset`
  - **Note:** `m_playbackSpeed` and `m_playbackDuration` are **computed at runtime** in `StartPlayback()` based on function parameters, and are intentionally NOT persisted to YAML files.

---

### **5.2 Update Loop (Frame-by-Frame)**
**Called by:** `MainUpdateHook::Nullsub()` every frame

```cpp
void TimelineManager::Update() {
    size_t activeID = m_activeTimelineID;
    if (activeID == 0) return;  // No active timeline
    
    TimelineState* state = GetTimeline(activeID);  // Internal overload (no ownership check)
    if (!state) return;
    
    if (UI->GameIsPaused()) {
        if (state->isPlaybackRunning) ui->ShowMenus(state->isShowingMenus);
        return;
    } else if (state->isPlaybackRunning) {
        ui->ShowMenus(state->showMenusDuringPlayback);
    }
    
    PlayTimeline(state);     // Update camera from interpolated points
    RecordTimeline(state);   // Sample camera position if recording
}
```

**Multi-Timeline Changes:**
- Gets active timeline ID from `m_activeTimelineID` (atomic read)
- Fetches `TimelineState*` via direct map access (bypasses ownership check for internal Update loop)
- Passes `TimelineState*` to all helper functions
- Only ONE timeline can be active (recording or playing) at a time

**Critical Insight:** All operations coexist in the loop, but guards prevent conflicts:
- `PlayTimeline(state)` checks `state->isPlaybackRunning && !state->isRecording`
- `RecordTimeline(state)` checks `state->isRecording`

---

### **5.3 Recording System**

#### **Recording Lifecycle:**
```
StartRecording(timelineID, pluginHandle)
├─> Validate ownership: GetTimeline(timelineID, pluginHandle)
├─> Validate: Free camera mode, not already recording/playing (exclusive check)
├─> ClearTimeline(timelineID, pluginHandle, notify=false)
├─> Capture initial point (easeIn=true)
├─> Set state->isRecording = true
└─> Set m_activeTimelineID = timelineID

RecordTimeline(TimelineState* state) [called every frame]
├─> Check: state->isRecording (early exit if false)
├─> Update state->currentRecordingTime += deltaTime
├─> If (time - lastSample >= m_recordingInterval):
│   ├─> GetCameraPos/Rotation (from _ts_SKSEFunctions)
│   ├─> AddTranslationPoint(timelineID, pluginHandle, kWorld, pos)
│   ├─> AddRotationPoint(timelineID, pluginHandle, kWorld, rotation)
│   └─> Update state->lastRecordedPointTime
└─> If not in free camera: auto-call StopRecording(timelineID, pluginHandle)

StopRecording(timelineID, pluginHandle)
├─> Validate ownership: GetTimeline(timelineID, pluginHandle)
├─> Capture final point (easeOut=true)
├─> Set state->isRecording = false
└─> Set m_activeTimelineID = 0
```

**Multi-Timeline Changes:**
- All functions require `timelineID` + `pluginHandle` parameters
- Ownership validated via `GetTimeline(timelineID, pluginHandle)` on public API calls
- RecordTimeline() helper receives `TimelineState*` (pre-validated)
- Exclusive enforcement: Only ONE timeline can record at a time (`m_activeTimelineID`)

**Key Characteristics:**
- **PointType:** Always creates `kWorld` points (static coordinates)
- **Sampling:** Fixed 1-second intervals (`m_recordingInterval`, shared across all timelines)
- **Interpolation:** All recorded points use `kCubicHermite` mode
- **Position:** `_ts_SKSEFunctions::GetCameraPos()` returns current camera world position
- **Rotation:** `_ts_SKSEFunctions::GetCameraRotation()` returns pitch (x) and yaw (z)
- **Auto-Stop:** Exits free camera → terminates recording

**Camera Utility Functions:**
The following Papyrus API functions provide convenient access to current camera state:
- `FCFW_GetCameraPosX/Y/Z()` - Returns individual X/Y/Z components of camera world position
- `FCFW_GetCameraPitch/Yaw()` - Returns individual pitch/yaw components in radians
- These are simple wrappers around `_ts_SKSEFunctions::GetCameraPos()` and `GetCameraRotation()`
- Useful for: dynamic timeline generation, debugging, conditional logic based on camera state
- No parameters required, no ownership validation (reads global camera state)

---

### **5.4 Playback System**

#### **Playback Lifecycle:**
```
StartPlayback(timelineID, pluginHandle, speed, globalEaseIn, globalEaseOut, useDuration, duration)
├─> Validate ownership: GetTimeline(timelineID, pluginHandle)
├─> Validate: ≥1 point, not in free camera, not recording/playing, duration > 0
├─> Calculate playback speed:
│   ├─> useDuration=true: state->playbackSpeed = timelineDuration / duration
│   └─> useDuration=false: state->playbackSpeed = speed parameter
├─> Save pre-playback state:
│   ├─> state->isShowingMenus = ui->IsShowingMenus()
│   └─> state->lastFreeRotation = ThirdPersonState->freeRotation
├─> Reset timelines: state->timeline.ResetTimeline(), UpdateCameraPoints(state)
├─> Enter free camera mode: ToggleFreeCameraMode(false)
├─> Dispatch timeline started events:
│   ├─> DispatchTimelineEvent(kPlaybackStart, timelineID)  [SKSE messaging]
│   └─> DispatchTimelineEventPapyrus("OnPlaybackStart", timelineID)  [Papyrus events]
├─> Set state->isPlaybackRunning = true
└─> Set m_activeTimelineID = timelineID

PlayTimeline(TimelineState* state) [called every frame]
├─> Validate: state->isPlaybackRunning, not recording, free camera active, points exist
├─> Update timeline playback time:
│   ├─> deltaTime = GetRealTimeDeltaTime() * state->playbackSpeed
│   ├─> state->timeline.m_translationTrack.UpdateTimeline(deltaTime)
│   └─> state->timeline.m_rotationTrack.UpdateTimeline(deltaTime)
├─> Apply global easing (if enabled):
│   ├─> linearProgress = playbackTime / timelineDuration
│   ├─> easedProgress = ApplyEasing(linearProgress, state->globalEaseIn, state->globalEaseOut)
│   └─> sampleTime = easedProgress * timelineDuration
├─> Sample interpolated points:
│   ├─> position = state->timeline.m_translationTrack.GetPointAtTime(sampleTime)
│   └─> rotation = state->timeline.m_rotationTrack.GetPointAtTime(sampleTime)
├─> Handle user rotation:
│   ├─> IF (state->userTurning && state->allowUserRotation):
│   │   ├─> Update state->rotationOffset = current - timeline
│   │   ├─> Reset state->userTurning flag
│   │   └─> Don't override camera (user controls rotation)
│   └─> ELSE: Apply rotation = timeline + state->rotationOffset
├─> Write to FreeCameraState: translation, rotation.x (pitch), rotation.y (yaw)
└─> Check completion: if both tracks stopped → StopPlayback(timelineID, pluginHandle)

StopPlayback(timelineID, pluginHandle)
├─> Validate ownership: GetTimeline(timelineID, pluginHandle)
├─> Dispatch timeline stopped events:
│   ├─> DispatchTimelineEvent(kPlaybackStop, timelineID)  [SKSE messaging]
│   └─> DispatchTimelineEventPapyrus("OnPlaybackStop", timelineID)  [Papyrus events]
├─> If in free camera:
│   ├─> Exit free camera mode
│   ├─> Restore state->isShowingMenus
│   └─> Restore state->lastFreeRotation (for third-person)
├─> Set state->isPlaybackRunning = false
└─> Set m_activeTimelineID = 0

SetPlaybackMode(pluginHandle, timelineID, playbackMode, loopTimeOffset = 0.0f)
├─> Validate ownership: GetTimeline(timelineID, pluginHandle)
├─> Validate playback mode: 0 (kEnd), 1 (kLoop), or 2 (kWait)
├─> Cast int to PlaybackMode enum
└─> Call state->timeline.SetPlaybackMode(mode)
    └─> Sets mode on both translation and rotation tracks
└─> Call state->timeline.SetLoopTimeOffset(loopTimeOffset)
    └─> Sets loop time offset for timeline

SwitchPlayback(fromTimelineID, toTimelineID, pluginHandle)
├─> Validate target timeline exists and is owned by caller
├─> Find source timeline:
│   ├─> If fromTimelineID == 0: search for any owned timeline that is actively playing
│   └─> Else: validate specific source timeline is actively playing
├─> Validate target timeline has points
├─> Validate camera is in free camera mode
├─> Stop source timeline WITHOUT exiting free camera:
│   ├─> Set fromState->isPlaybackRunning = false
│   ├─> Clear m_activeTimelineID temporarily
│   ├─> Dispatch kPlaybackStop for source timeline
│   └─> Dispatch "OnPlaybackStop" Papyrus event
├─> Initialize target timeline (camera already in free mode):
│   ├─> toState->timeline.ResetPlayback()
│   └─> toState->timeline.StartPlayback() (bakes kCamera points internally)
├─> Copy playback settings from source to target:
│   ├─> playbackSpeed
│   ├─> rotationOffset (conditional: only if target allows user rotation, otherwise reset to zero)
│   ├─> showMenusDuringPlayback
│   ├─> globalEaseIn, globalEaseOut
│   └─> Reset isCompletedAndWaiting flag
│   Note: allowUserRotation uses target timeline's existing setting (not copied)
├─> Activate target timeline:
│   ├─> Set m_activeTimelineID = toTimelineID
│   ├─> Set toState->isPlaybackRunning = true
│   ├─> Dispatch kPlaybackStart for target timeline
│   └─> Dispatch "OnPlaybackStart" Papyrus event
└─> Return true (no camera mode toggle, glitch-free transition)
```

**Multi-Timeline Changes:**
- All functions require `timelineID` + `pluginHandle` parameters
- Ownership validated via `GetTimeline(timelineID, pluginHandle)` on public API calls
- PlayTimeline() helper receives `TimelineState*` (pre-validated)
- Exclusive enforcement: Only ONE timeline can play at a time (`m_activeTimelineID`)
- All state variables (playbackSpeed, userTurning, etc.) now stored per-timeline in `TimelineState`
```

**Critical Behaviors:**
- **Event Dispatching:**
  - **SKSE Messaging (C++ Plugins):**
    * `DispatchTimelineEvent(messageType, timelineID)` broadcasts via `SKSE::GetMessagingInterface()->Dispatch()`
    * Message types: `kPlaybackStart` (0), `kPlaybackStop` (1)
    * Data: `FCFWTimelineEventData{ size_t timelineID }`
    * Sender: `FCFW_API::FCFWPluginName` ("FreeCameraFramework")
  - **Papyrus Events (Scripts):**
    * `DispatchTimelineEventPapyrus(eventName, timelineID)` queues to Papyrus thread
    * Thread Safety: Uses `SKSE::GetTaskInterface()->AddTask()` with lambda
    * Event names: "OnPlaybackStart", "OnPlaybackStop"
    * Dispatched to all registered forms in `m_eventReceivers` vector
    * VM access: Queued lambda gets handle, creates args, calls `vm->SendEvent()`

- **User Rotation Control (Per-Timeline):**
  - `state->userTurning` flag set by `LookHook` when user moves mouse/thumbstick
  - `state->allowUserRotation` enables accumulated offset mode
  - `state->rotationOffset` persists across frames (allows looking around during playback)
  - Offset calculation: `NormalRelativeAngle(current - timeline)`
  - Hook calls: `SetUserTurning(timelineID, true)` / `AllowUserRotation(timelineID, bool)`

- **Global Easing:**
  - Applied to **sample time**, not speed
  - Affects entire timeline progress curve
  - Independent of per-point easing
  - Per-timeline: `state->globalEaseIn`, `state->globalEaseOut`

- **Camera State:**
  - Playback forces `kFree` camera state
  - Directly writes `FreeCameraState->translation` and `->rotation`
  - Movement/look hooks block user input ONLY during playback (not recording)
  - Hooks check: `IsPlaybackRunning(activeID)` to distinguish from recording

- **PlaybackMode Behaviors:**
  - **kEnd (0):** Timeline reaches end → dispatches kPlaybackStop → stops playback automatically
  - **kLoop (1):** Timeline reaches end → wraps to beginning seamlessly using modulo time
  - **kWait (2):** Timeline reaches end → dispatches kPlaybackWait event → continues playing at final frame
    * Stays in playback state (`IsPlaybackRunning() == true`)
    * Camera updates every frame at final position (allows dynamic reference tracking)
    * User must manually call `StopPlayback()` to end
    * Event dispatched only once (tracked via `m_isCompletedAndWaiting` flag)
    * Use case: Hold camera on moving NPC, wait for player trigger, cinematic "freeze" effects

---

### **5.5 Timeline & TimelineTrack Architecture**

#### **Timeline Class (Non-Template Wrapper)**
**Purpose:** High-level coordinator for paired translation + rotation tracks  
**Location:** `Timeline.h` (declaration), `Timeline.cpp` (implementation)

**Member Variables:**
```cpp
TranslationTrack m_translationTrack;  // Position keyframes
RotationTrack m_rotationTrack;        // Rotation keyframes
```

**Note:** Timeline ID is managed externally by `TimelineState::m_id` (size_t), not stored in Timeline class. Playback settings (`m_playbackSpeed`, `m_globalEaseIn`, `m_globalEaseOut`) are also managed in `TimelineState`, not in Timeline.

**Type Aliases:**
```cpp
using TranslationTrack = TimelineTrack<TranslationPath>;
using RotationTrack = TimelineTrack<RotationPath>;
```

**Public API - Complete Encapsulation:**

Timeline now provides complete encapsulation - TimelineManager never directly accesses TimelineTrack. All operations go through Timeline's public API.

**Point Management:**
```cpp
size_t AddTranslationPoint(const TranslationPoint& a_point);  // Returns new point count
size_t AddRotationPoint(const RotationPoint& a_point);        // Returns new point count
void RemoveTranslationPoint(size_t a_index);
void RemoveRotationPoint(size_t a_index);
void ClearPoints();  // Clears both tracks
```

**Coordinated Playback:**
```cpp
void UpdatePlayback(float a_deltaTime);  // Updates both tracks in sync
void StartPlayback();      // Bakes kCamera points in both tracks
void ResetPlayback();      // Resets both tracks to time 0
void PausePlayback();
void ResumePlayback();
```

**Query Methods:**
```cpp
// State queries (OR logic: true if either track matches)
float GetPlaybackTime() const;  // Returns translation track playback time
bool IsPlaying() const;         // true if translation OR rotation track is playing
bool IsPaused() const;          // true if translation OR rotation track is paused

// Count and duration
size_t GetTranslationPointCount() const;
size_t GetRotationPointCount() const;
float GetDuration() const;  // max(translation.GetDuration(), rotation.GetDuration())

// Playback mode (applied to both tracks)
PlaybackMode GetPlaybackMode() const;
float GetLoopTimeOffset() const;
void SetPlaybackMode(PlaybackMode a_mode);
void SetLoopTimeOffset(float a_offset);
```

**Sampling:**
```cpp
RE::NiPoint3 GetTranslation(float a_time) const;     // Query translation at time
RE::BSTPoint2<float> GetRotation(float a_time) const; // Query rotation at time
```

**Import/Export:**
```cpp
// Camera point access (for TimelineManager::Add...PointAtCamera)
// Note: Takes parameters (time, easeIn, easeOut) - creates TransitionPoint with PointType::kCamera
TranslationPoint GetTranslationPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const;
RotationPoint GetRotationPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const;

// File I/O (delegates to Path::AddPathFromFile / Path::ExportPath)
bool AddTranslationPathFromFile(std::ifstream& a_file, float a_timeOffset = 0.0f, float a_conversionFactor = 1.0f);
bool AddRotationPathFromFile(std::ifstream& a_file, float a_timeOffset = 0.0f, float a_conversionFactor = 1.0f);
bool ExportTranslationPath(std::ofstream& a_file, float a_conversionFactor = 1.0f) const;
bool ExportRotationPath(std::ofstream& a_file, float a_conversionFactor = 1.0f) const;
```

**Point Position Queries:**
```cpp
RE::NiPoint3 GetTranslationPoint(size_t a_index) const;        // Get world position of translation point
RE::BSTPoint2<float> GetRotationPoint(size_t a_index) const;   // Get rotation (pitch, yaw in radians)
```

**Private Track Access:**
```cpp
// These are now private - only accessible within Timeline.cpp implementations
TranslationTrack& GetTranslationTrack();
RotationTrack& GetRotationTrack();
const TranslationTrack& GetTranslationTrack() const;
const RotationTrack& GetRotationTrack() const;
```

**Architectural Benefits:**
1. **Complete Encapsulation:** TimelineManager has no direct access to TimelineTrack internals
2. **Unified Interface:** All operations go through Timeline public API
3. **OR Logic:** IsPlaying()/IsPaused() check both tracks (returns true if either matches)
4. **Future-Proof:** Track implementation can change without affecting TimelineManager
5. **Single Point of Access:** All track coordination logic contained in Timeline class

**Key Insight:** Timeline class enforces paired track coordination while preserving track independence (different point counts/times allowed). TimelineTrack is now a pure implementation detail.

---

#### **TimelineTrack<PathType> Template (Low-Level Engine)**
**Purpose:** Generic interpolation and playback engine for any path type  
**Location:** `TimelineTrack.h` (single-file template: declaration + implementation)  
**Access:** Private to Timeline class only (not exposed to TimelineManager)

**Template Specializations:**
```cpp
TimelineTrack<TranslationPath>  // Position timeline (RE::NiPoint3 values)
TimelineTrack<RotationPath>     // Rotation timeline (RE::BSTPoint2<float> values)
```

**Member Variables:**
```cpp
PathType m_path;                          // CameraPath<TransitionPoint> - stores ordered points (PRIVATE ACCESS)
float m_playbackTime{ 0.0f };             // Current position in timeline (seconds)
bool m_isPlaying{ false };                // Playback active
bool m_isPaused{ false };                 // Playback paused
PlaybackMode m_playbackMode{ PlaybackMode::kEnd };  // kEnd (stop) or kLoop (wrap)
float m_loopTimeOffset{ 0.0f };           // Extra time for loop interpolation (last→first)
```

**Type Traits (provided by PathType):**
```cpp
PathType::TransitionPoint  // TranslationPoint or RotationPoint
PathType::ValueType        // RE::NiPoint3 or RE::BSTPoint2<float>
```

**Encapsulation Architecture:**

TimelineTrack now provides complete path encapsulation through wrapper methods. The `GetPath()` accessor is private (internal use only).

**Public Path Operations (wrapper methods):**
```cpp
// Sampling
TransitionPoint GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const;  // Delegate to m_path
const TransitionPoint& GetPoint(size_t a_index) const;                                // Delegate to m_path

// File I/O
bool AddPathFromFile(std::ifstream& a_file, float a_timeOffset, float a_conversionFactor);  // Delegate to m_path
bool ExportPath(std::ofstream& a_file, float a_conversionFactor) const;                    // Delegate to m_path
```

**Private Path Access (internal only):**
```cpp
PathType& GetPath() { return m_path; }              // For internal modifications
const PathType& GetPath() const { return m_path; }  // For internal const access
```

**Three-Tier Encapsulation:**
1. **Timeline** (public API) → hides TimelineTrack implementation
2. **TimelineTrack** (engine) → hides Path implementation (via wrapper methods)
3. **Path** (storage) → fully encapsulated, only accessible through TimelineTrack wrappers

**Core Operations:**

| Method | Behavior |
|--------|----------|
| `AddPoint(point)` | Insert sorted by time, calls `ResetTimeline()` |
| `UpdateTimeline(deltaTime)` | Advance `m_playbackTime`, handle loop wrap, end, or wait based on `m_playbackMode` |
| `GetPointAtTime(time)` | Calculate segment index + progress, interpolate (returns ValueType) |
| `StartPlayback()` | Call `UpdateCameraPoints()` (bake kCamera), set playing |
| `ResetTimeline()` | Zero playback time, clear playing/paused flags |
| `GetPlaybackTime()` | Returns current playback position |

**UpdateTimeline PlaybackMode Logic:**
```cpp
if (m_playbackTime >= timelineDuration) {
    if (m_playbackMode == PlaybackMode::kLoop) {
        m_playbackTime = std::fmod(m_playbackTime, timelineDuration);  // Wrap time
    } else if (m_playbackMode == PlaybackMode::kWait) {
        m_playbackTime = timelineDuration;  // Clamp to final frame
        // m_isPlaying stays true - continues updating at final position
    } else {  // kEnd
        m_playbackTime = timelineDuration;  // Clamp to final frame
        m_isPlaying = false;  // Stop playback
    }
}
```

**Interpolation Dispatch:**
```cpp
ValueType GetInterpolatedPoint(index, progress) {
    switch (point.m_transition.m_mode) {
        case kNone: return currentPoint.m_point;
        case kLinear: return GetPointLinear(index, progress);
        case kCubicHermite: return GetPointCubicHermite(index, progress);
    }
}
```

**Cubic Hermite Logic:**
- **Tangent Calculation:** Uses 4-point neighborhood (p0, p1, p2, p3)
- **Loop Mode:** Modulo wrapping for neighbor indices
- **End Mode:** Clamp neighbors at boundaries
- **Progress:** Apply per-point easing before interpolation
- **Angular (Rotation):** Uses `CubicHermiteInterpolateAngular()` for angle wrapping

---

### **5.6 CameraPath<T> Template (Storage Layer)**

**Purpose:** Point collection with type-specific I/O operations  
**Location:** `CameraPath.h` (declaration), `CameraPath.cpp` (implementation)

**Template Architecture:**
```cpp
template <typename T>
class CameraPath {
    // Friend declarations for template helper functions (allow access to protected m_points)
    template <typename PointType, typename PathType>
    friend bool ImportPathFromYAML(PathType& a_path, const YAML::Node& a_node, float a_timeOffset, float a_conversionFactor);
    
    template <typename PointType, typename PathType>
    friend bool ExportPathToYAML(const PathType& a_path, YAML::Emitter& a_out, float a_conversionFactor);

protected:
    std::vector<T> m_points;  // Ordered point storage (accessible to template friends)
};
```

**Concrete Implementations:**
```cpp
class TranslationPath : public CameraPath<TranslationPoint> {
public:
    using TransitionPoint = TranslationPoint;
    using ValueType = RE::NiPoint3;  // Type returned by GetPoint()
    
    TranslationPoint GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const override;  // Creates kCamera point
    bool AddPathFromFile(...) override;  // Delegates to ImportPathFromYAML<TranslationPoint, TranslationPath>
    bool ExportPath(...) const override;  // Delegates to ExportPathToYAML<TranslationPoint, TranslationPath>
};

class RotationPath : public CameraPath<RotationPoint> {
public:
    using TransitionPoint = RotationPoint;
    using ValueType = RE::BSTPoint2<float>;  // Type returned by GetPoint()
    
    RotationPoint GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const override;  // Creates kCamera point
    bool AddPathFromFile(...) override;  // Delegates to ImportPathFromYAML<RotationPoint, RotationPath>
    bool ExportPath(...) const override;  // Delegates to ExportPathToYAML<RotationPoint, RotationPath>
};
```

**Template-Based YAML I/O (CameraPath.cpp):**
- **PointTraits Specialization:** Type-specific metadata for template logic
  ```cpp
  template<> struct PointTraits<TranslationPoint> {
      using ValueType = RE::NiPoint3;
      static constexpr size_t Dimensions = 3;
      static constexpr const char* FieldName = "translationPoints";
      static constexpr const char* ValueKey = "position";
      // ReadValue/WriteValue methods with conversion factor support
  };
  
  template<> struct PointTraits<RotationPoint> {
      using ValueType = RE::BSTPoint2<float>;
      static constexpr size_t Dimensions = 2;
      static constexpr const char* FieldName = "rotationPoints";
      static constexpr const char* ValueKey = "rotation";
      // ReadValue/WriteValue methods with conversion factor support
  };
  ```

- **ImportPathFromYAML Template:** Generic YAML→Points import (~130 lines)
  - Handles all three point types (kWorld, kCamera, kReference)
  - Reference resolution: EditorID lookup with FormID fallback
  - Applies time offset and conversion factor
  - Uses PointTraits for type-specific behavior

- **ExportPathToYAML Template:** Generic Points→YAML export (~100 lines)
  - Writes properly formatted YAML with array notation
  - Applies conversion factor (e.g., radians→degrees for rotation)
  - Uses PointTraits for field names and dimensions

- **Code Consolidation:** Replaces ~440 lines of duplicated logic with ~238 lines of shared templates + 8 lines of wrappers (47% reduction)

**Key Insights:** 
- `ValueType` alias resolves naming conflict with `PointType` enum (kWorld/kReference/kCamera)
- `GetPointAtCamera()` is const-correct: creates new point without modifying path state
- Takes time/easing parameters, returns TransitionPoint with PointType::kCamera and transition metadata
- **Friend Declarations:** Template helper functions granted access to protected `m_points` member for direct manipulation during I/O
- **Angle Conversion:** RotationPath supports conversion via `a_conversionFactor` parameter
  - Import: `AddPathFromFile(path, offset, conversionFactor)` - multiplies YAML angles by factor
    * `conversionFactor = π/180` (≈ 0.0174533) converts degrees → radians
    * `conversionFactor = 1.0` (default) for radians (no conversion)
  - Export: `ExportPath(file, conversionFactor)` - multiplies internal angles by factor before writing
    * `conversionFactor = 180/π` (≈ 57.2958) converts radians → degrees
    * `conversionFactor = 1.0` (default) for radians (no conversion)
  - TranslationPath has no conversion (world units remain unchanged)

---

### **5.7 Point Construction Patterns**

#### **Transition Object:**
```cpp
struct Transition {
    float m_time;                // Keyframe timestamp (seconds)
    InterpolationMode m_mode;    // kNone, kLinear, kCubicHermite
    bool m_easeIn, m_easeOut;    // Apply smoothing
};
```

#### **TranslationPoint Constructor:**
```cpp
TranslationPoint(
    const Transition& a_transition,         // Time + interpolation settings
    PointType a_pointType,                  // kWorld, kReference, kCamera
    const RE::NiPoint3& a_point,            // World position (kWorld/kCamera baked)
    const RE::NiPoint3& a_offset = {},      // Offset for kReference/kCamera
    RE::TESObjectREFR* a_reference = null,  // Target ref (kReference only)
    bool a_isOffsetRelative = false         // Local space offset (kReference only)
)
```

**PointType Semantics:**

| Type | `m_point` | `m_offset` | `m_reference` | `GetPoint()` Behavior |
|------|-----------|------------|---------------|----------------------|
| **kWorld** | Static position | Unused | null | Returns `m_point` directly |
| **kReference** | Unused (mutable cache) | World/local offset | Required | `ref->GetPosition() + (rotated offset if relative)` |
| **kCamera** | Baked at StartPlayback | Initial offset | null | Returns baked `m_point` |

**Critical: kCamera Baking**
- Created with `PointType::kCamera` and `m_offset`
- `GetPointAtCamera()` calculates: `GetCameraPos() + m_offset`
- `UpdateCameraPoints()` calls `GetPointAtCamera()` → stores result in `m_point`
- After baking, behaves like `kWorld` (static position)

#### **RotationPoint Constructor:**
```cpp
RotationPoint(
    const Transition& a_transition,
    PointType a_pointType,
    const RE::BSTPoint2<float>& a_point,    // Pitch/Yaw (kWorld/kCamera baked)
    const RE::BSTPoint2<float>& a_offset = {},
    RE::TESObjectREFR* a_reference = null,
    bool a_isOffsetRelative = false
)
```

**PointType Semantics:**

| Type | Behavior |
|------|----------|
| **kWorld** | Static pitch/yaw angles |
| **kReference** (relative=false) | Camera looks at ref, offset adjusts aim direction |
| **kReference** (relative=true) | Uses ref's heading + offset (ignores camera position) |
| **kCamera** | Baked at StartPlayback: `GetCameraRotation() + m_offset` |

**Reference Rotation Math:**
- **relative=false:** Calculate camera→ref direction, build coordinate frame, apply offset as local rotation
- **relative=true:** Use `ref->GetHeading()` or `GetAngleX/Z()`, add offset directly

---

### **5.8 Import/Export System**

#### **YAML File Format:**
FreeCameraFramework uses YAML for timeline import/export, providing human-readable, compact camera path definitions.

**File Structure:**
```yaml
# FreeCameraFramework Timeline (YAML format)
formatVersion: 1

# Metadata section
playbackMode: end  # end, loop, or wait
loopTimeOffset: 0.0
globalEaseIn: false
globalEaseOut: false
showMenusDuringPlayback: false
allowUserRotation: true
useDegrees: true  # Rotation angles in degrees (false=radians)

# Translation points
translationPoints:
  - time: 0.0
    pointType: world
    position: [100.0, 200.0, 50.0]
    interpolationMode: cubicHermite
    easeIn: true
    easeOut: false
  
  - time: 5.0
    pointType: reference
    offset: [0, 100, 50]
    reference: "MyQuestMarkerRef"
    isOffsetRelative: false
    interpolationMode: linear

# Rotation points  
rotationPoints:
  - time: 0.0
    pointType: world
    rotation: [0.0, 90.0]  # [pitch, yaw] in degrees
    interpolationMode: cubicHermite
    easeIn: true
  
  - time: 5.0
    pointType: camera
    offset: [5.7, 0.0]  # Slight upward pitch offset in degrees
    interpolationMode: linear

# Note: Points inherit hardcoded defaults (interpolationMode: cubicHermite, easeIn: false, easeOut: false)
# Each point can override these individually. No separate "defaults" section needed.
```

**Key Features:**
- **Format Versioning**: `formatVersion: 1` field enables backwards compatibility and future format evolution
  - Current version: 1 (string-based enums, array notation, load-order independence)
  - Legacy files without `formatVersion` default to version 1 for backwards compatibility
  - Unknown versions log warning and attempt version 1 parser (graceful degradation)
  - Export always writes current format version
- **String Enums**: Human-readable values (`"world"`, `"cubicHermite"`, `"end"`) instead of magic numbers
- **Array Notation**: Compact vectors `[x, y, z]` or `[pitch, yaw]` instead of separate fields
- **Load-Order Independence**: EditorID-first reference resolution with FormID fallback
- **Angle Units Control**: `useDegrees` flag determines rotation angle units
  - `useDegrees: true` (default) - Rotation angles in **degrees** for human readability
  - `useDegrees: false` - Rotation angles in **radians** (for backwards compatibility)
  - Internal engine always uses radians; conversion happens during import/export

**PointType Values:**
- `world`: Static world coordinates
- `reference`: Dynamic position/rotation relative to a reference object
- `camera`: Offset from camera position at playback start (baked once)

**InterpolationMode Values:**
- `none`: Jump to point (no interpolation)
- `linear`: Linear interpolation
- `cubicHermite`: Smooth cubic Hermite spline (default)

**PlaybackMode Values:**
- `end`: Stop at timeline end (default)
- `loop`: Wrap back to start with optional time offset
- `wait`: Stay at final position indefinitely (manual stop required)

**Translation Point Fields:**

| PointType | Required Fields | Optional Fields | Notes |
|-----------|----------------|-----------------|-------|
| **world** | `time`, `position: [x,y,z]` | `interpolationMode`, `easeIn`, `easeOut` | Static world coordinates |
| **reference** | `time`, `offset: [x,y,z]`, `reference: "EditorID"` | `isOffsetRelative`, `interpolationMode`, `easeIn`, `easeOut` | Dynamic reference tracking |
| **camera** | `time`, `offset: [x,y,z]` | `interpolationMode`, `easeIn`, `easeOut` | Baked at playback start |

**Rotation Point Fields:**

| PointType | Required Fields | Optional Fields | Notes |
|-----------|----------------|-----------------|-------|
| **world** | `time`, `rotation: [pitch,yaw]` | `interpolationMode`, `easeIn`, `easeOut` | Angles in degrees if `useDegrees: true`, radians if false |
| **reference** | `time`, `offset: [pitch,yaw]`, `reference: "EditorID"` | `isOffsetRelative`, `interpolationMode`, `easeIn`, `easeOut` | Look at reference + offset (units match `useDegrees`) |
| **camera** | `time`, `offset: [pitch,yaw]` | `interpolationMode`, `easeIn`, `easeOut` | Baked at playback start (units match `useDegrees`) |

**Import Process (TimelineManager.cpp):**
```
AddTimelineFromFile(timelineID, pluginHandle, path, timeOffset)
├─> Validate ownership: GetTimeline(timelineID, pluginHandle)
├─> Load YAML file and parse metadata
├─> Check formatVersion field (defaults to 1 if missing for legacy files)
│   └─> Log warning if version != 1, attempt version 1 parser
├─> Read useDegrees flag (default: false if missing)
├─> Calculate rotation conversion factor:
│   ├─> If useDegrees=true: conversionFactor = π/180 (degrees → radians)
│   └─> If useDegrees=false: conversionFactor = 1.0 (radians, no conversion)
├─> Import translation: state->timeline.AddTranslationPathFromFile(path, offset)
├─> Import rotation: state->timeline.AddRotationPathFromFile(path, offset, conversionFactor)
└─> Log point counts
```

**Export Process (TimelineManager.cpp):**
```
ExportTimeline(timelineID, pluginHandle, path)
├─> Validate ownership: GetTimeline(timelineID, pluginHandle)
├─> Write formatVersion: 1
├─> Write metadata section with useDegrees: true
├─> Calculate rotation conversion factor: 180/π (radians → degrees)
├─> Export translation: state->timeline.ExportTranslationPath(file)  // No conversion
├─> Export rotation: state->timeline.ExportRotationPath(file, conversionFactor)
└─> Log point counts
```

**Multi-Timeline Changes:**
- All import/export functions require `timelineID` + `pluginHandle` parameters
- Ownership validated before file operations
- Timeline ownership tracked via `state->ownerPluginHandle`

**Critical Details:**
- **Format Versioning:** `formatVersion: 1` written to all exported files
  - Import defaults to version 1 for legacy files without the field
  - Unknown versions trigger warning but attempt parsing (graceful fallback)
  - Enables future format migrations without breaking old files
- **Time Offset:** Applied to all imported point times (for timeline concatenation)
- **Angle Conversion (useDegrees Flag):**
  - **New files (Export):** Always write `formatVersion: 1`, `useDegrees: true` with angles in degrees (conversion factor = 180/π ≈ 57.2958)
  - **Import:** Read `useDegrees` flag from YAML
    * `useDegrees: true` → Apply π/180 (≈ 0.0174533) to convert degrees to radians
    * `useDegrees: false` or missing → Use 1.0 (no conversion, assume radians)
  - **Translation:** No conversion (world units, no conversion factor parameter)
  - **Backwards Compatibility:** Old files without `useDegrees` flag default to radians (no conversion)
- **Reference Resolution:** EditorID lookup first (requires po3's Tweaks), FormID fallback
- **File Extension:** `.yaml` or `.yml` recommended for clarity

---

### **5.9 Point Query API**

**Purpose:** Allow external plugins to query timeline point data for custom visualization or analysis.

**Available Functions:**
- `GetTranslationPoint(pluginHandle, timelineID, index)` - Returns `RE::NiPoint3` with position data
- `GetRotationPoint(pluginHandle, timelineID, index)` - Returns `RE::BSTPoint2<float>` with pitch/yaw data  
- Both return zero sentinels `(0,0,0)` or `(0,0)` on error
- Validate index with `GetTranslationPointCount()` / `GetRotationPointCount()` first

**Use Cases:**
- Custom 3D visualization plugins
- Path analysis tools
- Timeline debugging utilities
- Export to external formats

---

### **5.10 Event Callback System**

**Purpose:** Notify external consumers when timeline playback starts/stops

**Architecture:** Dual notification system for different consumer types:
1. **SKSE Messaging Interface** - For C++ plugins
2. **Papyrus Event System** - For scripts

#### **SKSE Messaging Interface (C++ Plugins)**

**Event Types (FCFW_API.h):**
```cpp
namespace FCFW_API {
    constexpr const auto FCFWPluginName = "FreeCameraFramework";
    
    enum class FCFWMessage : uint32_t {
        kPlaybackStart = 0,
        kPlaybackStop = 1
    };
    
    struct FCFWTimelineEventData {
        size_t timelineID;
    };
}
```

**Implementation (TimelineManager.cpp):**
```cpp
void TimelineManager::DispatchTimelineEvent(uint32_t a_messageType, size_t a_timelineID) {
    auto* messaging = SKSE::GetMessagingInterface();
    if (!messaging) return;
    
    FCFW_API::FCFWTimelineEventData eventData{ a_timelineID };
    messaging->Dispatch(
        a_messageType,
        &eventData,
        sizeof(eventData),
        FCFW_API::FCFWPluginName
    );
}
```

**Consumer Pattern:**
External plugins register a message listener in `SKSEPlugin_Load()`:
```cpp
void MessageHandler(SKSE::MessagingInterface::Message* msg) {
    if (msg->sender && strcmp(msg->sender, FCFW_API::FCFWPluginName) == 0) {
        switch (msg->type) {
            case static_cast<uint32_t>(FCFW_API::FCFWMessage::kPlaybackStart): {
                auto* data = static_cast<FCFW_API::FCFWTimelineEventData*>(msg->data);
                // Handle timeline started: data->timelineID
                break;
            }
            case static_cast<uint32_t>(FCFW_API::FCFWMessage::kPlaybackStop): {
                auto* data = static_cast<FCFW_API::FCFWTimelineEventData*>(msg->data);
                // Handle timeline stopped: data->timelineID
                break;
            }
        }
    }
}

SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
```

#### **Papyrus Event System (Scripts)**

**Registration API (TimelineManager.h):**
```cpp
void RegisterForTimelineEvents(RE::TESForm* a_form);
void UnregisterForTimelineEvents(RE::TESForm* a_form);
```

**Event Storage:**
```cpp
std::vector<RE::TESForm*> m_eventReceivers;  // Global list of registered forms
```

**Thread-Safe Event Dispatch (TimelineManager.cpp):**
```cpp
void TimelineManager::DispatchTimelineEventPapyrus(const char* a_eventName, size_t a_timelineID) {
    std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
    
    for (auto* receiver : m_eventReceivers) {
        if (!receiver) continue;
        
        auto* task = SKSE::GetTaskInterface();
        if (task) {
            task->AddTask([receiver, eventName = std::string(a_eventName), timelineID = a_timelineID]() {
                auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
                auto* policy = vm->GetObjectHandlePolicy();
                auto handle = policy->GetHandleForObject(receiver->GetFormType(), receiver);
                
                auto args = RE::MakeFunctionArguments(static_cast<std::int32_t>(timelineID));
                vm->SendEvent(handle, RE::BSFixedString(eventName), args);
            });
        }
    }
}
```

**Thread Safety Design:**
- VM methods cannot be called from arbitrary threads (cross-thread safety)
- Solution: `SKSE::GetTaskInterface()->AddTask()` queues lambda to Papyrus thread
- Lambda captures: receiver pointer, eventName as std::string copy, timelineID as size_t
- VM access only happens inside queued lambda (guaranteed Papyrus thread)

**Papyrus Bindings (plugin.cpp):**
```cpp
void RegisterForTimelineEvents(RE::StaticFunctionTag*, RE::TESForm* a_form) {
    if (!a_form) {
        log::error("{}: Null form provided", __FUNCTION__);
        return;
    }
    FCFW::TimelineManager::GetSingleton().RegisterForTimelineEvents(a_form);
}

void UnregisterForTimelineEvents(RE::StaticFunctionTag*, RE::TESForm* a_form) {
    if (!a_form) return;
    FCFW::TimelineManager::GetSingleton().UnregisterForTimelineEvents(a_form);
}

// Registration in RegisterPapyrusFunctions():
a_vm->RegisterFunction("FCFW_RegisterForTimelineEvents", "FCFW_SKSEFunctions", RegisterForTimelineEvents);
a_vm->RegisterFunction("FCFW_UnregisterForTimelineEvents", "FCFW_SKSEFunctions", UnregisterForTimelineEvents);
```

**Papyrus Consumer Pattern (FCFW_SKSEFunctions.psc):**
```papyrus
Scriptname MyQuest extends Quest

Event OnInit()
    FCFW_SKSEFunctions.FCFW_RegisterForTimelineEvents(self)
EndEvent

Event OnPlaybackStart(int timelineID)
    Debug.Notification("Timeline " + timelineID + " started playing")
EndEvent

Event OnPlaybackStop(int timelineID)
    Debug.Notification("Timeline " + timelineID + " stopped")
EndEvent
```

**Event Dispatch Locations:**
- `StartPlayback()`: Dispatches both SKSE message and Papyrus event for `kPlaybackStart`
- `StopPlayback()`: Dispatches both SKSE message and Papyrus event for `kPlaybackStop`

**Key Design Decisions:**
- **Global Event Receivers:** All registered forms receive events for ALL timelines
- **No Filtering:** Consumers receive timelineID parameter and filter themselves
- **No Ownership Required:** Event registration doesn't require timeline ownership
- **Thread Safety:** Papyrus events queued via task interface (no direct VM access)
- **Dual System:** SKSE messaging for performance, Papyrus events for script convenience

---

### **5.11 Point Modification Rules**

**Thread Safety:** 
- TimelineManager uses `std::mutex m_timelineMutex` for thread-safe timeline map access
- Atomic operations: `m_nextTimelineID`, `m_activeTimelineID`

**Playback Protection (TimelineManager.cpp):**
All modification methods check `state->isPlaybackRunning`:
```cpp
TimelineState* state = GetTimeline(timelineID, pluginHandle);
if (!state) return false;  // Ownership validation

if (state->isPlaybackRunning) {
    log::info("Timeline {} modified during playback, stopping", timelineID);
    StopPlayback(timelineID, pluginHandle);
}
```

**Protected Operations (require timelineID + pluginHandle):**
- `AddTranslationPoint(timelineID, pluginHandle, ...)` (returns `bool` - success)
- `AddRotationPoint(timelineID, pluginHandle, ...)` (returns `bool` - success)
- `RemoveTranslationPoint(timelineID, pluginHandle, index)` (returns `bool` - success)
- `RemoveRotationPoint(timelineID, pluginHandle, index)` (returns `bool` - success)
- `AddTimelineFromFile(timelineID, pluginHandle, path, offset)` (returns `bool` - success)

**Note:** `ClearTimeline(timelineID, pluginHandle)` blocks if `state->isRecording` (prevents accidental wipe)

**Multi-Timeline Changes:**
- Ownership validation via `GetTimeline(timelineID, pluginHandle)` before all operations
- Per-timeline playback checks (`state->isPlaybackRunning`)
- Return bool instead of void for error handling

---

## **6. Supporting Systems**

### **6.1 Import/Export Implementation (CameraPath.cpp)**

**Hardcoded Defaults:**
- Import functions use hardcoded defaults: `InterpolationMode::kCubicHermite`, `easeIn = false`, `easeOut = false`
- Points can override these individually in YAML (point-level values take precedence)
- No separate "defaults" section in YAML files (removed for simplicity - redundant with hardcoded values)
- Export optimization: Points only write non-default values to minimize file size

**Reference Resolution Strategy:**
1. **EditorID First** (load-order independent)
   - `RE::TESForm::LookupByEditorID<RE::TESObjectREFR>(editorID)`
   - Validates plugin name if `RefPlugin` field present
   - **Requires:** po3's Tweaks for reliable EditorID support
2. **FormID Fallback** (load-order dependent)
   - `RE::TESForm::LookupByID(formID)` from hex string
3. **Failure Handling:** Creates kWorld point using offset as absolute position/rotation

**INI Parsing:**
- Uses `ParseFCFWTimelineFileSections()` with callback pattern
- Inline comments (`;`) supported and stripped from values
- Graceful error handling: logs warnings, skips invalid entries

**Export Warnings:**
- Logs if reference has no EditorID (portability risk)
- Always writes FormID + plugin name for debugging

---

### **6.2 Timeline Architecture: Current Structure**

**File Organization:**
- **TimelineTrack.h**: Complete template implementation (declaration + all methods)
  - `TimelineTrack<PathType>` template class
  - Type aliases: `TranslationTrack`, `RotationTrack`
  - All template method implementations inline (no separate .inl file)
- **Timeline.h**: Non-template Timeline class declaration
  - Paired track coordinator (wraps TranslationTrack + RotationTrack)
  - Includes `TimelineTrack.h` for template definitions
- **Timeline.cpp**: Timeline class implementations
  - All non-template Timeline methods

**Design Rationale:**
- **Single-file templates**: TimelineTrack.h contains both declaration and implementation (no .inl split)
- **Clean separation**: Template code (TimelineTrack) in one file, non-template code (Timeline) in another
- **Automatic dependencies**: Timeline.h includes TimelineTrack.h, so users only need to include Timeline.h

**Historical Notes:**
- **Previous structure** (Timeline_old.h, now deleted):
  - Original design: Single template `Timeline<PathType>` (all-in-one)
  - Type aliases: `TranslationTimeline`, `RotationTimeline` 
  - TimelineManager stored two separate timeline instances
- **Refactoring migration**:
  - Renamed `Timeline<T>` → `TimelineTrack<T>` (emphasizes engine role)
  - Created new `Timeline` wrapper class (paired track coordination)
  - Split implementation: Timeline.inl (deleted) → merged into TimelineTrack.h
  - Benefits: Better encapsulation, clearer API, multi-timeline ready
---

### **6.3 Utility Functions (_ts_SKSEFunctions)**

**Camera Functions:**
```cpp
RE::NiPoint3 GetCameraPos()         // World position
RE::NiPoint3 GetCameraRotation()    // {pitch, yaw, roll} in radians
float GetCameraPitch/Yaw()          // Individual components
```

**Angle Utilities:**
```cpp
float NormalRelativeAngle(angle)    // Wrap to [-π, π]
float NormalAbsoluteAngle(angle)    // Wrap to [0, 2π]
```

**Timing:**
```cpp
float GetRealTimeDeltaTime()        // Frame delta (from RELOCATION_ID)
```

**Easing:**
```cpp
float ApplyEasing(t, easeIn, easeOut) {
    if (easeIn && easeOut) return SCurveFromLinear(t, 0.33, 0.66);  // S-curve
    if (easeIn) return InterpEaseIn(0, 1, t, 2.0);                   // Quadratic ease-in
    if (easeOut) return 1 - InterpEaseIn(0, 1, 1-t, 2.0);            // Quadratic ease-out
    return t;                                                         // Linear
}
```

**INI File Access:**
```cpp
T GetValueFromINI(vm, stackId, "key:section", filePath, defaultValue)
// Supports: bool, long, double, string
// Uses CSimpleIniA for parsing
// Path resolved from Data/ directory
```

---

### **6.4 Helper Functions (FCFW_Utils)**

**YAML Enum Conversion Helpers:**
```cpp
// String to Enum converters (with validation)
PointType StringToPointType(const std::string& str)
  // "world" -> kWorld, "reference" -> kReference, "camera" -> kCamera
  // Unknown values log warning and default to kWorld

InterpolationMode StringToInterpolationMode(const std::string& str)
  // "none" -> kNone, "linear" -> kLinear, "cubicHermite"/"cubic" -> kCubicHermite
  // Unknown values log warning and default to kCubicHermite

PlaybackMode StringToPlaybackMode(const std::string& str)
  // "end" -> kEnd, "loop" -> kLoop, "wait" -> kWait
  // Unknown values log warning and default to kEnd

// Enum to String converters (for export)
std::string PointTypeToString(PointType type)
  // Switch statement with default fallback to "world"

std::string InterpolationModeToString(InterpolationMode mode)
  // Switch statement with default fallback to "cubicHermite"

std::string PlaybackModeToString(PlaybackMode mode)
  // Switch statement with default fallback to "end"
```
**Usage:** Used by `CameraPath.cpp` and `TimelineManager.cpp` for YAML import/export
- Import reads string values and converts to enums
- Export writes enums as human-readable strings
- All conversions include validation with warning logs for unknown values

**Cubic Hermite Interpolation:**
```cpp
// Standard (positions, linear values)
CubicHermiteInterpolate(a0, a1, a2, a3, t)
  ├─> Catmull-Rom tangents: m1 = (a2-a0)*0.5, m2 = (a3-a1)*0.5
  └─> Hermite basis: h00, h10, h01, h11 polynomial

// Angular (pitch/yaw, wraps correctly)
CubicHermiteInterpolateAngular(a0, a1, a2, a3, t)
  ├─> Convert angles to unit circle (sin/cos)
  ├─> Interpolate in 2D space
  └─> Convert back via atan2(sin, cos)
```

**GetTargetPoint(actor)** - Returns head bone position:
```cpp
1. Get race->bodyPartData
2. Find kHead body part (fallback: kTotal)
3. Lookup bone node: NiAVObject_LookupBoneNodeByName(actor3D, targetName)
4. Returns NiPointer<RE::NiAVObject> (world transform)
```
**Usage:** Calculate head bone offset for reference-based camera tracking
- Computes offset from actor's position to head bone world position
- Adds offset to Y (forward direction relative to actor's heading)
- Used for local-space kReference translation points

**ParseFCFWTimelineFileSections():**
- Parses INI sections with callback pattern
- Handles inline comments (`;` anywhere in value)
- Trims whitespace from keys/values
- Returns true if file parsed (even with errors in individual entries)

---

## **7. Critical Implementation Details**

### **Multi-Timeline Architecture**

**Current Architecture:**
- **Timeline Storage:** `std::unordered_map<size_t, TimelineState> m_timelines` - unlimited independent timelines
- **Per-timeline state:** TimelineState struct encapsulates timeline + all state variables (recording/playback flags, speeds, offsets)
- **Ownership tracking:** `SKSE::PluginHandle ownerPluginHandle` per timeline for access control
- **Active timeline:** `std::atomic<size_t> m_activeTimelineID` enforces exclusive playback/recording (only one timeline active at a time)
- **API pattern:** All functions require `timelineID` + `modName`/`pluginHandle` parameters with ownership validation

**Key Implementation Details:**

1. **Update() Loop:**
   - Gets active timeline ID from `m_activeTimelineID`
   - Fetches `TimelineState*` via direct map access (internal operation, no ownership check)
   - Passes state pointer to Play/Record helper functions

2. **Camera State Management:**
   - Only ONE timeline can be active (recording OR playing) at any time
   - Enforced via `m_activeTimelineID` (atomic, thread-safe)
   - PlayTimeline() checks `state->isPlaybackRunning` to distinguish from recording

3. **User Rotation Handling:**
   - Per-timeline rotation offset: `state->rotationOffset` in TimelineState
   - Hooks pass timelineID to SetUserTurning(timelineID, true)
   - Allows user to look around during playback while maintaining timeline-relative rotation

4. **Playback State Restoration:**
   - Per-timeline snapshots: `state->lastFreeRotation`, `state->isShowingMenus`
   - Restored on StopPlayback(timelineID, pluginHandle) to return to pre-playback state

5. **Point Baking (kCamera type):**
   - Per-timeline: `UpdateCameraPoints(TimelineState* state)` bakes kCamera points to world coordinates
   - Called on StartPlayback(timelineID, ...) before playback begins
   - Ensures consistent camera-relative positions throughout playback

6. **API Design:**
   - All functions use `timelineID` + `modName`/`pluginHandle` parameters
   - Return bool/int for error handling (not void)
   - Papyrus: modName (string) converted to pluginHandle via ModNameToHandle()
   - C++ Mod API: Direct pluginHandle parameter from SKSE::GetPluginHandle()

7. **File Operations:**
   - AddTimelineFromFile(timelineID, pluginHandle, filePath, timeOffset)
   - ExportTimeline(timelineID, pluginHandle, filePath)
   - Ownership validated before all file I/O operations

8. **Ownership Validation Pattern:**
   - Helper function: `GetTimeline(timelineID, pluginHandle)` validates ownership
   - Returns `nullptr` if timeline doesn't exist OR pluginHandle mismatch
   - All public API functions call this validator before operations
   - Internal overloads (no pluginHandle) available for FCFW-internal operations (hooks, Update loop)

---

## **8. Known Limitations**
- ⚠️ **Reference Deletion Safety:** No validation if `m_reference` becomes invalid during playback (dangling pointer risk)
  - `GetPoint()` will crash if reference deleted
  - No RefHandle tracking or validity checks
- ⚠️ **Loop + kCamera:** Baked values don't update on loop wrap (snapshot taken once at StartPlayback)
- ⚠️ **Gimbal Lock:** GetYaw/GetPitch functions unreliable near ±90° pitch (documented in _ts_SKSEFunctions.h)
- ⚠️ **Thread Safety:** None - all operations assume SKSE main thread (single-threaded by design)

---

## **9. External Dependencies**

**Required:**
- CommonLibSSE-NG (SKSE framework)
- spdlog (logging)
- SimpleIni (INI parsing with comment support)

**Optional:**
- po3's Tweaks (for reliable EditorID in exports)

**Build Tools:**
- CMake 3.29+, Ninja, MSVC (cl.exe)
- vcpkg for dependencies

