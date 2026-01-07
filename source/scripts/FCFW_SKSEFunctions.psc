Scriptname FCFW_SKSEFunctions

; Get plugin version
; Encoded as: major * 10000 + minor * 100 + patch
; Example: version 1.2.3 returns 10203
int Function FCFW_GetPluginVersion() global native

; ===== Timeline Management =====

; Register a new timeline and get its unique ID
; Each mod can register multiple independent timelines
; IMPORTANT: Timeline IDs are permanent once registered. To update a timeline's
; content, use ClearTimeline() followed by Add...Point() calls. Only unregister
; when you no longer need the timeline at all (e.g., plugin shutdown).
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; Returns: new timeline ID (>0) on success, or -1 on failure
int Function FCFW_RegisterTimeline(string modName) global native

; Unregister a timeline and free its resources
; This will stop any active playback/recording on the timeline before removing it
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to unregister
; Returns: true if successfully unregistered, false on failure
bool Function FCFW_UnregisterTimeline(string modName, int timelineID) global native

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
int Function FCFW_AddTranslationPoint(string modName, int timelineID, float time, float posX, float posY, float posZ, bool easeIn = false, bool easeOut = false, int interpolationMode = 2) global native

; Add a translation point relative to a reference object
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to add the point to
; time: time in seconds when this point occurs
; reference: the object reference to track
; offsetX, offsetY, offsetZ: offset from reference position
; isOffsetRelative: if true, offset is relative to reference's heading (local space), otherwise world space
; easeIn: ease in at the start of interpolation
; easeOut: ease out at the end of interpolation
; interpolationMode: 0=None, 1=Linear, 2=CubicHermite (default)
; Returns: index of the added point, or -1 on failure
int Function FCFW_AddTranslationPointAtRef(string modName, int timelineID, float time, ObjectReference reference, float offsetX, float offsetY, float offsetZ, bool isOffsetRelative = false, bool easeIn = false, bool easeOut = false, int interpolationMode = 2) global native

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
int Function FCFW_AddTranslationPointAtCamera(string modName, int timelineID, float time, bool easeIn = false, bool easeOut = false, int interpolationMode = 2) global native

; Add a rotation point with specified pitch and yaw
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to add the point to
; time: time in seconds when this point occurs
; pitch: relative to world coords
; yaw: relative to world coords
; easeIn: ease in at the start of interpolation
; easeOut: ease out at the end of interpolation
; interpolationMode: 0=None, 1=Linear, 2=CubicHermite (default)
; Returns: index of the added point, or -1 on failure
int Function FCFW_AddRotationPoint(string modName, int timelineID, float time, float pitch, float yaw, bool easeIn = false, bool easeOut = false, int interpolationMode = 2) global native

; Add a rotation point that sets the rotation relative to camera-to-reference direction, or alternatively the ref's heading
; time: time in seconds when this point occurs
; reference: the object reference to track
; offsetPitch: pitch offset from camera-to-reference direction (isOffsetRelative == false) / the ref's heading (isOffsetRelative == true)
; offsetYaw: isOffsetRelative == false - yaw offset from camera-to-reference direction. A value of 0 means looking directly at the reference.
;            isOffsetRelative == true - yaw offset from reference's heading. A value of 0 means looking into the direction the ref is heading.
; isOffsetRelative: if true, offset is relative to reference's heading instead of camera-to-reference direction.
; easeIn: ease in at the start of interpolation
; easeOut: ease out at the end of interpolation
; interpolationMode: 0=None, 1=Linear, 2=CubicHermite (default)
; Returns: index of the added point on success, -1 on failure
int Function FCFW_AddRotationPointAtRef(string modName, int timelineID, float time, ObjectReference reference, float offsetPitch, float offsetYaw, bool isOffsetRelative = false, bool easeIn = false, bool easeOut = false, int interpolationMode = 2) global native

