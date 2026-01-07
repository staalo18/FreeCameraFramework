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
- **Import/Export**: Save/load camera paths as `.fcfw` timeline files with load-order independent reference tracking
- **Event Callback System**: Notify consumers when timeline playback starts/stops/completes
  - **SKSE Messaging Interface**: C++ plugins receive events via SKSE messaging system
  - **Papyrus Events**: Scripts receive `OnTimelinePlaybackStarted`, `OnTimelinePlaybackStopped`, and `OnTimelinePlaybackCompleted` events with timeline ID
- **Three API Surfaces**:
  - **Papyrus Scripts**: Full scripting integration with mod name + timeline ID parameters
  - **C++ Mod API**: Native plugin-to-plugin communication with SKSE plugin handle validation
  - **Debug Hotkeys**: 8 keyboard shortcuts for testing (operates on most recently used timeline)
- **Optional TrueHUD Integration**: Real-time 3D path visualization during playback

**Target Users:**
- Skyrim machinima creators needing cinematic camera control
- Modders building scripted scenes with dynamic camera movements
- Screenshot artists creating complex camera animations
- Developers extending FCFW functionality through the C++ API

---

## **Developer Quick Start**

### **Project Architecture Summary**
FCFW follows a **singleton manager pattern** with **multi-timeline storage** and **template-based timeline engines**. The core design separates concerns into:
1. **API Layer**: Three entry points (Papyrus, C++ Mod API, Keyboard) → all funnel into `TimelineManager`
2. **Orchestration Layer**: `TimelineManager` singleton manages timeline map and coordinates recording/playback state
   - Stores `std::unordered_map<size_t, TimelineState>` for multi-timeline support
   - Tracks ownership via SKSE plugin handles
   - Enforces exclusive playback/recording (only one active timeline at a time)
3. **Timeline Layer**: `Timeline` class pairs translation and rotation tracks with metadata
4. **Engine Layer**: `TimelineTrack<T>` template handles interpolation and playback mechanics
5. **Storage Layer**: `CameraPath<T>` template manages point collections and file I/O

