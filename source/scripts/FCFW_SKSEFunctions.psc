Scriptname FCFW_SKSEFunctions

; For Debug/Testing: Toggle display of body part rotation matrix for the specified actor
function ToggleBodyPartRotationMatrixDisplay(actor a_actor, int a_bodyPart) global native

; Get plugin version
; Encoded as: major * 10000 + minor * 100 + patch
; Example: version 1.2.3 returns 10203
int Function GetPluginVersion() global native

; ===== Plugin Registration =====

; Register your plugin with FCFW (required before RegisterTimeline)
; This function should be called during plugin initialization (OnInit/OnPlayerLoadGame)
; If your plugin was already registered, this will clean up any orphaned timelines
; from previous game sessions (eg when loading a savegame while in-game)
; and prepare for fresh timeline registration
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; Returns: true on success, false on failure
bool Function RegisterPlugin(string modName) global native

; ===== Timeline Management =====

; Register a new timeline and get its unique ID
; Each mod can register multiple independent timelines
; IMPORTANT: You must call RegisterPlugin() first before calling this function
; IMPORTANT: Timeline IDs are permanent once registered. To update a timeline's
; content, use ClearTimeline() followed by Add...Point() calls. Only unregister
; when you no longer need the timeline at all (e.g., plugin shutdown).
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; Returns: new timeline ID (>0) on success, or -1 on failure
int Function RegisterTimeline(string modName) global native

; Unregister a timeline and free its resources
; This will stop any active playback/recording on the timeline before removing it
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to unregister
; Returns: true if successfully unregistered, false on failure
bool Function UnregisterTimeline(string modName, int timelineID) global native

; ===== Timeline Building =====

; Add a translation point at a specified position
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to add the point to
; time: time in seconds when this point occurs
; posX, posY, posZ: position coordinates
; easeIn: ease in at the start of interpolation
; easeOut: ease out at the end of interpolation
; interpolationMode: 0=None, 1=Linear, 2=CubicHermite (default)
; Returns: index of the added point, or -1 on failure
; NOTE: Both easeIn and easeOut control the INCOMING segment (previous->current point), not the outgoing segment.
; For smooth transition through a point, set easeOut=false for the current point AND easeIn=false for the next point.
int Function AddTranslationPoint(string modName, int timelineID, float time, float posX, float posY, float posZ, bool easeIn = false, bool easeOut = false, int interpolationMode = 2) global native

; Add a translation point relative to a reference object
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to add the point to
; time: time in seconds when this point occurs
; reference: the object reference to track
; bodyPart: body part to track for actors (0=kNone/root, 1=kHead, 2=kTorso). Non-actors always use root. Each has fallback logic if unavailable.
; offsetX, offsetY, offsetZ: offset from reference position
; isOffsetRelative: if true, offset is relative to reference's heading (local space), otherwise world space
; easeIn: ease in at the start of interpolation
; easeOut: ease out at the end of interpolation
; interpolationMode: 0=None, 1=Linear, 2=CubicHermite (default)
; Returns: index of the added point, or -1 on failure
; NOTE: Both easeIn and easeOut control the INCOMING segment (previous->current point), not the outgoing segment.
; For smooth transition through a point, set easeOut=false for the current point AND easeIn=false for the next point.
int Function AddTranslationPointAtRef(string modName, int timelineID, float time, ObjectReference reference, int bodyPart = 0, float offsetX = 0.0, float offsetY = 0.0, float offsetZ = 0.0, bool isOffsetRelative = false, bool easeIn = false, bool easeOut = false, int interpolationMode = 2) global native

; Add a translation point that captures camera position at the start of playback
; This point can be used to start playback smoothly from the last camera position, and return to it later.
; time: time in seconds when this point occurs
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to add the point to
; time: time in seconds when this point occurs
; easeIn: ease in at the start of interpolation
; easeOut: ease out at the end of interpolation
; interpolationMode: 0=None, 1=Linear, 2=CubicHermite (default)
; Returns: index of the added point, or -1 on failure
int Function AddTranslationPointAtCamera(string modName, int timelineID, float time, bool easeIn = false, bool easeOut = false, int interpolationMode = 2) global native

; Add a rotation point with specified pitch and yaw
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to add the point to
; time: time in seconds when this point occurs
; pitch: pitch in degrees relative to world coords
; yaw: yaw in degrees relative to world coords
; easeIn: ease in at the start of interpolation
; easeOut: ease out at the end of interpolation
; interpolationMode: 0=None, 1=Linear, 2=CubicHermite (default)
; Returns: index of the added point, or -1 on failure
; NOTE: Both easeIn and easeOut control the INCOMING segment (previous->current point), not the outgoing segment.
; For smooth transition through a point, set easeOut=false for the current point AND easeIn=false for the next point.
int Function AddRotationPoint(string modName, int timelineID, float time, float pitch, float yaw, bool easeIn = false, bool easeOut = false, int interpolationMode = 2) global native

