Scriptname FCFW_EXAMPLE_PLAYERSCRIPT extends ReferenceAlias


Event OnPlayerLoadGame()
    (GetOwningQuest() as FCFW_EXAMPLE_QUESTSCRIPT).Initalize()
EndEvent