**Key Design Patterns:**
- **Singleton**: `TimelineManager`, `APIManager`, `ControlsManager` (thread-safe Meyer's singleton)
- **Multi-Timeline Storage**: `std::unordered_map<size_t, TimelineState>` with atomic ID generation
  - Thread-safe access via `std::mutex m_timelineMutex`
  - Each `TimelineState` contains: `Timeline`, ownership info, recording/playback state
- **Ownership Validation**: Two-tier system for external API vs internal operations
  - **External API (C++, Papyrus, Keyboard):** `GetTimeline(timelineID, pluginHandle)` validates ownership for ALL modification and query operations
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
  ├─ CameraPath.cpp       # Point storage + import/export (.fcfw file format)
  ├─ ControlsManager.cpp  # Keyboard input handling (8 debug hotkeys)
  ├─ Hooks.cpp            # Game loop interception (MainUpdateHook)
  ├─ APIManager.cpp       # External API requests (TrueHUD integration)
  └─ FCFW_Utils.cpp       # Hermite interpolation + file parsing helpers

include/
  ├─ TimelineTrack.h      # TimelineTrack<T> template (declaration + implementation)
  ├─ Timeline.h           # Timeline class declaration (non-template wrapper)
  ├─ CameraPath.h         # Point storage templates (TranslationPath, RotationPath)
  ├─ CameraTypes.h        # Core enums (InterpolationMode, PointType, PlaybackMode)
  ├─ TimelineManager.h    # Main orchestrator interface
  ├─ FCFW_API.h           # C++ Mod API + event messaging interface definition
  └─ API/TrueHUDAPI.h     # External API header (v3)
```

**Data Flow:**
```
User Input (Papyrus/Hotkey/ModAPI)
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
  └─ DrawTimeline(const TimelineState*) → TrueHUD->DrawLine() (if available)
  ↓
Apply to RE::FreeCameraState (position/rotation)
```

**Encapsulation Boundaries:**
- **TimelineManager** → sees only Timeline public API
- **Timeline** → sees TimelineTrack public API (GetPath() hidden)
- **TimelineTrack** → directly accesses m_path member (Path fully encapsulated)

**Common Tasks:**
- **Adding New API Functions**: Bind in `plugin.cpp` (Papyrus), implement in `TimelineManager`, expose in `FCFW_API.h` (Mod API)
- **Changing Interpolation**: Modify `TimelineTrack<T>::GetPointAtTime()` in `Timeline.inl` (calls interpolation helpers)
- **File Format Changes**: Update `CameraPath::ExportPath()` and `AddPathFromFile()` in `CameraPath.cpp`
- **New Point Types**: Extend `PointType` enum in `CameraTypes.h`, add cases in `TranslationPoint::GetPoint()` / `RotationPoint::GetPoint()` methods

**Build Dependencies:**
- **CommonLibSSE-NG**: SKSE plugin framework (set `COMMONLIBSSE_PATH` env var)
- **_ts_SKSEFunctions**: Utility library (must be sibling directory) - provides interpolation/easing functions
- **vcpkg**: Dependency manager (fmt, spdlog, rapidcsv, simpleini)
- **CMake 3.29+**: Build system with MSVC presets

**Testing Entry Points:**
1. Launch Skyrim AE with SKSE
2. Open console: `tfc` (enable free camera)
3. From Papyrus: Call `FCFW_RegisterTimeline("YourMod.esp")` to get a timeline ID
4. Press `8` to start recording (or call `FCFW_StartRecording("YourMod.esp", timelineID)`)
5. Move camera, then press `9` to stop
6. Exit free camera (`tfc`), then press `7` to play timeline
7. Check logs: `Documents/My Games/Skyrim Special Edition/SKSE/FreeCameraFramework.log`

**Note**: Keyboard controls operate on the most recently used timeline ID (stored in `ControlsManager`)

**Common Pitfalls & Debugging:**
- **C++ API Returns 0 or -1**: Check that `RegisterTimeline()` succeeded before using returned ID. ID of 0 indicates registration failure (check logs for errors).
- **Crash on API Call**: All API functions validate timeline ID and ownership before proceeding. If crashing, check for use-after-move bugs or null pointer dereferences in wrapper code.
- **Timeline Operations Fail Silently**: Verify ownership - only the plugin that registered a timeline can access it. ALL operations (queries, playback control, modification) require ownership validation and will fail if the timeline doesn't belong to the calling plugin.
- **Missing Log Output**: Ensure `FreeCameraFramework.ini` has correct `LogLevel` (0-6, default 3=info). Use-after-move bugs can cause logging to crash before return values propagate.

---

## **Quick Reference**

### **Architecture at a Glance:**
```
Entry Points (3 surfaces):
  ├─ Papyrus API (plugin.cpp) → FCFW_SKSEFunctions script
  │   └─ Parameters: string modName, int timelineID, ...
  ├─ Mod API (ModAPI.h) → FCFW_API::IVFCFW1 interface
  │   └─ Parameters: size_t timelineID, SKSE::PluginHandle, ...
  └─ Keyboard (ControlsManager.cpp) → Hardcoded DXScanCode handlers
      └─ Operates on m_lastUsedTimelineID (most recent timeline)

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
    ├─ DrawTimeline(activeState) → TrueHUD visualization
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
- **Current State**: ✅ **Multi-timeline implementation complete (Phase 4 finished January 1, 2026)**
  - Unlimited independent timelines per mod/plugin
  - Ownership validation via SKSE plugin handles
  - Timeline ID + mod name/plugin handle required for all API calls
  - Exclusive playback/recording enforcement (one active timeline at a time)
  - Papyrus API, C++ Mod API, and keyboard controls all migrated

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
└─> kPostLoadGame/kNewGame:
    ├─> APIs::RequestAPIs()
    └─> Register ControlsManager as input event sink

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

**Timeline Building Functions:**
| Papyrus Function | Return | Parameters | Notes |
|-----------------|--------|------------|-------|
| `FCFW_AddTranslationPointAtCamera` | int | **modName, timelineID**, time, easeIn, easeOut, interpolationMode | Captures camera position |
| `FCFW_AddTranslationPoint` | int | **modName, timelineID**, time, posX, posY, posZ, easeIn, easeOut, interpolationMode | Absolute world position |
| `FCFW_AddTranslationPointAtRef` | int | **modName, timelineID**, time, ref, offsetX/Y/Z, isOffsetRelative, easeIn, easeOut, interpolationMode | Ref-based position |
| `FCFW_AddRotationPointAtCamera` | int | **modName, timelineID**, time, easeIn, easeOut, interpolationMode | Captures camera rotation |
| `FCFW_AddRotationPoint` | int | **modName, timelineID**, time, pitch, yaw, easeIn, easeOut, interpolationMode | Absolute world rotation |
| `FCFW_AddRotationPointAtRef` | int | **modName, timelineID**, time, ref, offsetPitch/Yaw, isOffsetRelative, easeIn, easeOut, interpolationMode | Ref-based rotation |
| `FCFW_RemoveTranslationPoint` | bool | **modName, timelineID**, index | Remove by index (requires ownership) |
| `FCFW_RemoveRotationPoint` | bool | **modName, timelineID**, index | Remove by index (requires ownership) |
| `FCFW_ClearTimeline` | bool | **modName, timelineID**, notifyUser | Clear all points (requires ownership) |
| `FCFW_GetTranslationPointCount` | int | **modName, timelineID** | Query point count (-1 if timeline not found) |
| `FCFW_GetRotationPointCount` | int | **modName, timelineID** | Query point count (-1 if timeline not found) |

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
| `FCFW_SetPlaybackMode` | bool | **modName, timelineID**, playbackMode | Set playback mode (0=kEnd, 1=kLoop, 2=kWait) - requires ownership |

**Import/Export Functions:**
| Papyrus Function | Return | Parameters | Notes |
|-----------------|--------|------------|-------|
| `FCFW_AddTimelineFromFile` | bool | **modName, timelineID**, filePath, timeOffset | Import INI file (requires ownership) |
| `FCFW_ExportTimeline` | bool | **modName, timelineID**, filePath | Export to INI file (validates ownership) |

**Parameter Notes:**
- **modName**: Your mod's ESP/ESL filename (e.g., `"MyMod.esp"`), case-sensitive. Required for ALL timeline operations to validate that the calling plugin has access to the specified timeline.
- **timelineID**: Integer ID returned by `RegisterTimeline()`, must be > 0
- **playbackMode**: Integer value for PlaybackMode enum (0=kEnd, 1=kLoop, 2=kWait)
- **Ownership Validation**: Applied universally to all timeline API calls. The pluginHandle/modName parameter is required as the FIRST parameter and is always validated - operations fail if the timeline doesn't exist or doesn't belong to the calling plugin.
- **Return Values**: `-1` for query functions on error, `false` for boolean functions on error

**Type Conversion Layer:**
- **InterpolationMode**: Papyrus passes `int` (0=None, 1=Linear, 2=CubicHermite), converted via `ToInterpolationMode()` in `CameraTypes.h`
- **Mod Name to Handle**: `ModNameToHandle(modName)` searches loaded files for matching ESP/ESL, returns `file->compileIndex` as plugin handle
- **Return Values**: TimelineManager returns `int` for point indices, `bool` for operations

**Event System:**
Papyrus scripts can register forms (Quests, ReferenceAliases, etc.) to receive timeline playback events. Registered forms receive these event callbacks:
- `OnTimelinePlaybackStarted(int timelineID)` - Fired when any timeline starts playing
- `OnTimelinePlaybackStopped(int timelineID)` - Fired when any timeline stops playing (manual stop or kEnd mode completion)
- `OnTimelinePlaybackCompleted(int timelineID)` - Fired when timeline reaches end in kWait mode (stays at final position)

**Example Usage:**
```papyrus
Scriptname MyQuest extends Quest

Event OnInit()
    FCFW_SKSEFunctions.FCFW_RegisterForTimelineEvents(self)
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

Event OnTimelinePlaybackStarted(int timelineID)
    Debug.Notification("Timeline " + timelineID + " started")
EndEvent

Event OnTimelinePlaybackStopped(int timelineID)
    Debug.Notification("Timeline " + timelineID + " stopped")
EndEvent

Event OnTimelinePlaybackCompleted(int timelineID)
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
    kTimelinePlaybackStarted = 0,
    kTimelinePlaybackStopped = 1,
    kTimelinePlaybackCompleted = 2  // kWait mode: reached end, staying at final position
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
            case static_cast<uint32_t>(FCFW_API::FCFWMessage::kTimelinePlaybackStarted): {
                auto* data = static_cast<FCFW_API::FCFWTimelineEventData*>(msg->data);
                // Handle timeline data->timelineID started
                break;
            }
            case static_cast<uint32_t>(FCFW_API::FCFWMessage::kTimelinePlaybackStopped): {
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
  - Query/playback operations don't require ownership validation
- **Signatures:** Similar to Papyrus but with native C++ types (`size_t`, `SKSE::PluginHandle` instead of `string`, `int`)
  - All functions marked `const noexcept`
  - `SetPlaybackMode(timelineID, pluginHandle, playbackMode)` - playbackMode as int (0=kEnd, 1=kLoop, 2=kWait)
  - All functions delegate to `TimelineManager::GetSingleton()`
  - Query/playback operations don't require ownership validation
- **Signatures:** Similar to Papyrus but with native C++ types (`size_t`, `SKSE::PluginHandle` instead of `string`, `int`)
  - All functions marked `const noexcept`
  - `int interpolationMode` converted via `ToInterpolationMode()` at API boundary
  - All functions delegate to `TimelineManager::GetSingleton()`

**Implementation: `FCFWInterface`** (in `ModAPI.h`, implemented in `Messaging` namespace)
- Singleton pattern: `GetSingleton()` returns static instance
- Private constructor/destructor
- Stores `apiTID` member for thread tracking
- All methods marked `const noexcept override`

---

### **2.4 Keyboard Controls (`ControlsManager.cpp`)**
**Integration:** Registered as `RE::BSTEventSink<RE::InputEvent*>` on game load

**Multi-Timeline Context:**
- All operations use a static `timelineID` variable (declared in ControlsManager.cpp namespace scope)
- Timeline ID is passed to all TimelineManager calls (with SKSE::GetPluginHandle())
- Timeline is registered on first keypress (lazy initialization)
- Once registered, the same timeline ID is used for all keyboard operations

**Hardcoded Keybindings** (DXScanCode values):
| Key | Action | TimelineManager Call |
|-----|--------|----------------------|
| 2 | Toggle Pause | `PausePlayback(timelineID)` / `ResumePlayback(timelineID)` |
| 3 | Stop Playback | `StopPlayback(timelineID)` |
| 4 | Toggle User Rotation | `AllowUserRotation(timelineID, !IsUserRotationAllowed(timelineID))` |
| 5 | **Debug Scene** | Complex hardcoded camera path demo (uses timelineID) |
| 6 | Clear Timeline | `ClearTimeline(timelineID)` |
| 7 | Start Playback | `StartPlayback(timelineID, ...)` |
| 8 | Start Recording | `StartRecording(timelineID, ...)` |
| 9 | Stop Recording | `StopRecording(timelineID)` |
| 10 | Export | `ExportTimeline(timelineID, "SKSE/Plugins/FCFW_CameraPath.ini")` |
| 11 | Import | `AddTimelineFromFile(timelineID, "SKSE/Plugins/FCFW_CameraPath.ini")` |

**Key 5 Scene Details:**
- Looks up FormID `0xd8c56` as `RE::TESObjectREFR` (must be an actor)
- Builds 11-point camera path: camera → actor tracking → player → camera
- Uses `GetTargetPoint()` to calculate head bone offset from actor's position
- Adds +20 units to Y offset (forward direction, not upward)
- Demonstrates all point types: kCamera, kReference (world-space and local-space)

**Input Filtering:**
- Ignores input when `RE::UI::GetSingleton()->GameIsPaused()` is true
- Only processes `kButton` events with `IsDown()` state

---

### **2.5 External APIs (`APIManager.h/cpp`)**
**Purpose:** Request APIs from other SKSE plugins

**Currently Integrated:**
- **TrueHUD API v3** (`TRUEHUD_API::IVTrueHUD3`)
  - Requested at multiple SKSE lifecycle stages (workaround for messaging bug)
  - Stored in static inline pointer: `APIs::TrueHUD`
  - **Usage:**
    - `TimelineManager::DrawTimeline()` (line 286): Calls `DrawLine(point1, point2)` to visualize camera path in 3D world space
    - `FCFW_Utils::SetHUDMenuVisible()` (line 52): Calls `SetHUDMenuVisible()` to control menu visibility
  - Optional: Functions only execute if TrueHUD is installed and API is available

---

### **2.6 Hooks (`Hooks.h/cpp`)**
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
- **kWait Behavior:** Timeline reaches end, dispatches kTimelinePlaybackCompleted event, then continues playing at final frame position (allowing dynamic reference tracking). User must call StopPlayback() to end playback.

### **3.2 Naming Conventions**
    - **Member Variables:** `m_` prefix (e.g., `m_translationTrack`, `m_isPlaying`)
- **Function Parameters:** `a_` prefix (e.g., `a_time`, `a_reference`, `a_point`)
- **Logger:** Uses `log::info`, `log::warn`, `log::error` (CommonLib pattern)
- **Error Handling:** Return error codes (negative `int`) or `false`, log errors to file

### **3.3 Singleton Pattern**
- **TimelineManager**, **ControlsManager**, **FCFWInterface**, **APIManager**
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
bool m_isShowingMenus{ true };                          // Pre-playback UI state
bool m_showMenusDuringPlayback{ false };                // UI visibility during playback
bool m_userTurning{ false };                            // User camera control flag
RE::NiPoint2 m_lastFreeRotation;                        // Third-person camera rotation snapshot

// Event System (global)
std::vector<RE::TESForm*> m_eventReceivers;             // Forms registered for Papyrus events
```

**TimelineState Structure** (per-timeline storage):
```cpp
struct TimelineState {
    // Timeline data
    size_t m_id;                           // Unique timeline identifier
    Timeline m_timeline;                   // Paired translation + rotation tracks
    
    // Recording state (per-timeline)
    bool m_isRecording{ false };           // Currently capturing camera
    float m_currentRecordingTime{ 0.0f };
    float m_lastRecordedPointTime{ 0.0f };
    
    // Playback state (per-timeline)
    bool m_isPlaybackRunning{ false };     // Active playback
    float m_playbackSpeed{ 1.0f };
    bool m_globalEaseIn{ false };
    bool m_globalEaseOut{ false };
    float m_playbackDuration{ 0.0f };
    bool m_showMenusDuringPlayback{ false };
    bool m_allowUserRotation{ false };     // Allow user to control rotation during playback
    bool m_isCompletedAndWaiting{ false }; // Track if kTimelinePlaybackCompleted event was dispatched (for kWait mode)
    RE::BSTPoint2<float> m_rotationOffset{ 0.0f, 0.0f }; // Per-timeline rotation offset from user input
    
    // Owner tracking
    SKSE::PluginHandle m_ownerHandle;      // Plugin that registered this timeline
    std::string m_ownerName;               // Plugin name (for logging)
};
```

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
  - Global (shared): `m_recordingInterval`, `m_isShowingMenus`, `m_userTurning`, `m_lastFreeRotation`, `m_eventReceivers`
  - Per-Timeline (in TimelineState): `m_isRecording`, `m_currentRecordingTime`, `m_lastRecordedPointTime`, `m_isPlaybackRunning`, `m_playbackSpeed`, `m_globalEaseIn`, `m_globalEaseOut`, `m_playbackDuration`, `m_showMenusDuringPlayback`, `m_allowUserRotation`, `m_isCompletedAndWaiting`, `m_rotationOffset`

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
    
    DrawTimeline(state);     // Visualize path via TrueHUD (if available)
    PlayTimeline(state);     // Update camera from interpolated points
    RecordTimeline(state);   // Sample camera position if recording
}
```

**Multi-Timeline Changes:**
- Gets active timeline ID from `m_activeTimelineID` (atomic read)
- Fetches `TimelineState*` via direct map access (bypasses ownership check for internal Update loop)
- Passes `TimelineState*` to all helper functions
- Only ONE timeline can be active (recording or playing) at a time

**Critical Insight:** All three operations coexist in the loop, but guards prevent conflicts:
- `PlayTimeline(state)` checks `state->isPlaybackRunning && !state->isRecording`
- `RecordTimeline(state)` checks `state->isRecording`
- `DrawTimeline(state)` only draws when `!state->isPlaybackRunning && !state->isRecording`

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
│   ├─> DispatchTimelineEvent(kTimelinePlaybackStarted, timelineID)  [SKSE messaging]
│   └─> DispatchTimelineEventPapyrus("OnTimelinePlaybackStarted", timelineID)  [Papyrus events]
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
│   ├─> DispatchTimelineEvent(kTimelinePlaybackStopped, timelineID)  [SKSE messaging]
│   └─> DispatchTimelineEventPapyrus("OnTimelinePlaybackStopped", timelineID)  [Papyrus events]
├─> If in free camera:
│   ├─> Exit free camera mode
│   ├─> Restore state->isShowingMenus
│   └─> Restore state->lastFreeRotation (for third-person)
├─> Set state->isPlaybackRunning = false
└─> Set m_activeTimelineID = 0

SetPlaybackMode(timelineID, pluginHandle, playbackMode)
├─> Validate ownership: GetTimeline(timelineID, pluginHandle)
├─> Validate playback mode: 0 (kEnd), 1 (kLoop), or 2 (kWait)
├─> Cast int to PlaybackMode enum
└─> Call state->timeline.SetPlaybackMode(mode)
    └─> Sets mode on both translation and rotation tracks

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
│   ├─> Dispatch kTimelinePlaybackStopped for source timeline
│   └─> Dispatch "OnTimelinePlaybackStopped" Papyrus event
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
│   ├─> Dispatch kTimelinePlaybackStarted for target timeline
│   └─> Dispatch "OnTimelinePlaybackStarted" Papyrus event
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
    * Message types: `kTimelinePlaybackStarted` (0), `kTimelinePlaybackStopped` (1)
    * Data: `FCFWTimelineEventData{ size_t timelineID }`
    * Sender: `FCFW_API::FCFWPluginName` ("FreeCameraFramework")
  - **Papyrus Events (Scripts):**
    * `DispatchTimelineEventPapyrus(eventName, timelineID)` queues to Papyrus thread
    * Thread Safety: Uses `SKSE::GetTaskInterface()->AddTask()` with lambda
    * Event names: "OnTimelinePlaybackStarted", "OnTimelinePlaybackStopped"
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
  - **kEnd (0):** Timeline reaches end → dispatches kTimelinePlaybackStopped → stops playback automatically
  - **kLoop (1):** Timeline reaches end → wraps to beginning seamlessly using modulo time
  - **kWait (2):** Timeline reaches end → dispatches kTimelinePlaybackCompleted event → continues playing at final frame
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

uint32_t m_timelineID{ 0 };           // Unique identifier (for future multi-timeline)
float m_playbackSpeed{ 1.0f };        // Time multiplier
bool m_globalEaseIn{ false };         // Apply easing to entire timeline
bool m_globalEaseOut{ false };        // Apply easing to entire timeline
```

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

