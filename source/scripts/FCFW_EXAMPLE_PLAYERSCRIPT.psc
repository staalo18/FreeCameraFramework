Scriptname FCFW_EXAMPLE_PLAYERSCRIPT extends ReferenceAlias


Event OnPlayerLoadGame()
    (GetOwningQuest() as FCFW_EXAMPLE_QUESTSCRIPT).Initalize()
    
    ; Restore playback state if there was an active timeline when saved
    (GetOwningQuest() as FCFW_EXAMPLE_QUESTSCRIPT).RestorePlaybackState()

EndEvent