; Add a rotation point that captures camera rotation at the start of playback
; This point can be used to start playback smoothly from the last camera rotation, and return to it later.
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to add the point to
; time: time in seconds when this point occurs
; easeIn: ease in at the start of interpolation
; easeOut: ease out at the end of interpolation
; interpolationMode: 0=None, 1=Linear, 2=CubicHermite (default)
; Returns: index of the added point, or -1 on failure
int Function FCFW_AddRotationPointAtCamera(string modName, int timelineID, float time, bool easeIn = false, bool easeOut = false, int interpolationMode = 2) global native

; Start recording camera movements to the timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to record to
; Returns: true on success, false on failure
bool Function FCFW_StartRecording(string modName, int timelineID) global native

; Stop recording camera movements on a timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to stop recording on
; Returns: true on success, false on failure
bool Function FCFW_StopRecording(string modName, int timelineID) global native

; Remove a translation point from the timeline
; modName: name of your mod's ESP/ESL file
; timelineID: timeline ID to remove the point from
; index: index of the point to remove
; Returns: true if removed, false on failure
bool Function FCFW_RemoveTranslationPoint(string modName, int timelineID, int index) global native

; Remove a rotation point from the timeline
; modName: name of your mod's ESP/ESL file
; timelineID: timeline ID to remove the point from
; index: index of the point to remove
; Returns: true if removed, false on failure
bool Function FCFW_RemoveRotationPoint(string modName, int timelineID, int index) global native

; Clear the entire camera timeline
; modName: name of your mod's ESP/ESL file
; timelineID: timeline ID to clear
; notifyUser: whether to show a notification to the user
; Returns: true if cleared, false on failure
bool Function FCFW_ClearTimeline(string modName, int timelineID, bool notifyUser = true) global native

; Get the number of translation points in the timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; Returns: number of translation points, or -1 if timeline not found
int Function FCFW_GetTranslationPointCount(string modName, int timelineID) global native

; Get the number of rotation points in the timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; Returns: number of rotation points, or -1 if timeline not found
int Function FCFW_GetRotationPointCount(string modName, int timelineID) global native

; Get the X coordinate of a translation point by index
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; index: 0-based index of the point
; Returns: X coordinate, or 0.0 if timeline not found or index out of range
float Function FCFW_GetTranslationPointX(string modName, int timelineID, int index) global native

; Get the Y coordinate of a translation point by index
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; index: 0-based index of the point
; Returns: Y coordinate, or 0.0 if timeline not found or index out of range
float Function FCFW_GetTranslationPointY(string modName, int timelineID, int index) global native

; Get the Z coordinate of a translation point by index
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; index: 0-based index of the point
; Returns: Z coordinate, or 0.0 if timeline not found or index out of range
float Function FCFW_GetTranslationPointZ(string modName, int timelineID, int index) global native

; Get the pitch (in radians) of a rotation point by index
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; index: 0-based index of the point
; Returns: pitch in radians, or 0.0 if timeline not found or index out of range
float Function FCFW_GetRotationPointPitch(string modName, int timelineID, int index) global native

; Get the yaw (in radians) of a rotation point by index
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to query
; index: 0-based index of the point
; Returns: yaw in radians, or 0.0 if timeline not found or index out of range
float Function FCFW_GetRotationPointYaw(string modName, int timelineID, int index) global native

; Start playback with advanced options
; speed: playback speed multiplier (only used if useDuration=false)
; useDuration: if true, plays timeline over duration seconds
;              if false, plays timeline with speed as speed multiplier
; duration: total duration in seconds for entire timeline (only used if useDuration=true)
; globalEaseIn: apply ease-in at the start of entire playback
; globalEaseOut: apply ease-out at the end of entire playback
; Start playback of a camera path timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to play
; speed: playback speed multiplier, only used if useDuration=false (default: 1.0)
; globalEaseIn: apply global ease-in to entire playback (default: false)
; globalEaseOut: apply global ease-out to entire playback (default: false) 
; useDuration: if true, plays timeline over duration seconds
;              if false (default), plays timeline with speed as speed multiplier
; duration: total duration in seconds for entire timeline, only used if useDuration=true (default: 0.0)
; Returns: true on success, false on failure
bool Function FCFW_StartPlayback(string modName, int timelineID, float speed = 1.0, bool globalEaseIn = false, bool globalEaseOut = false, bool useDuration = false, float duration = 0.0) global native