**Visualization:**
```cpp
RE::NiPoint3 GetTranslationPointPosition(size_t a_index) const;  // For DrawTimeline visualization
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

**Concrete Implementations:**
```cpp
class TranslationPath : public CameraPath<TranslationPoint> {
public:
    using TransitionPoint = TranslationPoint;
    using ValueType = RE::NiPoint3;  // Type returned by GetPoint()
    
    TranslationPoint GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const override;  // Creates kCamera point
    bool AddPathFromFile(...) override;
    bool ExportPath(...) const override;
};

class RotationPath : public CameraPath<RotationPoint> {
public:
    using TransitionPoint = RotationPoint;
    using ValueType = RE::BSTPoint2<float>;  // Type returned by GetPoint()
    
    RotationPoint GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const override;  // Creates kCamera point
    bool AddPathFromFile(...) override;
    bool ExportPath(...) const override;
};
```

**Key Insights:** 
- `ValueType` alias resolves naming conflict with `PointType` enum (kWorld/kReference/kCamera)
- `GetPointAtCamera()` is const-correct: creates new point without modifying path state
- Takes time/easing parameters, returns TransitionPoint with PointType::kCamera and transition metadata

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

#### **INI File Format:**
```ini
[General]
Version=10203           ; major*10000 + minor*100 + patch
UseDegrees=1            ; Convert angles (always 1 for export)
PlaybackMode=0          ; 0=kEnd, 1=kLoop
LoopTimeOffset=0.0      ; Interpolation time for loop wrap