; Add a rotation point that sets the rotation relative to camera-to-reference direction, or alternatively the ref's heading
; time: time in seconds when this point occurs
; reference: the object reference to track
; bodyPart: which body part to track rotation from (0=root, 1=eye, 2=head, 3=torso, default: 0=root)
; offsetPitch: pitch offset in degrees from camera-to-reference direction (isOffsetRelative == false) / the ref's heading (isOffsetRelative == true)
; offsetYaw: yaw offset in degrees. isOffsetRelative == false - offset from camera-to-reference direction. A value of 0 means looking directly at the reference.
;            isOffsetRelative == true - offset from reference's heading. A value of 0 means looking into the direction the ref is heading.
; isOffsetRelative: if true, offset is relative to reference's heading instead of camera-to-reference direction.
; easeIn: ease in at the start of interpolation
; easeOut: ease out at the end of interpolation
; interpolationMode: 0=None, 1=Linear, 2=CubicHermite (default)
; Returns: index of the added point on success, -1 on failure
; NOTE: Both easeIn and easeOut control the INCOMING segment (previous->current point), not the outgoing segment.
; For smooth transition through a point, set easeOut=false for the current point AND easeIn=false for the next point.
int Function AddRotationPointAtRef(string modName, int timelineID, float time, ObjectReference reference, int bodyPart = 0, float offsetPitch = 0.0, float offsetYaw = 0.0, bool isOffsetRelative = false, bool easeIn = false, bool easeOut = false, int interpolationMode = 2) global native

; Add a rotation point that captures camera rotation at the start of playback
; This point can be used to start playback smoothly from the last camera rotation, and return to it later.
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to add the point to
; time: time in seconds when this point occurs
; easeIn: ease in at the start of interpolation
; easeOut: ease out at the end of interpolation
; interpolationMode: 0=None, 1=Linear, 2=CubicHermite (default)
; Returns: index of the added point, or -1 on failure
; NOTE: Both easeIn and easeOut control the INCOMING segment (previous->current point), not the outgoing segment.
; For smooth transition through a point, set easeOut=false for the current point AND easeIn=false for the next point.
int Function AddRotationPointAtCamera(string modName, int timelineID, float time, bool easeIn = false, bool easeOut = false, int interpolationMode = 2) global native

; Add a FOV (Field of View) point to the timeline
; FOV controls camera zoom level in degrees (1-160, default 80)
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to add the point to
; time: time in seconds when this point occurs
; fov: field of view in degrees (1-160)
; easeIn: ease in at the start of interpolation
; easeOut: ease out at the end of interpolation
; interpolationMode: 0=None, 1=Linear, 2=CubicHermite (default)
; Returns: index of the added point, or -1 on failure
; NOTE: Both easeIn and easeOut control the INCOMING segment (previous->current point), not the outgoing segment.
; For smooth transition through a point, set easeOut=false for the current point AND easeIn=false for the next point.
int Function AddFOVPoint(string modName, int timelineID, float time, float fov, bool easeIn = false, bool easeOut = false, int interpolationMode = 2) global native

; Start recording camera movements to the timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to record to
; recordingInterval: time between samples in seconds. 0.0 = capture every frame (default: 1.0)
; append: if true, append to existing timeline; if false, clear timeline first (default: false)
; timeOffset: time offset in seconds added after the last existing point when appending (default: 0.0)
; Returns: true on success, false on failure
; When interval = 0.0 or negative: captures every frame (frame-rate dependent).
; When append=true: First recorded point placed at (lastPointTime + timeOffset). Uses easeIn=false.
; If timeline is empty when appending, starts at timeOffset with easeIn=false.
; When append=false: Clears timeline and starts at time 0.0 with easeIn=true.
bool Function StartRecording(string modName, int timelineID, float recordingInterval = 1.0, bool append = false, float timeOffset = 0.0) global native

; Stop recording camera movements on a timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to stop recording on
; Returns: true on success, false on failure
bool Function StopRecording(string modName, int timelineID) global native

; Remove a translation point from the timeline
; modName: name of your mod's ESP/ESL file
; timelineID: timeline ID to remove the point from
; index: index of the point to remove
; Returns: true if removed, false on failure
bool Function RemoveTranslationPoint(string modName, int timelineID, int index) global native

; Remove a rotation point from the timeline
; modName: name of your mod's ESP/ESL file
; timelineID: timeline ID to remove the point from
; index: index of the point to remove
; Returns: true if removed, false on failure
bool Function RemoveRotationPoint(string modName, int timelineID, int index) global native