; Stop playback of the camera timeline
; Stop playback of a camera path timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to stop
; Returns: true on success, false on failure
bool Function FCFW_StopPlayback(string modName, int timelineID) global native

; Switch playback from one timeline to another without exiting free camera mode
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; fromTimelineID: source timeline ID (0 = any owned timeline currently playing)
; toTimelineID: target timeline ID to switch to
; Returns: true on successful switch, false on failure
bool Function FCFW_SwitchPlayback(string modName, int fromTimelineID, int toTimelineID) global native

; Pause the camera timeline playback
; Pause playback of a camera path timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to pause
; Returns: true on success, false on failure
bool Function FCFW_PausePlayback(string modName, int timelineID) global native

; Resume the camera timeline playback
; Resume playback of a camera path timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to resume
; Returns: true on success, false on failure
bool Function FCFW_ResumePlayback(string modName, int timelineID) global native

; Check if a timeline is currently playing back
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to check
; Returns: true if playing, false otherwise
bool Function FCFW_IsPlaybackRunning(string modName, int timelineID) global native

; Check if a timeline is currently recording
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to check
; Returns: true if recording, false otherwise
bool Function FCFW_IsRecording(string modName, int timelineID) global native

; Check if timeline playback is currently paused
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to check
; Returns: true if paused, false otherwise
bool Function FCFW_IsPlaybackPaused(string modName, int timelineID) global native

; Get the ID of the currently active timeline (recording or playing)
; Returns: timeline ID if active (>0), or 0 if no timeline is active
int Function FCFW_GetActiveTimelineID() global native

; Enable or disable user rotation control during playback for a specific timeline
; This setting is stored per-timeline and applied when that timeline is playing
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to configure
; allow: true to allow user rotation, false to disable
Function FCFW_AllowUserRotation(string modName, int timelineID, bool allow) global native

; Check if user rotation is currently allowed for a specific timeline
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to check
; Returns: true if user can control rotation, false otherwise
bool Function FCFW_IsUserRotationAllowed(string modName, int timelineID) global native

; Set the playback mode for a timeline
; This determines what happens when the timeline reaches its end
; modName: name of your mod's ESP/ESL file (e.g., "MyMod.esp")
; timelineID: timeline ID to configure
; playbackMode: 0=kEnd (stop at end), 1=kLoop (wrap to beginning), 2=kWait (stay at final position until StopPlayback is called)
; Returns: true if successfully set, false on failure
bool Function FCFW_SetPlaybackMode(string modName, int timelineID, int playbackMode) global native

; Adds camera timeline imported from filePath at timeOffset to the specified timeline.
; modName: Name of your mod (case-sensitive, as defined in SKSE plugin)
; timelineID: ID of the timeline to add points to
; filePath: Relative path from Data folder (e.g., "SKSE/Plugins/MyTimeline.dat")
; timeOffset: Time offset to add to all imported point times (default 0.0)
; Returns: true if successful, false otherwise
bool Function FCFW_AddTimelineFromFile(string modName, int timelineID, string filePath, float timeOffset = 0.0) global native

; Export the specified timeline to a file
; modName: Name of your mod (case-sensitive, as defined in SKSE plugin)
; timelineID: ID of the timeline to export
; filePath: Relative path from Data folder (e.g., "SKSE/Plugins/MyTimeline.dat")
; Returns: true if successful, false otherwise
bool Function FCFW_ExportTimeline(string modName, int timelineID, string filePath) global native

; ===== Event Registration =====

; Register a form (Quest, ReferenceAlias, etc.) to receive timeline playback events
; The form's script must define these event handlers:
;   Event OnTimelinePlaybackStarted(int timelineID)
;   Event OnTimelinePlaybackStopped(int timelineID)
;   Event OnTimelinePlaybackCompleted(int timelineID)  ; For kWait mode: timeline reached end and is waiting
; form: The form/alias to register (typically 'self' from a script)
Function FCFW_RegisterForTimelineEvents(Form form) global native

; Unregister a form from receiving timeline playback events
; form: The form/alias to unregister
Function FCFW_UnregisterForTimelineEvents(Form form) global native