[TranslationPoints]
; Point fields vary by PointType (see below)

[RotationPoints]
; Point fields vary by PointType
```

**Field Encoding by PointType:**

| PointType | Translation Fields | Rotation Fields |
|-----------|-------------------|-----------------|
| **kWorld** | PositionX/Y/Z | Pitch, Yaw |
| **kReference** | OffsetX/Y/Z, RefFormID, IsOffsetRelative | OffsetPitch/Yaw, RefFormID, IsOffsetRelative |
| **kCamera** | OffsetX/Y/Z | OffsetPitch/Yaw |

**All Points Include:**
- `Time`, `InterpolationMode` (0/1/2), `EaseIn` (0/1), `EaseOut` (0/1), `PointType` (0/1/2)

**Import Process (TimelineManager.cpp):**
```
AddTimelineFromFile(timelineID, pluginHandle, path, timeOffset)
├─> Validate ownership: GetTimeline(timelineID, pluginHandle)
├─> Read General section (version, degrees, playback mode, loop offset)
├─> Import translation: state->timeline.GetTranslationTrack().GetPath().AddPathFromFile(stream, offset, 1.0f)
├─> Rewind stream
├─> Import rotation: state->timeline.GetRotationTrack().GetPath().AddPathFromFile(stream, offset, degToRad)
├─> Set playback mode/offset: state->timeline.SetPlaybackMode(), SetLoopTimeOffset()
└─> Log point counts
```

**Export Process (TimelineManager.cpp):**
```
ExportTimeline(timelineID, pluginHandle, path)
├─> Validate ownership: GetTimeline(timelineID, pluginHandle)
├─> Write General section (get mode from state->timeline.GetTranslationTrack().GetPlaybackMode())
├─> Export translation: state->timeline.GetTranslationTrack().GetPath().ExportPath(stream, 1.0f)
├─> Export rotation: state->timeline.GetRotationTrack().GetPath().ExportPath(stream, radToDeg)
└─> Log point counts
```

**Multi-Timeline Changes:**
- All import/export functions require `timelineID` + `pluginHandle` parameters
- Ownership validated before file operations
- Timeline ownership tracked via `state->ownerPluginHandle`

**Critical Details:**
- **Inline Comments:** Import supports `;` comments (via CSimpleIniA)
- **Time Offset:** Applied to all imported point times (for timeline concatenation)
- **Conversion Factor:** Translation uses 1.0 (no conversion), Rotation uses deg/rad conversion
- **Playback Mode:** Read from General section, applied via `state->timeline.SetPlaybackMode()` (syncs both tracks)
- **Version Check:** Warns if file version ≠ plugin version (non-blocking)

---

### **5.9 DrawTimeline() Visualization**

**Conditions:**
- `APIs::TrueHUD != nullptr` (TrueHUD plugin loaded)
- Active timeline exists (`m_activeTimelineID != 0`)
- `TimelineState* state` fetched successfully
- Points exist (translation or rotation)
- `!state->isPlaybackRunning && !state->isRecording`
- Free camera mode active

**Behavior (TimelineManager.cpp):**
```cpp
void TimelineManager::DrawTimeline(TimelineState* state) {
    if (!APIs::TrueHUD || !state) return;
    if (state->isPlaybackRunning || state->isRecording) return;
    
    for (i = 0; i < state->timeline.GetTranslationPointCount() - 1; ++i) {
        auto& point1 = state->timeline.GetTranslationTrack().GetPath().GetPoint(i).m_point;
        auto& point2 = state->timeline.GetTranslationTrack().GetPath().GetPoint(i+1).m_point;
        APIs::TrueHUD->DrawLine(point1, point2);
    }
}
```

**Multi-Timeline Changes:**
- Operates on `TimelineState*` parameter (passed from Update() loop)
- Only draws active timeline (m_activeTimelineID)
- Per-timeline playback/recording checks

**Visual Result:** Line segments connecting translation keyframes (visible in HUD)

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
        kTimelinePlaybackStarted = 0,
        kTimelinePlaybackStopped = 1
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
            case static_cast<uint32_t>(FCFW_API::FCFWMessage::kTimelinePlaybackStarted): {
                auto* data = static_cast<FCFW_API::FCFWTimelineEventData*>(msg->data);
                // Handle timeline started: data->timelineID
                break;
            }
            case static_cast<uint32_t>(FCFW_API::FCFWMessage::kTimelinePlaybackStopped): {
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

Event OnTimelinePlaybackStarted(int timelineID)
    Debug.Notification("Timeline " + timelineID + " started playing")
EndEvent

Event OnTimelinePlaybackStopped(int timelineID)
    Debug.Notification("Timeline " + timelineID + " stopped")
EndEvent
```