; Remove a FOV point from the timeline
; modName: name of your mod's ESP/ESL file
; timelineID: timeline ID to remove the point from
; index: index of the point to remove
; Returns: true if removed, false on failure
bool Function RemoveFOVPoint(string modName, int timelineID, int index) global native

; Clear the entire camera timeline
; modName: name of your mod's ESP/ESL file
; timelineID: timeline ID to clear
; Returns: true if cleared, false on failure
bool Function ClearTimeline(string modName, int timelineID) global native

; Get the number of translation points in the timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; Returns: number of translation points, or -1 if timeline not found
int Function GetTranslationPointCount(string modName, int timelineID) global native

; Get the number of rotation points in the timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; Returns: number of rotation points, or -1 if timeline not found
int Function GetRotationPointCount(string modName, int timelineID) global native

; Get the number of FOV points in the timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; Returns: number of FOV points, or -1 if timeline not found
int Function GetFOVPointCount(string modName, int timelineID) global native

; Get the X coordinate of a translation point by index
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; index: 0-based index of the point
; Returns: X coordinate, or 0.0 if timeline not found or index out of range
float Function GetTranslationPointX(string modName, int timelineID, int index) global native

; Get the Y coordinate of a translation point by index
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; index: 0-based index of the point
; Returns: Y coordinate, or 0.0 if timeline not found or index out of range
float Function GetTranslationPointY(string modName, int timelineID, int index) global native

; Get the Z coordinate of a translation point by index
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; index: 0-based index of the point
; Returns: Z coordinate, or 0.0 if timeline not found or index out of range
float Function GetTranslationPointZ(string modName, int timelineID, int index) global native

; Get the pitch (in radians) of a rotation point by index
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; index: 0-based index of the point
; Returns: pitch in radians, or 0.0 if timeline not found or index out of range
float Function GetRotationPointPitch(string modName, int timelineID, int index) global native

; Get the yaw (in radians) of a rotation point by index
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; index: 0-based index of the point
; Returns: yaw in radians, or 0.0 if timeline not found or index out of range
float Function GetRotationPointYaw(string modName, int timelineID, int index) global native

; Get the FOV value of a FOV point by index
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; index: 0-based index of the point
; Returns: FOV in degrees, or 80.0 if timeline not found or index out of range
float Function GetFOVPoint(string modName, int timelineID, int index) global native

; Start playback of a camera path timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to play
; speed: playback speed multiplier, only used if useDuration=false (default: 1.0)
; globalEaseIn: apply global ease-in to entire playback (default: false)
; globalEaseOut: apply global ease-out to entire playback (default: false) 
; useDuration: if true, plays timeline over duration seconds
;              if false (default), plays timeline with speed as speed multiplier
; duration: total duration in seconds for entire timeline, only used if useDuration=true (default: 0.0)
; startTime: start playback at this time in seconds (default: 0.0 = start from beginning). Useful for resuming playback after save/load.
; Returns: true on success, false on failure
; NOTE: To control ground following, menu visibility, and user rotation, use the separate SetFollowGround(),
;       SetMenuVisibility(), and AllowUserRotation() functions before or during playback.
bool Function StartPlayback(string modName, int timelineID, float speed = 1.0, bool globalEaseIn = false, bool globalEaseOut = false, bool useDuration = false, float duration = 0.0, float startTime = 0.0) global native

; Stop playback of the camera timeline
; Stop playback of a camera path timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to stop
; Returns: true on success, false on failure
bool Function StopPlayback(string modName, int timelineID) global native

; Switch playback from one timeline to another without exiting free camera mode
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; fromTimelineID: source timeline ID (0 = any owned timeline currently playing)
; toTimelineID: target timeline ID to switch to
; Returns: true on successful switch, false on failure
bool Function SwitchPlayback(string modName, int fromTimelineID, int toTimelineID) global native

; Pause the camera timeline playback
; Pause playback of a camera path timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to pause
; Returns: true on success, false on failure
bool Function PausePlayback(string modName, int timelineID) global native

; Resume the camera timeline playback
; Resume playback of a camera path timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to resume
; Returns: true on success, false on failure
bool Function ResumePlayback(string modName, int timelineID) global native

; Check if a timeline is currently playing back
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to check
; Returns: true if playing, false otherwise
bool Function IsPlaybackRunning(string modName, int timelineID) global native

; Check if a timeline is currently recording
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to check
; Returns: true if recording, false otherwise
bool Function IsRecording(string modName, int timelineID) global native

; Check if timeline playback is currently paused
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to check
; Returns: true if paused, false otherwise
bool Function IsPlaybackPaused(string modName, int timelineID) global native

