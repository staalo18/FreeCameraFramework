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
; For a detailed reference on FreeCameraFramework and its usage, see Documentation/FCFW_PAPYRUS_REFERENCE.md.
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
int importTimeline_Key = 6 ; "5"
bool keyRegistered = false 

; timeline configuration
float CircleRadius = 1000.0 ; radius of orbit around marker
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
int currentTimelineID = -1


Event OnInit()
    if(!keyRegistered)
        RegisterForKey(togglePlayback1_Key)
        RegisterForKey(togglePlayback2_Key)
        RegisterForKey(toggleRecording_Key)
        RegisterForKey(exportTimeline_Key)
        RegisterForKey(importTimeline_Key)
        keyRegistered = true
    endif

    IntializeTimelines()
EndEvent


Event OnPlayerLoadGame()
    if(!keyRegistered)
        RegisterForKey(togglePlayback1_Key)
        RegisterForKey(togglePlayback2_Key)
        RegisterForKey(toggleRecording_Key)
        RegisterForKey(exportTimeline_Key)
        RegisterForKey(importTimeline_Key)
        keyRegistered = true
    endif
    
    IntializeTimelines()
EndEvent


Function IntializeTimelines()
    ; Register timelines with FCFW
    if timeline1ID > 0
        FCFW_SKSEFunctions.ClearTimeline(ModName, timeline1ID)
    else
        timeline1ID = RegisterTimeline()
    endif

    if timeline2ID > 0
        FCFW_SKSEFunctions.ClearTimeline(ModName, timeline2ID)
    else
        timeline2ID = RegisterTimeline()
    endif

    if timeline3ID > 0
        FCFW_SKSEFunctions.ClearTimeline(ModName, timeline3ID)
    else
        timeline3ID = RegisterTimeline()
    endif

    if timeline4ID > 0
        FCFW_SKSEFunctions.ClearTimeline(ModName, timeline4ID)
    else
        timeline4ID = RegisterTimeline()
    endif

    if timeline5ID > 0
        FCFW_SKSEFunctions.ClearTimeline(ModName, timeline5ID)
    else
        timeline5ID = RegisterTimeline()
    endif

    FCFW_SKSEFunctions.RegisterForTimelineEvents(self as Form)

    BuildTimeline2() 
    ; The other timelines will be built dynamically on playback start, or will be recorded by the user

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
    if FCFW_SKSEFunctions.AddTranslationPointAtRef(ModName, timeline1ID, time = transitionTime, reference = marker, offsetX = CircleRadius, offsetY = 0.0, offsetZ = 0.0, isOffsetRelative = true, easeIn = true, easeOut = false) < 0
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
        if FCFW_SKSEFunctions.AddTranslationPointAtRef(ModName, timeline2ID, time, marker, offsetX, offsetY, offsetZ, isOffsetRelative = true) < 0
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

            ; build timelines 1 and 3 dynamically to capture camera position at start of playback
            BuildTimeline1()
            BuildReturnTimeline(timeline3ID)

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
    endif
EndEvent


Event OnPlaybackStart(int eventtimelineID)
    if eventtimelineID == timeline1ID || eventtimelineID == timeline2ID || \
       eventtimelineID == timeline3ID || eventtimelineID == timeline4ID || \
       eventtimelineID == timeline5ID
        
        currentTimelineID = eventtimelineID
    endif
EndEvent

Event OnPlaybackStop(int eventtimelineID)
    if eventtimelineID == timeline1ID || eventtimelineID == timeline2ID || \
       eventtimelineID == timeline3ID || eventtimelineID == timeline4ID || \
       eventtimelineID == timeline5ID
       
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