**Event Dispatch Locations:**
- `StartPlayback()`: Dispatches both SKSE message and Papyrus event for `kTimelinePlaybackStarted`
- `StopPlayback()`: Dispatches both SKSE message and Papyrus event for `kTimelinePlaybackStopped`

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

**HUD Control:**
```cpp
SetHUDMenuVisible(visible)
  ├─> Requires TrueHUD API
  └─> Sets TrueHUD menu visibility via uiMovie->SetVisible()
```

**GetTargetPoint(actor)** - Returns head bone position:
```cpp
1. Get race->bodyPartData
2. Find kHead body part (fallback: kTotal)
3. Lookup bone node: NiAVObject_LookupBoneNodeByName(actor3D, targetName)
4. Returns NiPointer<RE::NiAVObject> (world transform)
```
**Usage:** Calculate head bone offset in ControlsManager Key 5 demo
- Computes offset from actor's position to head bone world position
- Adds +20 units to Y (forward direction relative to actor's heading)
- Used for local-space kReference translation points

**ParseFCFWTimelineFileSections():**
- Parses INI sections with callback pattern
- Handles inline comments (`;` anywhere in value)
- Trims whitespace from keys/values
- Returns true if file parsed (even with errors in individual entries)

---

## **7. Critical Implementation Details**

### **Multi-Timeline Migration: ✅ COMPLETED (Phase 4 + Cleanup)**

**Migration Completion:** January 1, 2026  
**Cleanup Completion:** January 2, 2026  
**Final Status:** All 20 functions migrated, tested, and all old code removed

**Architecture Transformation:**

**BEFORE (Single-Timeline):**
- Single Timeline: `m_timeline` (direct member)
- Flat state: 12+ bool/float members (recording, playback, user rotation)
- No timeline IDs - direct member access pattern
- API functions: No parameters, operate on singleton state
- Single camera state: One FreeCameraState updated per frame

**AFTER (Multi-Timeline):**
- Timeline Map: `std::unordered_map<size_t, TimelineState> m_timelines`
- Per-timeline state: TimelineState struct encapsulates timeline + all state variables
- Ownership tracking: `SKSE::PluginHandle ownerPluginHandle` per timeline
- Active timeline: `std::atomic<size_t> m_activeTimelineID` (exclusive enforcement)
- API functions: All require `timelineID` + `modName`/`pluginHandle` parameters

**Key Migrations Completed:**

1. **Update() Loop** - ✅ COMPLETE
   - Gets active timeline ID from `m_activeTimelineID`
   - Fetches `TimelineState*` via direct map access (no ownership check needed)
   - Passes state to Play/Record/Draw helpers

2. **Camera State Management** - ✅ COMPLETE
   - Only ONE timeline can be active (recording OR playing)
   - Enforced via `m_activeTimelineID` (atomic, thread-safe)
   - PlayTimeline() checks `state->isPlaybackRunning`

3. **User Rotation Offset** - ✅ COMPLETE
   - Per-timeline: `state->rotationOffset` in TimelineState
   - Hooks pass timelineID: `SetUserTurning(timelineID, true)`

4. **Playback State Restoration** - ✅ COMPLETE
   - Per-timeline snapshots: `state->lastFreeRotation`, `state->isShowingMenus`
   - Restored on StopPlayback(timelineID, pluginHandle)

5. **Point Baking** - ✅ COMPLETE
   - Per-timeline: `UpdateCameraPoints(TimelineState* state)`
   - Called on StartPlayback(timelineID, ...) for that timeline only

6. **API Surface** - ✅ COMPLETE
   - All 20 functions migrated with `timelineID` + `modName`/`pluginHandle` parameters
   - Return bool/int instead of void for error handling
   - Papyrus: modName (string) converted to pluginHandle via ModNameToHandle()
   - C++ Mod API: Direct pluginHandle parameter

7. **Import/Export** - ✅ COMPLETE
   - AddTimelineFromFile(timelineID, pluginHandle, filePath, timeOffset)
   - ExportTimeline(timelineID, pluginHandle, filePath)
   - Timeline ownership tracked via pluginHandle

8. **Ownership Validation** - ✅ COMPLETE
   - Helper function: `GetTimeline(timelineID, pluginHandle)` validates ownership
   - Returns `nullptr` if timeline doesn't exist or pluginHandle mismatch
   - All public API functions validate before operations

**Bug Fixes Post-Migration:**
- ✅ Fixed: AddTimelineFromFile missing pluginHandle parameter in C++ API
- ✅ Fixed: ModAPI.h abstract class error (5 missing function declarations)
- ✅ Fixed: Hook movement controls blocked during recording (now check IsPlaybackRunning())

**Phase 4 Cleanup (January 2, 2026):**
- ✅ Removed: Old `RecordTimeline()` function (no parameters version)
- ✅ Removed: Old `DrawTimeline()` function (no parameters version)
- ✅ Removed: Old `GetTimelineDuration()` function
- ✅ Removed: Old `AddTranslationPoint(const TranslationPoint&)` helper
- ✅ Removed: Old `AddRotationPoint(const RotationPoint&)` helper
- ✅ Added: Missing `m_recordingInterval` member variable to TimelineManager
- ✅ Verified: All member variables correctly categorized as global vs per-timeline

**Reference Documents:**
- TASK_MULTI_TIMELINE_SUPPORT.md: Design specification
- PHASE4_FUNCTION_MIGRATION.md: Migration guide with all 20 functions

---

## **8. Known Limitations & Unimplemented Features**

### **Implemented & Verified:**
- ✅ **GetTargetPoint()** - Returns head bone's `NiAVObject` (world transform) via `NiAVObject_LookupBoneNodeByName`
- ✅ **SetHUDMenuVisible()** - Controls TrueHUD menu visibility via `uiMovie->SetVisible()`
- ✅ **ModAPI.cpp** - Simple pass-through to TimelineManager (converts `int` → `InterpolationMode`)
- ✅ **Offsets.h** - Defines `NiAVObject_LookupBoneNodeByName` (RELOCATION_ID 74481/76207, from True Directional Movement)

### **Known Limitations:**
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
- TrueHUD API v3 (for DrawTimeline visualization)
- po3's Tweaks (for reliable EditorID in exports)

**Build Tools:**
- CMake 3.29+, Ninja, MSVC (cl.exe)
- vcpkg for dependencies

