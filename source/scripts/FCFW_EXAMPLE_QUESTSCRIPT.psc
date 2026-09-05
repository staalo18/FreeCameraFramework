Scriptname FCFW_EXAMPLE_QUESTSCRIPT extends Quest

; ============================================================================
; FCFW Example Script - Multi-Timeline Camera System
; ============================================================================
; This script demonstrates how to use the FreeCameraFramework API with
; multiple independent timeline sequences. Five hotkeys control different
; camera behaviors:
;
; Key '1' - Cinematic Orbit Sequence:
;   - Timeline 1: Smooth approach from current camera position to orbit entry
;   - Timeline 2: Continuous circular orbit around marker (looping). 
;                 During this stage the user can rotate the camera freely while it is orbiting.
;   - Timeline 3: Return to initial camera position (triggered by toggling key '1' again)
;
; Key '2' - Playback timeline sequence  :
;   - Timeline 4: Plays back camera movement from recording (via key '3'), or from imported YAML (via key '5')
;   - Timeline 5: Returns to initial position after playback ends
;
; Key '3' - Recording Mode:
;   - Records camera movement in real-time to Timeline 4
;   - Can be exported to a YAML file via key '4', and played back via Key '2'
;
; Key '4' - Export Recorded Timeline to YAML File:
;   - Exports Timeline 4 to a YAML file for external editing and later import/use
;   - Exported file can be found in the Skyrim Data folder
;
; Key '5' - Import Recorded Timeline from YAML File:
;   - Imports timeline from YAML file into Timeline 4
;   - Play back imported timeline via Key '2'
;
; Features demonstrated:
; - Multi-timeline registration and management (5 independent timelines)
; - Procedural path generation (circular orbit with reference-based positioning)
; - Dynamic timeline building (capturing camera state at runtime)
; - Real-time camera path recording
; - Possibility to let the user control camera rotation during playback
; - Exporting and importing timelines to/from YAML files
; - Automatic timeline switching via OnPlaybackWait events
; - Manual playback interruption with smooth return paths
; - Looping vs. wait playback modes
; - Event handling for playback state tracking
; - Save/load state preservation example (playback state is saved before save and restored after load)
;
; First exercise: Change the reference in markerAlias to an actor reference (other than the player)
;                 and watch how the camera orbits around that actor, while the actor keeps moving.
;                 Don't forget to remove the marker.MoveTo() statement in BuildTimeline2() if you do this!
; Second exercise: Pick some of the timeline YAMLs provided in the Documentation/TimelineFileExample folder,
;                  change the references in the YAML to  a reference of your choice,
;                  import them via key '5' (don't forget to rename), and then play them back via key '2'.
;                  Eg: FCFW_Timeline_orbitReference.yaml - a looping orbit around a reference
;                      FCFW_Timeline_trackReferenceAndReturn.yaml - move camera to reference, track it for a while, then return
;                      FCFW_Timeline_worldPath.yaml - a pre-recorded world-space camera path with wait at end
;                  The format description for timeline YAML files can be found in TIMELINE_FORMAT.md.
;
; For a complete FreeCameraFramework API description, see Source/Scripts/FCFW_SKSEFunctions.psc.
;     You need to include this file (and the compiled script Scripts/FCFW_SKSEFunctions.pex) in your mod if you want to use FCFW's functions. 
;
; For a detailed reference on FreeCameraFramework and its usage, see Documentation/FCFW_DEVELOPER_REFERENCE.md.
;
; ============================================================================


ReferenceAlias Property markerAlias Auto ; the marker alias used as the center of the orbit
ReferenceAlias Property refAlias Auto ; the reference alias used to anchor the marker

string ModName = "FCFW_EXAMPLE.esp" ; The mod name registered with SKSE - must match your ESP/ESL filename
string TimelineFilename = "FCFW_EXAMPLE_TIMELINE.yaml" ; Filename for timeline export/import

; control keys for the FCFW Example mod
int togglePlayback1_Key = 2 ; "1"
int togglePlayback2_Key = 3 ; "2"
int toggleRecording_Key = 4 ; "3"
int exportTimeline_Key = 5 ; "4"
int importTimeline_Key = 6 ; "5
int toggleDollyZoom_Key = 7 ; "6"

; timeline configuration
float CircleRadius = 2000.0 ; radius of orbit around marker
float PositionOffset = 500.0 ; position offset of marker relative to reference
int NumPoints = 16 ; number of sample points in orbit
float Duration = 8.0 ; time for one complete rotation (seconds)
float transitionTime = 2.0 ; time to transition to new position

; timeline IDs
int timeline1ID = -1
int timeline2ID = -1
int timeline3ID = -1
int timeline4ID = -1
int timeline5ID = -1
int dollyZoomTimelineID = -1
int currentTimelineID = -1

; ===== Save/Load State Preservation =====
; These properties demonstrate how to preserve playback state across saves
; The properties are automatically saved/loaded by Skyrim's save system
int savedActiveTimeline = -1      ; Which timeline was playing when saved
float savedPlaybackTime = -1.0      ; Where in the timeline playback was
float savedMinHeightAboveGround = 0.0


Event OnInit()
    Initalize()
EndEvent


Function Initalize()
    InitializeKeys()
    IntializeTimelines()
EndFunction


Function InitializeKeys()
    RegisterForKey(togglePlayback1_Key)
    RegisterForKey(togglePlayback2_Key)
    RegisterForKey(toggleRecording_Key)
    RegisterForKey(exportTimeline_Key)
    RegisterForKey(importTimeline_Key)
    RegisterForKey(toggleDollyZoom_Key)
endFunction


Function IntializeTimelines()
    ; Stop any existing update loops from previous session
    UnregisterForUpdate()
    
    currentTimelineID = -1 ; no active timeline at start
    FCFW_SKSEFunctions.RegisterPlugin(ModName)
    timeline1ID = RegisterTimeline()
    timeline2ID = RegisterTimeline()
    timeline3ID = RegisterTimeline()
    timeline4ID = RegisterTimeline()
    timeline5ID = RegisterTimeline()
    dollyZoomTimelineID = RegisterTimeline()

    FCFW_SKSEFunctions.RegisterForTimelineEvents(self as Form)
EndFunction


int Function RegisterTimeline()
    int id = FCFW_SKSEFunctions.RegisterTimeline(ModName)
    if id <= 0
        Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to register timeline")
        Debug.Notification("FCFW Example: Failed to register timeline")
    endif
    return id
EndFunction


Function BuildTimeline1()
    ; timeline 1 defines the approach from the current camera position
    ; to the start of the circular orbit path around the marker

    if timeline1ID <= 0 ; assume that timeline1ID has been registered already
        Debug.Trace("FCFW_EXAMPLE: ERROR - timeline1ID not valid!")
        return
    endif    

    ; Clear any existing points
    FCFW_SKSEFunctions.ClearTimeline(ModName, timeline1ID)

    if !markerAlias || !refAlias
        Debug.Trace("FCFW_EXAMPLE: ERROR -  aliases not property not set!")
        return
    endif

    ObjectReference ref = refAlias.GetReference()
    ObjectReference marker = markerAlias.GetReference()

    if !ref || !marker
        Debug.Trace("FCFW_EXAMPLE: ERROR - Could not get references for aliases!")
    endif
    
    ; Set to wait mode (playbackMode = 2) so camera waits at end of timeline
    ; This allows for switching to next timeline in OnPlaybackWait()
    FCFW_SKSEFunctions.SetPlaybackMode(ModName, timeline1ID, playbackMode = 2)
    
    ; add translation point at camera position (determined dynamically at start of playback)
    ; apply easing to start and end of transition
    if FCFW_SKSEFunctions.AddTranslationPointAtCamera(ModName, timeline1ID, time = 0.0) < 0
        Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to add initial translation point at camera") 
    endif

    ; Add translation point at first point of timeline 2 (entry point for timeline 2 circular path)    
    if FCFW_SKSEFunctions.AddTranslationPointAtRef(ModName, timeline1ID, time = transitionTime, reference = marker, bodyPart = 0, offsetX = CircleRadius, offsetY = 0.0, offsetZ = 0.0, isOffsetRelative = true, easeIn = true, easeOut = false) < 0
        Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to add translation point")
    endif

    ; add rotation point at camera position (determined dynamically at start of playback)
    if FCFW_SKSEFunctions.AddRotationPointAtCamera(ModName, timeline1ID, time = 0.0) < 0
        Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to add initial rotation point at camera")
    endif

    ; Add rotation point matching timeline 2 start
    if FCFW_SKSEFunctions.AddRotationPointAtRef(ModName, timeline1ID, time = transitionTime, reference = ref, offsetPitch = 0.0, offsetYaw = 0.0, interpolationMode = 1) < 0
        Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to add rotation point")
    endif
EndFunction


Function BuildTimeline2()
    ; timeline 2 defines the orbiting path around the marker 

    if timeline2ID <= 0 ; assume that timeline2ID has been registered already
        Debug.Trace("FCFW_EXAMPLE: ERROR - timeline2ID not valid!")
        return
    endif

    ; Clear any existing points
    FCFW_SKSEFunctions.ClearTimeline(ModName, timeline2ID)
    
    ; move marker to reference position with offset
    if !markerAlias || !refAlias
        Debug.Trace("FCFW_EXAMPLE: ERROR -  aliases not property not set!")
        return
    endif

    ObjectReference ref = refAlias.GetReference()
    ObjectReference marker = markerAlias.GetReference()

    if ref && marker
        marker.MoveTo(ref, -PositionOffset, 0.0 , PositionOffset)
        marker.SetAngle(60.0, 0.0, 135.0) ; add tilt to the marker to get tilted orbit
    else
        Debug.Trace("FCFW_EXAMPLE: ERROR - Could not get references for aliases!")
    endif

    ; Build circular camera path around marker
    
    float timeStep = Duration / NumPoints ; time between timeline points

    ; Set to loop mode so camera continuously orbits (playbackMode = 1)
    ; loopTimeOffset = timeStep to ensure smooth looping from last to first point
    FCFW_SKSEFunctions.SetPlaybackMode(ModName, timeline2ID, playbackMode = 1, loopTimeOffset = timeStep)

    ; Allow free user rotation during orbiting
    FCFW_SKSEFunctions.AllowUserRotation(ModName, timeline2ID, allow = true)
        
    ; Generate points in a circle around the marker using reference-based positioning
    ; This allows the camera to follow the marker if it moves during playback
    int i = 0
    while i < NumPoints
        float angle = (i as float / NumPoints as float) * 360.0
        float time = i * timeStep
        
        ; Calculate offset on circle (relative to ref)
        float offsetX = CircleRadius * Math.Cos(angle)
        float offsetY = CircleRadius * Math.Sin(angle)
        float offsetZ = 0.0

        ; Add translation point relative to marker
        ; isOffsetRelative = true means offset is relative to marker's position
        if FCFW_SKSEFunctions.AddTranslationPointAtRef(ModName, timeline2ID, time, marker, bodyPart = 0, offsetX = offsetX, offsetY = offsetY, offsetZ = offsetZ, isOffsetRelative = true) < 0
            Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to add translation point " + i)
        endif
        
        i += 1
    endWhile

    ; Add rotation point to look at the ref
    ; offsetPitch = 0, offsetYaw = 0 means look directly at the ref
    if FCFW_SKSEFunctions.AddRotationPointAtRef(ModName, timeline2ID, time = 0.0, reference = ref, offsetPitch = 0.0, offsetYaw = 0.0, interpolationMode = 1) < 0
        Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to add rotation point " + i)
    endif
    
EndFunction


Function BuildReturnTimeline(int timelineID)
    ; defines the return path to the original camera position

    if timelineID <= 0 ; assume that timelineID has been registered already
        Debug.Trace("FCFW_EXAMPLE: ERROR - timelineID not valid!")
        return
    endif

    ; Clear any existing points
    FCFW_SKSEFunctions.ClearTimeline(ModName, timelineID)

    ; obtain current camera position and rotation
    float cameraPosX = FCFW_SKSEFunctions.GetCameraPosX()
    float cameraPosY = FCFW_SKSEFunctions.GetCameraPosY()
    float cameraPosZ = FCFW_SKSEFunctions.GetCameraPosZ()
    float cameraPitch = FCFW_SKSEFunctions.GetCameraPitch()
    float cameraYaw = FCFW_SKSEFunctions.GetCameraYaw()

    ; first point is a camera point. It uses the camera position at the time the playback of this timeline starts
    if FCFW_SKSEFunctions.AddTranslationPointAtCamera(ModName, timelineID, time = 0.0, easeIn = false, easeOut = true) < 0
        Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to add initial translation point at camera") 
    endif

    ; second point is at the current camera position to create a smooth transition back to the original position
    if FCFW_SKSEFunctions.AddTranslationPoint(ModName, timelineID, time = transitionTime, posX = cameraPosX, posY = cameraPosY, posZ = cameraPosZ, easeIn = true, easeOut = true) < 0
        Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to add initial translation point at camera position (" + cameraPosX + ", " + cameraPosY + ", " + cameraPosZ + ")")
    endif

    ; first point is a camera point. It uses the camera rotation at the time the playback of this timeline starts
    if FCFW_SKSEFunctions.AddRotationPointAtCamera(ModName, timelineID, time = 0.0, easeIn = false, easeOut = true) < 0
        Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to add initial rotation point at camera")
    endif

    ; second point is at the current camera rotation to create a smooth transition back to the original rotation
    if FCFW_SKSEFunctions.AddRotationPoint(ModName, timelineID, time = transitionTime, pitch = cameraPitch, yaw = cameraYaw, easeIn = true, easeOut = true) < 0
        Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to add initial rotation point at camera rotation (" + cameraPitch + ", " + cameraYaw + ")")
    endif

EndFunction


Event OnKeyDown(int keyCode)
    if keyCode == togglePlayback1_Key
        ; playback 1: toggling moves the camera from its current position to an orbit around the marker, and keeps orbiting until
        ;             toggling again, which moves the camera back to its original position and ends the playback
        ;
        ;      timeline1ID: move from current camera position to orbit start
        ;      timeline2ID: orbit around marker (looping) - triggered via OnPlaybackWait() at the end of timeline1ID
        ;      timeline3ID: move back to original camera position - triggered via toggling togglePlayback1_Key while timeline1ID or timeline2ID is playing
        
        if timeline1ID <= 0 || timeline2ID <= 0 || timeline3ID <= 0
            Debug.Trace("FCFW_EXAMPLE: ERROR - Timelines not registered!")
            Debug.Notification("FCFW Example: Timelines not registered!")
            return
        endif
        
        ; Toggle playback
        if currentTimelineID < 0 ; not currently playing, start playback

            ; build timelines dynamically to capture camera position at start of playback
            BuildTimeline1()
            BuildTimeline2() 
            BuildReturnTimeline(timeline3ID)

            FCFW_SKSEFunctions.SetFollowGround(ModName, timeline1ID, follow = true, minHeight = 100.0)

            ; Start playback
            if !FCFW_SKSEFunctions.StartPlayback(ModName, timeline1ID)
                Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to start playback")
                Debug.Notification("FCFW Example: Failed to start playback")
            endif
        else ; switch to timeline 3 to stop playback and return to original camera position
            if currentTimelineID == timeline1ID || currentTimelineID == timeline2ID
                if !FCFW_SKSEFunctions.SwitchPlayback(ModName, currentTimelineID, timeline3ID)
                    Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to stop playback")
                    Debug.Notification("FCFW Example: Failed to stop playback")
                endif
            endif
        endif
    elseif keyCode == togglePlayback2_Key
        ; playback 2: toggling starts playback of recorded camera movement, with return to the original camera position
        ;
        ;     timeline4ID: recorded camera movement
        ;     timeline5ID: return to original camera position after timeline4ID ends (triggered via OnPlaybackWait())
        
        if timeline4ID <= 0 || timeline5ID <= 0
            Debug.Trace("FCFW_EXAMPLE: ERROR - Timelines not registered!")
            Debug.Notification("FCFW Example: Timelines not registered!")
            return
        endif

        if FCFW_SKSEFunctions.GetTranslationPointCount(ModName, timeline4ID) == 0
            Debug.Trace("FCFW_EXAMPLE: ERROR - No points in timeline4ID!")
            Debug.Notification("FCFW Example: No points to play back!")
            return
        endif
        ; Toggle playback
        if currentTimelineID < 0 ; not currently in playback, start playback of recorded timeline
            ; dynamically build return timeline to capture camera position at start of playback
            BuildReturnTimeline(timeline5ID)                
                
            ; Set to wait mode (playbackMode = 2) so camera waits at end of timeline
            ; This allows for switching to timeline5 in OnPlaybackWait()
            FCFW_SKSEFunctions.SetPlaybackMode(ModName, timeline4ID, playbackMode = 2)

            if !FCFW_SKSEFunctions.StartPlayback(ModName, timeline4ID)
                Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to start playback")
                Debug.Notification("FCFW Example: Failed to start playback")
            endif
        endif
    elseif keyCode == toggleRecording_Key
        ; Toggle recording on/off for timeline 4

        if currentTimelineID > 0 ; not in playback
            Debug.Trace("FCFW_EXAMPLE: ERROR - Cannot toggle recording during playback!")
            return
        endif

        ; Toggle recording on timeline 4
        if timeline4ID <= 0 || timeline5ID <= 0
            Debug.Trace("FCFW_EXAMPLE: ERROR - Timelines not registered!")
            Debug.Notification("FCFW Example: Timelines not registered!")
            return
        endif

        ; toggle recording
        if FCFW_SKSEFunctions.IsRecording(ModName, timeline4ID)
            ; stop recording
            if FCFW_SKSEFunctions.StopRecording(ModName, timeline4ID)
                Debug.Trace("FCFW_EXAMPLE: Stopped recording on timeline 4")
                Debug.Notification("FCFW Example: Recording stopped")
            else
                Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to stop recording")
                Debug.Notification("FCFW Example: Failed to stop recording")
            endif
        else
            ; start recording
            FCFW_SKSEFunctions.ClearTimeline(ModName, timeline4ID)
            
            if FCFW_SKSEFunctions.StartRecording(ModName, timeline4ID)
                Debug.Trace("FCFW_EXAMPLE: Started recording on timeline 4")
                Debug.Notification("FCFW Example: Recording started")
            else
                Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to start recording")
                Debug.Notification("FCFW Example: Failed to start recording")
            endif
        endif
    elseif keyCode == exportTimeline_Key
        ; Export timeline 4 to YAML file

        if timeline4ID <= 0
            Debug.Trace("FCFW_EXAMPLE: ERROR - Timeline 4 not registered!")
            Debug.Notification("FCFW Example: Timeline not registered!")
            return
        endif

        if FCFW_SKSEFunctions.GetTranslationPointCount(ModName, timeline4ID) == 0
            Debug.Trace("FCFW_EXAMPLE: ERROR - No points in timeline 4 to export!")
            Debug.Notification("FCFW Example: No timeline data to export!")
            return
        endif

        if FCFW_SKSEFunctions.ExportTimeline(ModName, timeline4ID, TimelineFilename)
            Debug.Trace("FCFW_EXAMPLE: Successfully exported timeline to " + TimelineFilename)
            Debug.Notification("FCFW Example: Timeline exported to " + TimelineFilename)
        else
            Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to export timeline 4")
            Debug.Notification("FCFW Example: Failed to export timeline!")
        endif
    elseif keyCode == importTimeline_Key
        ; Import timeline from YAML file into timeline 4

        if timeline4ID <= 0
            Debug.Trace("FCFW_EXAMPLE: ERROR - Timeline not registered!")
            Debug.Notification("FCFW Example: Timeline not registered!")
            return
        endif

        ; Clear existing timeline data
        FCFW_SKSEFunctions.ClearTimeline(ModName, timeline4ID)

        if FCFW_SKSEFunctions.AddTimelineFromFile(ModName, timeline4ID, TimelineFilename)
            Debug.Trace("FCFW_EXAMPLE: Successfully imported timeline from " + TimelineFilename)
            Debug.Notification("FCFW Example: Timeline imported successfully!")
        else
            Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to import timeline from " + TimelineFilename)
            Debug.Notification("FCFW Example: Failed to import timeline!")
        endif
    elseif keyCode == toggleDollyZoom_Key
        ; Trigger a dolly zoom effect in front of the player

        float offsetStart = 1000.0
        float offsetEnd = 100.0
        float FOVStart = 15.0
        float zoomTime = 10.0

        DollyZoom(offsetStart, offsetEnd, FOVStart, zoomTime)   
    endif
EndEvent


Event OnPlaybackStart(int eventtimelineID)
    if eventtimelineID == timeline1ID || eventtimelineID == timeline2ID || \
       eventtimelineID == timeline3ID || eventtimelineID == timeline4ID || \
       eventtimelineID == timeline5ID || eventtimelineID == dollyZoomTimelineID
        
        currentTimelineID = eventtimelineID
        Debug.Trace("FCFW_EXAMPLE: Playback started for timeline " + eventtimelineID)

        UpdatePlaybackState() ; store initial playback state 
        
        ; Unregister any existing updates first to prevent multiple concurrent loops (eg leftover timelines from before loading current save)
        UnregisterForUpdate()
        RegisterForSingleUpdate(0.5) ; start a new update loop to document timeline progress for potential savegame events during playback
    endif
EndEvent

Event OnPlaybackStop(int eventtimelineID)
    if eventtimelineID == timeline1ID || eventtimelineID == timeline2ID || \
       eventtimelineID == timeline3ID || eventtimelineID == timeline4ID || \
       eventtimelineID == timeline5ID || eventtimelineID == dollyZoomTimelineID
       
        Debug.Trace("FCFW_EXAMPLE: Playback stopped for timeline " + eventtimelineID)
        
        ; Stop the update loop since playback ended
        UnregisterForUpdate()
        
        currentTimelineID = -1
    endif
EndEvent

Event OnPlaybackWait(int eventtimelineID)
    ; This fires when timeline reaches the end in playbackMode = 1 (wait) 

    if eventtimelineID == timeline1ID ; reached end of timeline 1
        if FCFW_SKSEFunctions.SwitchPlayback(ModName, timeline1ID, timeline2ID)
            currentTimelineID = timeline2ID
        else
            Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to switch playback")
            Debug.Notification("FCFW Example: Failed to switch playback")
            return
        endif
    elseif eventtimelineID == timeline4ID ; reached end of timeline 4
        if FCFW_SKSEFunctions.SwitchPlayback(ModName, timeline4ID, timeline5ID)
            currentTimelineID = timeline5ID
        else
            Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to switch playback")
            Debug.Notification("FCFW Example: Failed to switch playback")
            return
        endif
    endif
EndEvent

Event OnUpdate()
    if currentTimelineID > 0
        UpdatePlaybackState() ; store current playback state 
        RegisterForSingleUpdate(0.5) ; continue update loop while playback is active
    else
        ; Safety: if currentTimelineID is invalid, stop the update loop
        UnregisterForUpdate()
    endif
EndEvent

; ============================================================================
; Save/Load State Preservation Example
; ============================================================================
; These functions demonstrate how to preserve timeline playback state across
; save/load cycles. This is useful when the player saves during active playback.
;
; The workflow is:
; 1. UpdatePlaybackState stores current playback state in Papyrus properties in regular intervals
; 2. If user triggers a save, the current playback state parameters are saved automatically 
; 3. After load: OnPlayerLoadGame() (in the player script) calls RestorePlaybackState()
; 4. If timeline was active, resume playback at saved time
;
; ============================================================================

Function UpdatePlaybackState()
    ; Store away the current playback state
    ; This would typically be called before the game saves
    
    savedActiveTimeline = -1
    if currentTimelineID > 0
        ; Store which timeline is active and its progress
        if currentTimelineID == timeline1ID
            savedActiveTimeline = 1
            savedMinHeightAboveGround = 100.0
        elseif currentTimelineID == timeline2ID
            savedActiveTimeline = 2
            savedMinHeightAboveGround = 100.0
        elseif currentTimelineID == timeline3ID
            savedActiveTimeline = 3
            savedMinHeightAboveGround = 100.0
        elseif currentTimelineID == timeline4ID
            savedActiveTimeline = 4
            savedMinHeightAboveGround = 0.0
        elseif currentTimelineID == timeline5ID
            savedActiveTimeline = 5
            savedMinHeightAboveGround = 0.0
        elseif currentTimelineID == dollyZoomTimelineID
            savedActiveTimeline = 6
            savedMinHeightAboveGround = 0.0
        else 
            Debug.Trace("FCFW_EXAMPLE: ERROR - Unknown active timeline ID " + currentTimelineID)
            return
        endif
        
        savedPlaybackTime = FCFW_SKSEFunctions.GetPlaybackTime(ModName, currentTimelineID)
    endif
EndFunction

; ============================================================================
; Dolly Zoom
; ============================================================================
; Performs a dolly zoom camera movement facing the player.
;
; The dolly zoom (aka "Hitchcock zoom") moves the camera toward or away from
; the player while simultaneously adjusting the FOV so that the player's
; apparent size stays constant. This creates a striking depth-compression or
; depth-expansion effect on the background.
;
; Parameters:
;   offsetStart  - starting distance of camera from player (Skyrim units)
;   offsetEnd    - ending distance of camera from player (Skyrim units, must not be 0)
;   FOVStart     - starting field of view in degrees
;   zoomTime     - duration of the zoom in seconds
;
; The invariant maintained throughout: offset = width / (2 * tan(0.5 * FOV))
; where width = 2 * offsetStart * tan(0.5 * FOVStart) is constant.
;
; The camera is placed in front of the player (in the direction the player
; faces) and looks back at the player throughout the animation.
; ============================================================================

Function DollyZoom(float offsetStart, float offsetEnd, float FOVStart, float zoomTime)
    if dollyZoomTimelineID <= 0
        Debug.Trace("FCFW_EXAMPLE: DollyZoom - timeline not registered!")
        Debug.Notification("FCFW Example: DollyZoom failed - timeline not registered!")
        return
    endif

    if offsetEnd <= 0.0
        Debug.Trace("FCFW_EXAMPLE: DollyZoom - offsetEnd must be greater than 0!")
        Debug.Notification("FCFW Example: DollyZoom failed - offsetEnd must be greater than 0!")
        return
    endif

    ; Compute the invariant 'width' from the initial conditions.
    ; Relation: offset = width / (2 * tan(0.5 * FOV))
    ; => width = 2 * offsetStart * tan(0.5 * FOVStart)
    ; Note: Papyrus Math.Tan() takes degrees, Math.Atan() returns degrees.
    float width = 2.0 * offsetStart * Math.Tan(FOVStart * 0.5)

    FCFW_SKSEFunctions.ClearTimeline(ModName, dollyZoomTimelineID)

    FCFW_SKSEFunctions.SetFollowGround(ModName, dollyZoomTimelineID, follow = true)

    ObjectReference player = Game.GetPlayer()

    if FCFW_SKSEFunctions.AddTranslationPointAtRef(ModName, dollyZoomTimelineID, 0, \
            player, bodyPart = 0, offsetY = offsetStart, offsetz = 100.0, isOffsetRelative = true, \
            interpolationMode = 2) < 0
        Debug.Trace("FCFW_EXAMPLE: DollyZoom - failed to add translation point " + i)
    endif

    if FCFW_SKSEFunctions.AddFOVPoint(ModName, dollyZoomTimelineID, 0, FOVStart, \
            interpolationMode = 2) < 0
        Debug.Trace("FCFW_EXAMPLE: DollyZoom - failed to add FOV point " + i)
    endif

    ; Sample the dolly zoom curve at evenly spaced time steps.
    ; Linear interpolation between samples faithfully represents the
    ; non-linear FOV curve without cubic overshoot.
    int n = 20
    int i = 0
    while i <= n
        float alpha = i as float / n as float
        float t = alpha * zoomTime + 2.0
        float offset = offsetStart + (offsetEnd - offsetStart) * alpha

        ; FOV at this offset: FOV = 2 * atan(width / (2 * offset))
        float fov = 2.0 * Math.Atan(width / (2.0 * offset))

        ; Camera placed 'offset' units ahead of the player (in player's facing direction)
        if FCFW_SKSEFunctions.AddTranslationPointAtRef(ModName, dollyZoomTimelineID, t, \
                player, bodyPart = 0, offsetY = offset, offsetz = 100.0, isOffsetRelative = true, \
                interpolationMode = 2) < 0
            Debug.Trace("FCFW_EXAMPLE: DollyZoom - failed to add translation point " + i)
        endif

        if FCFW_SKSEFunctions.AddFOVPoint(ModName, dollyZoomTimelineID, t, fov, \
                interpolationMode = 2) < 0
            Debug.Trace("FCFW_EXAMPLE: DollyZoom - failed to add FOV point " + i)
        endif

        i += 1
    endWhile


    if FCFW_SKSEFunctions.AddTranslationPointAtRef(ModName, dollyZoomTimelineID, zoomTime + 4.0, \
            player, bodyPart = 0, offsetY = offsetEnd, offsetz = 100.0, isOffsetRelative = true, \
            interpolationMode = 2) < 0
        Debug.Trace("FCFW_EXAMPLE: DollyZoom - failed to add translation point " + i)
    endif

    float FOVEnd = 2.0 * Math.Atan(width / (2.0 * offsetEnd))
    if FCFW_SKSEFunctions.AddFOVPoint(ModName, dollyZoomTimelineID, zoomTime + 4.0, FOVEnd, \
            interpolationMode = 2) < 0
        Debug.Trace("FCFW_EXAMPLE: DollyZoom - failed to add FOV point " + i)
    endif


    ; Single rotation point: look directly at the player throughout the zoom.
    ; isOffsetRelative = false means "look at the reference" (offsetYaw = 0 = look straight at player).
    if FCFW_SKSEFunctions.AddRotationPointAtRef(ModName, dollyZoomTimelineID, 0.0, \
            player, bodyPart = 1, offsetPitch = 0.0, offsetYaw = 0.0, \
            isOffsetRelative = false, interpolationMode = 1) < 0
        Debug.Trace("FCFW_EXAMPLE: DollyZoom - failed to add rotation point")
    endif

    if !FCFW_SKSEFunctions.StartPlayback(ModName, dollyZoomTimelineID)
        Debug.Trace("FCFW_EXAMPLE: DollyZoom - failed to start playback")
        Debug.Notification("FCFW Example: DollyZoom failed to start!")
    endif
EndFunction

Function RestorePlaybackState()
    ; Restore playback if a timeline was active when the save was made
    ; Called from OnPlayerLoadGame()
    
    if savedActiveTimeline <= 0 || savedPlaybackTime < 0.0
        ; No active timeline when saved, nothing to restore
        Debug.Trace("FCFW_EXAMPLE: No playback state to restore")
        return
    endif
    
    Debug.Trace("FCFW_EXAMPLE: Restoring playback - Timeline " + savedActiveTimeline + " at time " + savedPlaybackTime)
    
    ; Determine which timeline to restore (map old ID to new ID after re-registration)
    int timelineToRestore = -1

    ; NOTE: Timeline IDs may change after re-registration on load!
    ; Assume that timelines have been re-registered via in IntializeTimelines() already.
    if savedActiveTimeline == 1
        timelineToRestore = timeline1ID
        ; Rebuild timelines 1-3 since they're dynamically generated
        BuildTimeline1()
        BuildTimeline2() 
        BuildReturnTimeline(timeline3ID)
    elseif savedActiveTimeline == 2
        timelineToRestore = timeline2ID
        ; Rebuild timelines 1-3 since they're dynamically generated
        BuildTimeline1()
        BuildTimeline2() 
        BuildReturnTimeline(timeline3ID)
    elseif savedActiveTimeline == 3
        timelineToRestore = timeline3ID
        ; Rebuild timelines 1-3 since they're dynamically generated
        BuildTimeline1()
        BuildTimeline2() 
        BuildReturnTimeline(timeline3ID)
    elseif savedActiveTimeline == 4 || savedActiveTimeline == 5
        ; Timeline 4 (recorded/imported) and Timeline 5 (return path after Timeline 4)
        ; are NOT supported for save/load restoration in this example.
        ; 
        ; Reason: Timeline 4's content exists only in memory (recorded or imported during session).
        ; After game load, Timeline 4 is empty and cannot be restored without additional persistence.
        ;
        ; A possible way to add support:
        ; 1. Auto-export Timeline 4 when recording stops or import completes:
        ;    FCFW_SKSEFunctions.ExportTimeline(ModName, timeline4ID, "Timeline4_AutoSave.yaml")
        ; 2. Re-import on load before restoring playback:
        ;    FCFW_SKSEFunctions.AddTimelineFromFile(ModName, timeline4ID, "Timeline4_AutoSave.yaml")
        ; 3. Then proceed with StartPlayback(startTime = savedPlaybackTime)
        
        Debug.MessageBox("FCFW Example: Timeline 4/5 playback cannot be restored from save.\n\n" + \
                         "Timeline 4 content (recorded/imported) is not persisted across saves.")
        Debug.Trace("FCFW_EXAMPLE: Timeline 4/5 restoration not supported - timeline content not persisted")
        
        ; Clear saved state and return without starting playback
        savedActiveTimeline = -1
        savedPlaybackTime = -1.0
        return
    elseif savedActiveTimeline == 6
        ; Dolly zoom timeline is NOT supported for save/load restoration.
        ; Reason: The zoom parameters (offsetStart, offsetEnd, FOVStart, zoomTime) are
        ; not persisted, so the timeline content cannot be rebuilt after loading.
        Debug.Trace("FCFW_EXAMPLE: Dolly zoom restoration not supported - zoom parameters not persisted")
        
        ; Clear saved state and return without starting playback
        savedActiveTimeline = -1
        savedPlaybackTime = -1.0
        return
    endif
    
    if timelineToRestore <= 0
        Debug.Trace("FCFW_EXAMPLE: ERROR - Could not map saved timeline ID to current timeline ID!")
        return
    endif
           
    ; Restore ground following settings
    FCFW_SKSEFunctions.SetFollowGround(ModName, timelineToRestore, follow = true, minHeight = savedMinHeightAboveGround)

    ; Resume playback at the saved time
    bool success = FCFW_SKSEFunctions.StartPlayback(modName = ModName, timelineID = timelineToRestore, \
        startTime = savedPlaybackTime)  ; <-- Resume at saved time!
    
    if success
        currentTimelineID = timelineToRestore
    else
        Debug.Trace("FCFW_EXAMPLE: ERROR - Failed to restore playback")
    endif
    
    ; Clear saved state after restoration
    savedActiveTimeline = -1
    savedPlaybackTime = -1.0
EndFunction