; Get the ID of the currently active timeline (recording or playing)
; Returns: timeline ID if active (>0), or 0 if no timeline is active
int Function GetActiveTimelineID() global native

; Get the current playback time for a timeline
; Useful for saving playback state to resume after loading a save
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; Returns: current playback time in seconds, or -1.0 if timeline not found or not owned
float Function GetPlaybackTime(string modName, int timelineID) global native

; Enable or disable user rotation control during playback for a specific timeline
; This setting is stored per-timeline and applied when that timeline is playing
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to configure
; allow: true to allow user rotation, false to disable
; Returns: true on success, false on failure
bool Function AllowUserRotation(string modName, int timelineID, bool allow) global native

; Check if user rotation is currently allowed for a specific timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to check
; Returns: true if user can control rotation, false otherwise
bool Function IsUserRotationAllowed(string modName, int timelineID) global native

; Enable or disable ground following for a timeline
; Ground following keeps the camera above the ground/water during playback
; Can be called before or during playback - takes effect immediately
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to configure
; follow: true to enable ground following, false to disable (default: true)
; minHeight: minimum height above ground/water when following (default: 0.0)
; Returns: true on success, false on failure
bool Function SetFollowGround(string modName, int timelineID, bool follow, float minHeight = 0.0) global native

; Check if ground following is enabled for a timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to check
; Returns: true if ground following enabled, false otherwise
bool Function IsGroundFollowingEnabled(string modName, int timelineID) global native

; Get the minimum height above ground for a timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; Returns: minimum height in game units, or -1.0 on error
float Function GetMinHeightAboveGround(string modName, int timelineID) global native

; Set menu visibility during timeline playback
; Controls whether menus are shown or hidden during playback
; Can be called before or during playback - takes effect immediately
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to configure
; show: true to show menus during playback, false to hide (default: false)
; Returns: true on success, false on failure
bool Function SetMenuVisibility(string modName, int timelineID, bool show) global native

; Check if menus are visible during timeline playback
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to check
; Returns: true if menus visible, false otherwise
bool Function AreMenusVisible(string modName, int timelineID) global native

; Set the playback mode and loop time offset for a timeline
; This determines what happens when the timeline reaches its end
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to configure
; playbackMode: 0=end (stop at end), 1=loop (wrap to beginning), 2=wait (stay at final position until StopPlayback is called)
; loopTimeOffset: Time offset in seconds when looping back (only used in loop mode, default: 0.0)
; Returns: true if successfully set, false on failure
bool Function SetPlaybackMode(string modName, int timelineID, int playbackMode, float loopTimeOffset = 0.0) global native

; Adds camera timeline imported from filePath at timeOffset to the specified timeline.
; modName: Name of your mod (case-sensitive, as defined in SKSE plugin)
; timelineID: ID of the timeline to add points to
; filePath: Relative path from Data folder (e.g., "SKSE/Plugins/MyTimeline.dat")
; timeOffset: Time offset to add to all imported point times (default 0.0)
; Returns: true if successful, false otherwise
bool Function AddTimelineFromFile(string modName, int timelineID, string filePath, float timeOffset = 0.0) global native

; Export the specified timeline to a file
; modName: Name of your mod (case-sensitive, as defined in SKSE plugin)
; timelineID: ID of the timeline to export
; filePath: Relative path from Data folder (e.g., "SKSE/Plugins/MyTimeline.dat")
; Returns: true if successful, false otherwise
bool Function ExportTimeline(string modName, int timelineID, string filePath) global native

; ===== Event Registration =====

; Register a form (Quest etc.) to receive timeline playback events
; The form's script must define these event handlers:
;   Event OnPlaybackStart(int timelineID)
;   Event OnPlaybackStop(int timelineID)
;   Event OnPlaybackWait(int timelineID)  ; For wait mode: timeline reached end and is waiting
; form: The form/alias to register (typically 'self' from a script)
Function RegisterForTimelineEvents(Form form) global native

; Unregister a form from receiving timeline playback events
; form: The form/alias to unregister
Function UnregisterForTimelineEvents(Form form) global native

; ===== Camera Utility Functions =====

; Get current camera X position (world coordinates)
; Returns: X coordinate of current camera position
float Function GetCameraPosX() global native

; Get current camera Y position (world coordinates)
; Returns: Y coordinate of current camera position
float Function GetCameraPosY() global native

; Get current camera Z position (world coordinates)
; Returns: Z coordinate of current camera position
float Function GetCameraPosZ() global native

; Get current camera pitch (rotation around X axis)
; Returns: Pitch in radians
float Function GetCameraPitch() global native

; Get current camera yaw (rotation around Z axis)
; Returns: Yaw in radians
float Function GetCameraYaw() global native
