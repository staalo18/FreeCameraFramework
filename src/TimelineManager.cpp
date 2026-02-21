#include "TimelineManager.h"
#include "FCFW_Utils.h"
#include <yaml-cpp/yaml.h>
#include "APIManager.h"
#include "Hooks.h"
namespace FCFW {

    void TimelineManager::ToggleBodyPartRotationMatrixDisplay(RE::Actor* a_actor, BodyPart a_bodyPart) {
        m_displayRotationMatrix = !m_displayRotationMatrix;
        m_rotationMatrixActor = a_actor;
        m_rotationMatrixBodyPart = a_bodyPart;
    }

    void TimelineManager::UpdateBodyPartRotationMatrixDisplay() {
        if (!m_displayRotationMatrix || !m_rotationMatrixActor) {
            return;
        }

        auto targetPoint = _ts_SKSEFunctions::GetTargetPoint(m_rotationMatrixActor, BodyPartToLimbEnum(m_rotationMatrixBodyPart));
        if (targetPoint) {
            // Extract rotation from body part node
            const RE::NiMatrix3& rotMatrix = targetPoint->world.rotate;


            auto rotX = rotMatrix.GetVectorX();
            auto rotY = rotMatrix.GetVectorY();
            auto rotZ = rotMatrix.GetVectorZ();

            if (APIs::TrueHUD) {
                APIs::TrueHUD->DrawLine(targetPoint->world.translate, targetPoint->world.translate + 20.f * rotX, 0.1f, 0xFF0000FF);
                APIs::TrueHUD->DrawLine(targetPoint->world.translate + 20.f * rotX, targetPoint->world.translate + 40.f * rotX, 0.1f, 0xFFFFFFFF);
                APIs::TrueHUD->DrawLine(targetPoint->world.translate + 40.f * rotX, targetPoint->world.translate + 60.f * rotX, 0.1f, 0xFF0000FF);
                APIs::TrueHUD->DrawLine(targetPoint->world.translate + 60.f * rotX, targetPoint->world.translate + 80.f * rotX, 0.1f, 0xFFFFFFFF);
                APIs::TrueHUD->DrawLine(targetPoint->world.translate + 80.f * rotX, targetPoint->world.translate + 100.f * rotX, 0.1f, 0xFF0000FF);

                APIs::TrueHUD->DrawLine(targetPoint->world.translate, targetPoint->world.translate + 20.f * rotY, 0.1f, 0x00FF00FF);
                APIs::TrueHUD->DrawLine(targetPoint->world.translate + 20.f * rotY, targetPoint->world.translate + 40.f * rotY, 0.1f, 0xFFFFFFFF);
                APIs::TrueHUD->DrawLine(targetPoint->world.translate + 40.f * rotY, targetPoint->world.translate + 60.f * rotY, 0.1f, 0x00FF00FF);
                APIs::TrueHUD->DrawLine(targetPoint->world.translate + 60.f * rotY, targetPoint->world.translate + 80.f * rotY, 0.1f, 0xFFFFFFFF);
                APIs::TrueHUD->DrawLine(targetPoint->world.translate + 80.f * rotY, targetPoint->world.translate + 100.f * rotY, 0.1f, 0x00FF00FF);

                APIs::TrueHUD->DrawLine(targetPoint->world.translate, targetPoint->world.translate + 20.f * rotZ, 0.1f, 0x0000FFFF);
                APIs::TrueHUD->DrawLine(targetPoint->world.translate + 20.f * rotZ, targetPoint->world.translate + 40.f * rotZ, 0.1f, 0xFFFFFFFF);
                APIs::TrueHUD->DrawLine(targetPoint->world.translate + 40.f * rotZ, targetPoint->world.translate + 60.f * rotZ, 0.1f, 0x0000FFFF);
                APIs::TrueHUD->DrawLine(targetPoint->world.translate + 60.f * rotZ, targetPoint->world.translate + 80.f * rotZ, 0.1f, 0xFFFFFFFF);
                APIs::TrueHUD->DrawLine(targetPoint->world.translate + 80.f * rotZ, targetPoint->world.translate + 100.f * rotZ, 0.1f, 0x0000FFFF);

            } else {
                log::info("{}: TrueHUD API not available for debug drawing", __FUNCTION__);
            }
        } else {
            log::info("{}: No target point found for body part {}", __FUNCTION__, static_cast<int>(BodyPart::kHead));
        }
    }

    void TimelineManager::DispatchTimelineEvent(uint32_t a_messageType, size_t a_timelineID) {
        auto* messaging = SKSE::GetMessagingInterface();
        if (messaging) {
            FCFW_API::FCFWTimelineEventData eventData{ a_timelineID };
            messaging->Dispatch(a_messageType, &eventData, sizeof(eventData), nullptr);
        }
    }

    void TimelineManager::DispatchTimelineEventPapyrus(const char* a_eventName, size_t a_timelineID) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        // Send event to all registered forms
        for (auto* receiver : m_eventReceivers) {
            if (!receiver) {
                continue;
            }
            
            // Queue a task to dispatch the event on the Papyrus thread
            auto* task = SKSE::GetTaskInterface();
            if (task) {
                task->AddTask([receiver, eventName = std::string(a_eventName), timelineID = a_timelineID]() {
                    auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
                    if (!vm) {
                        return;
                    }
                    
                    auto* policy = vm->GetObjectHandlePolicy();
                    if (!policy) {
                        return;
                    }
                    
                    auto handle = policy->GetHandleForObject(receiver->GetFormType(), receiver);
                    auto args = RE::MakeFunctionArguments(static_cast<std::int32_t>(timelineID));
                    
                    vm->SendEvent(handle, RE::BSFixedString(eventName), args);
                });
            }            
        }
    }

    void TimelineManager::RegisterForTimelineEvents(RE::TESForm* a_form) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        if (!a_form) {
            return;
        }
        
        // Check if already registered
        auto it = std::find(m_eventReceivers.begin(), m_eventReceivers.end(), a_form);
        if (it == m_eventReceivers.end()) {
            m_eventReceivers.push_back(a_form);
            log::info("{}: Form 0x{:X} registered for timeline events", __FUNCTION__, a_form->GetFormID());
        }
    }

    void TimelineManager::UnregisterForTimelineEvents(RE::TESForm* a_form) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        if (!a_form) {
            return;
        }
        
        auto it = std::find(m_eventReceivers.begin(), m_eventReceivers.end(), a_form);
        if (it != m_eventReceivers.end()) {
            m_eventReceivers.erase(it);
            log::info("{}: Form 0x{:X} unregistered from timeline events", __FUNCTION__, a_form->GetFormID());
        }
    }

    void TimelineManager::Update() {
        // Hold lock for entire Update() to prevent race conditions
        // This ensures the timeline cannot be deleted/modified while we're using it
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        // for debug/testing: update body part rotation matrix display
        UpdateBodyPartRotationMatrixDisplay(); 
        
        auto* ui = RE::UI::GetSingleton();  
        if (!ui) {
            log::error("{}: UI singleton not available", __FUNCTION__);
            return;
        }

        if (m_isSaveInProgress) {
            // Indirectly check for ongoing Save (did not find event or hook that triggers at completion of save)
            if (ui->GameIsPaused()) {
                return;
            } else { // save completed
                OnPostSaveGame();
            }         
        }

        // Check for active timeline
        if (m_activeTimelineID == 0) {
            return;
        }
        
        auto it = m_timelines.find(m_activeTimelineID);
        if (it == m_timelines.end()) {
            return;
        }
        TimelineState* activeState = &it->second;
 
        // Execute timeline operations under lock protection
        PlayTimeline(activeState);
        RecordTimeline(activeState);
    }

    bool TimelineManager::StartRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_recordingInterval, bool a_append, float a_timeOffset) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        // Check if any timeline is already active
        if (m_activeTimelineID != 0) {
            log::error("{}: Timeline {} is already active", __FUNCTION__, m_activeTimelineID);
            return false;
        }
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (!playerCamera) {
            log::error("{}: PlayerCamera not available", __FUNCTION__);
            return false;
        }
        
        if (playerCamera->currentState && (playerCamera->currentState->id == RE::CameraState::kFree)) {
            log::warn("{}: Already in free camera mode", __FUNCTION__);
            return false;
        }

        // Validate and set recording interval
        if (a_recordingInterval < 0.0f) {
            log::warn("{}: Negative recording interval ({}) provided, treating as 0.0 (every frame)", __FUNCTION__, a_recordingInterval);
            state->m_recordingInterval = 0.0f;
        } else if (a_recordingInterval == 0.0f) {
            state->m_recordingInterval = 0.0f;
        } else {
            state->m_recordingInterval = a_recordingInterval;
        }

        // Capture camera rotation before entering free camera mode
        RE::NiPoint3 preSwitchRotation = _ts_SKSEFunctions::GetCameraRotation();

        ToggleFreeCameraNotHooked();

        // Ensure free camera rotation is initialized correctly
        if (playerCamera->currentState && playerCamera->currentState->id == RE::CameraState::kFree) {
            RE::FreeCameraState* freeCamState = static_cast<RE::FreeCameraState*>(playerCamera->currentState.get());
            if (freeCamState) {
                // Set free camera rotation to match pre-switch camera state
                // FreeCameraState::rotation is NiPoint2 where .x=pitch, .y=yaw
                freeCamState->rotation.x = preSwitchRotation.x;  // pitch
                freeCamState->rotation.y = preSwitchRotation.z;  // yaw (from NiPoint3.z)
                log::info("{}: Initialized free camera rotation for recording to pitch={}, yaw={}", 
                         __FUNCTION__, preSwitchRotation.x, preSwitchRotation.z);
            }
        }

        // Calculate start time and easing based on append mode
        float startTime = 0.0f;
        bool useEaseIn = true;
        
        if (a_append) {
            // Get the latest point time from either track
            float timelineDuration = state->m_timeline.GetDuration();
            
            if (timelineDuration > 0.0f) {
                // Timeline has existing points - append after them
                startTime = timelineDuration + a_timeOffset;
                useEaseIn = false;  // Not the first point
            } else {
                // Timeline is empty - use timeOffset as absolute start time
                startTime = a_timeOffset;
                useEaseIn = false;  // No easing when starting at non-zero time
            }
        } else {
            state->m_timeline.ClearPoints();
        }

        // Set as active timeline
        m_activeTimelineID = a_timelineID;
        state->m_isRecording = true;
        state->m_currentRecordingTime = startTime;
        state->m_lastRecordedPointTime = startTime - state->m_recordingInterval;  // Ensure first point is captured immediately

        // Add initial point
        RE::NiPoint3 cameraPos = _ts_SKSEFunctions::GetCameraPos();
        RE::NiPoint3 cameraRot = _ts_SKSEFunctions::GetCameraRotation();
        float fov = playerCamera ? playerCamera->worldFOV : 80.0f;
        
        Transition transTranslation(startTime, InterpolationMode::kCubicHermite, useEaseIn, false);
        TranslationPoint translationPoint(transTranslation, PointType::kWorld, cameraPos);
        state->m_timeline.AddTranslationPoint(translationPoint);
        
        Transition transRotation(startTime, InterpolationMode::kCubicHermite, useEaseIn, false);
        float cameraRoll = Hooks::FreeCameraRollHook::GetFreeCameraRoll();
        RotationPoint rotationPoint(transRotation, PointType::kWorld, RE::NiPoint3{cameraRot.x, cameraRoll, cameraRot.z});
        state->m_timeline.AddRotationPoint(rotationPoint);
        
        Transition transFOV(startTime, InterpolationMode::kCubicHermite, useEaseIn, false);
        FOVPoint fovPoint(transFOV, fov);
        state->m_timeline.AddFOVPoint(fovPoint);
        
        return true;
    }

    bool TimelineManager::StopRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        if (!state->m_isRecording) {
            log::warn("{}: Timeline {} is not recording", __FUNCTION__, a_timelineID);
            return false;
        }
        
        if (m_activeTimelineID != a_timelineID) {
            log::error("{}: Timeline {} is not the active timeline", __FUNCTION__, a_timelineID);
            return false;
        }
        
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (!playerCamera) {
            return false;
        }
        
        if (!(playerCamera->currentState && (playerCamera->currentState->id == RE::CameraState::kFree))) {
            log::warn("{}: Not in free camera mode", __FUNCTION__);
        }

        RE::NiPoint3 cameraPos = _ts_SKSEFunctions::GetCameraPos();
        RE::NiPoint3 cameraRot = _ts_SKSEFunctions::GetCameraRotation();
        float fov = playerCamera ? playerCamera->worldFOV : 80.0f;
        
        Transition transTranslation(state->m_currentRecordingTime, InterpolationMode::kCubicHermite, false, true);
        TranslationPoint translationPoint(transTranslation, PointType::kWorld, cameraPos);
        state->m_timeline.AddTranslationPoint(translationPoint);
        
        Transition transRotation(state->m_currentRecordingTime, InterpolationMode::kCubicHermite, false, true);
        float cameraRoll = Hooks::FreeCameraRollHook::GetFreeCameraRoll();
        RotationPoint rotationPoint(transRotation, PointType::kWorld, RE::NiPoint3{cameraRot.x, cameraRoll, cameraRot.z});
        state->m_timeline.AddRotationPoint(rotationPoint);
        
        Transition transFOV(state->m_currentRecordingTime, InterpolationMode::kCubicHermite, false, true);
        FOVPoint fovPoint(transFOV, fov);
        state->m_timeline.AddFOVPoint(fovPoint);
        
        ToggleFreeCameraNotHooked();
        
        // Clear recording state
        m_activeTimelineID = 0;
        state->m_isRecording = false;
        
        log::info("{}: Stopped recording on timeline {}", __FUNCTION__, a_timelineID);
        return true;
    }

    int TimelineManager::AddTranslationPointAtCamera(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return -1;
        }
        
        if (state->m_isPlaybackRunning) {
            log::info("{}: Timeline modified during playback, stopping playback", __FUNCTION__);
            StopPlayback(a_pluginHandle, a_timelineID);
        }
        
        Transition transition(a_time, a_interpolationMode, a_easeIn, a_easeOut);
        TranslationPoint point = state->m_timeline.GetTranslationPointAtCamera(a_time, a_easeIn, a_easeOut);
        point.m_transition = transition;
        
        return static_cast<int>(state->m_timeline.AddTranslationPoint(point));
    }

    int TimelineManager::AddTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, const RE::NiPoint3& a_position, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return -1;
        }
        
        if (state->m_isPlaybackRunning) {
            log::info("{}: Timeline modified during playback, stopping playback", __FUNCTION__);
            StopPlayback(a_pluginHandle, a_timelineID);
        }
        
        Transition transition(a_time, a_interpolationMode, a_easeIn, a_easeOut);
        TranslationPoint point(transition, PointType::kWorld, a_position);
        
        return static_cast<int>(state->m_timeline.AddTranslationPoint(point));
    }

    int TimelineManager::AddTranslationPointAtRef(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, RE::TESObjectREFR* a_reference, BodyPart a_bodyPart, const RE::NiPoint3& a_offset, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return -1;
        }
        
        if (!a_reference) {
            log::error("{}: Null reference provided", __FUNCTION__);
            return -1;
        }
        
        if (state->m_isPlaybackRunning) {
            log::info("{}: Timeline modified during playback, stopping playback", __FUNCTION__);
            StopPlayback(a_pluginHandle, a_timelineID);
        }
        
        Transition transition(a_time, a_interpolationMode, a_easeIn, a_easeOut);
        TranslationPoint point(transition, PointType::kReference, RE::NiPoint3{}, a_offset, a_reference, a_isOffsetRelative, a_bodyPart);
        
        return static_cast<int>(state->m_timeline.AddTranslationPoint(point));
    }

    int TimelineManager::AddRotationPointAtCamera(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return -1;
        }
        
        if (state->m_isPlaybackRunning) {
            log::info("{}: Timeline modified during playback, stopping playback", __FUNCTION__);
            StopPlayback(a_pluginHandle, a_timelineID);
        }
        
        Transition transition(a_time, a_interpolationMode, a_easeIn, a_easeOut);
        RotationPoint point = state->m_timeline.GetRotationPointAtCamera(a_time, a_easeIn, a_easeOut);
        point.m_transition = transition;
        
        return static_cast<int>(state->m_timeline.AddRotationPoint(point));
    }

    int TimelineManager::AddRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, const RE::NiPoint3& a_rotation, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return -1;
        }
        
        if (state->m_isPlaybackRunning) {
            log::info("{}: Timeline modified during playback, stopping playback", __FUNCTION__);
            StopPlayback(a_pluginHandle, a_timelineID);
        }
        
        Transition transition(a_time, a_interpolationMode, a_easeIn, a_easeOut);
        RotationPoint point(transition, PointType::kWorld, a_rotation);
        
        return static_cast<int>(state->m_timeline.AddRotationPoint(point));
    }

    int TimelineManager::AddRotationPointAtRef(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, RE::TESObjectREFR* a_reference, BodyPart a_bodyPart, const RE::NiPoint3& a_offset, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return -1;
        }
        
        if (!a_reference) {
            log::error("{}: Null reference provided", __FUNCTION__);
            return -1;
        }
        
        if (state->m_isPlaybackRunning) {
            log::info("{}: Timeline modified during playback, stopping playback", __FUNCTION__);
            StopPlayback(a_pluginHandle, a_timelineID);
        }
        
        Transition transition(a_time, a_interpolationMode, a_easeIn, a_easeOut);
        RotationPoint point(transition, PointType::kReference, RE::NiPoint3{}, a_offset, a_reference, a_isOffsetRelative, a_bodyPart);
        
        return static_cast<int>(state->m_timeline.AddRotationPoint(point));
    }

    int TimelineManager::AddFOVPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, float a_fov, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return -1;
        }
        
        if (state->m_isPlaybackRunning) {
            log::info("{}: Timeline modified during playback, stopping playback", __FUNCTION__);
            StopPlayback(a_pluginHandle, a_timelineID);
        }
        
        Transition transition(a_time, a_interpolationMode, a_easeIn, a_easeOut);
        FOVPoint point(transition, a_fov);
        
        return static_cast<int>(state->m_timeline.AddFOVPoint(point));
    }

    bool TimelineManager::RemoveTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        if (state->m_isPlaybackRunning) {
            log::info("{}: Timeline modified during playback, stopping playback", __FUNCTION__);
            StopPlayback(a_pluginHandle, a_timelineID);
        }
        
        state->m_timeline.RemoveTranslationPoint(a_index);
        return true;
    }

    bool TimelineManager::RemoveRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        if (state->m_isPlaybackRunning) {
            log::info("{}: Timeline modified during playback, stopping playback", __FUNCTION__);
            StopPlayback(a_pluginHandle, a_timelineID);
        }
        
        state->m_timeline.RemoveRotationPoint(a_index);
        return true;
    }

    bool TimelineManager::RemoveFOVPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        if (state->m_isPlaybackRunning) {
            log::info("{}: Timeline modified during playback, stopping playback", __FUNCTION__);
            StopPlayback(a_pluginHandle, a_timelineID);
        }
        
        state->m_timeline.RemoveFOVPoint(a_index);
        return true;
    }
/* Experimentation for handling camera movements across multiple cells

    std::int32_t m_currentCellX = 0;
    std::int32_t m_currentCellY = 0;
    std::int32_t m_initialCellX = 0;
    std::int32_t m_initialCellY = 0;
    RE::NiPoint3 m_initialPlayerPosition;
    bool m_isPlayerAtInitialPosition = true;

    void TimelineManager::RecenterGridAroundCameraIfNeeded()
    {
        auto tes = RE::TES::GetSingleton();
        if (!tes || !tes->gridCells) {
            log::error("{}: TES or gridCells not available", __FUNCTION__);
            return;
        }

        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            log::error("{}: PlayerCharacter not available", __FUNCTION__);
            return;
        }

        auto cameraPos = _ts_SKSEFunctions::GetCameraPos();
        // Calculate cell coordinates manually (Skyrim cells are 4096 units)
        constexpr float CELL_SIZE = 4096.0f;
        std::int32_t cameraCellX = static_cast<std::int32_t>(std::floor(cameraPos.x / CELL_SIZE));
        std::int32_t cameraCellY = static_cast<std::int32_t>(std::floor(cameraPos.y / CELL_SIZE));

        float distanceToInitial = cameraPos.GetDistance(m_initialPlayerPosition);
        float distanceToPlayer = cameraPos.GetDistance(player->GetPosition());

        if ((cameraCellX  == m_initialCellX && cameraCellY == m_initialCellY) || distanceToInitial < CELL_SIZE) {
            if (!m_isPlayerAtInitialPosition) {
log::info("{}: Resetting player position to initial position", __FUNCTION__);
                player->SetPosition(m_initialPlayerPosition, true);
                
//                player->Enable(true);
                m_isPlayerAtInitialPosition = true;
            }
            m_currentCellX = cameraCellX;
            m_currentCellY = cameraCellY;
        } else if ((cameraCellX != m_currentCellX || cameraCellY != m_currentCellY) && distanceToPlayer > CELL_SIZE) {
log::info("{}: Recentering grid to cell ({}, {})", __FUNCTION__, cameraCellX, cameraCellY);
//            float landHeight = _ts_SKSEFunctions::GetLandHeightWithWater(cameraPos);
//            cameraPos.z = landHeight + 10.0f;
            
            player->SetPosition(cameraPos, true);
                        
//            player->Disable();    
            m_isPlayerAtInitialPosition = false;
            m_currentCellX = cameraCellX;
            m_currentCellY = cameraCellY;
        }
    }
*/
    void TimelineManager::PlayTimeline(TimelineState* a_state) {
        if (!a_state || !a_state->m_isPlaybackRunning) {
            return;
        }
        
        if (a_state->m_timeline.GetTranslationPointCount() == 0 && a_state->m_timeline.GetRotationPointCount() == 0) {
            m_activeTimelineID = 0;
            a_state->m_isPlaybackRunning = false;
            return;
        }
        
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (!playerCamera) {
            log::error("{}: PlayerCamera not found during playback", __FUNCTION__);
            m_activeTimelineID = 0;
            a_state->m_isPlaybackRunning = false;
            return;
        }
        
        if (!playerCamera->IsInFreeCameraMode()) {
            m_activeTimelineID = 0;
            a_state->m_isPlaybackRunning = false;
            return;
        }
        
        RE::FreeCameraState* cameraState = nullptr;
        if (playerCamera->currentState && (playerCamera->currentState->id == RE::CameraState::kFree)) {
            cameraState = static_cast<RE::FreeCameraState*>(playerCamera->currentState.get());
        }
        
        if (!cameraState) {
            log::error("{}: FreeCameraState not found during playback", __FUNCTION__);
            m_activeTimelineID = 0;
            a_state->m_isPlaybackRunning = false;
            return;
        }

        // Update UI visibility
        auto* ui = RE::UI::GetSingleton();        
        if (ui && ui->GameIsPaused()) {
            ui->ShowMenus(m_isShowingMenus);
            return;
        }        
        ui->ShowMenus(a_state->m_showMenusDuringPlayback);

        float deltaTime = _ts_SKSEFunctions::GetRealTimeDeltaTime() * a_state->m_playbackSpeed;
        a_state->m_timeline.UpdatePlayback(deltaTime);

//        RecenterGridAroundCameraIfNeeded();
        // re-center audio to current camera position
        CorrectAudioListener();

        // Apply global easing
        float sampleTime = a_state->m_timeline.GetPlaybackTime();
        if (a_state->m_globalEaseIn || a_state->m_globalEaseOut) {
            float timelineDuration = a_state->m_timeline.GetDuration();
            
            if (timelineDuration > 0.0f) {
                float linearProgress = std::clamp(sampleTime / timelineDuration, 0.0f, 1.0f);
                float easedProgress = _ts_SKSEFunctions::ApplyEasing(linearProgress, a_state->m_globalEaseIn, a_state->m_globalEaseOut);
                sampleTime = easedProgress * timelineDuration;
            }
        }
        
        // Get interpolated points
        RE::NiPoint3 cameraPos = a_state->m_timeline.GetTranslation(sampleTime);
        
        // Apply FOV if timeline has FOV points
        if (a_state->m_timeline.GetFOVPointCount() > 0) {
            playerCamera->worldFOV = a_state->m_timeline.GetFOV(sampleTime);
        }

        // Apply ground-following if enabled
        if (a_state->m_followGround) {
            float landHeight = _ts_SKSEFunctions::GetLandHeightWithWater(cameraPos);
            float cameraHeight = cameraPos.z - landHeight;
            if (cameraHeight < a_state->m_minHeightAboveGround) {
                cameraPos.z = landHeight + a_state->m_minHeightAboveGround;
            }
        }
        
        cameraState->translation = cameraPos;
        
        RE::NiPoint3 rotation = a_state->m_timeline.GetRotation(sampleTime);
        
        // Handle user rotation
        if (m_userTurning && a_state->m_allowUserRotation) {
            a_state->m_rotationOffset.x = _ts_SKSEFunctions::NormalRelativeAngle(cameraState->rotation.x - rotation.x);
            a_state->m_rotationOffset.z = _ts_SKSEFunctions::NormalRelativeAngle(cameraState->rotation.y - rotation.z);
            m_userTurning = false;
        } else {
            cameraState->rotation.x = _ts_SKSEFunctions::NormalRelativeAngle(rotation.x + a_state->m_rotationOffset.x);
            cameraState->rotation.y = _ts_SKSEFunctions::NormalRelativeAngle(rotation.z + a_state->m_rotationOffset.z);
        }

        // Inject roll from timeline via hook
        float roll = rotation.y;
        Hooks::FreeCameraRollHook::SetFreeCameraRoll(roll);
        
        if (a_state->m_timeline.GetPlaybackMode() == PlaybackMode::kWait) {
            float playbackTime = a_state->m_timeline.GetPlaybackTime();
            float timelineDuration = a_state->m_timeline.GetDuration();
            if ((playbackTime >= timelineDuration) && !a_state->m_isCompletedAndWaiting) {
                DispatchTimelineEvent(static_cast<uint32_t>(FCFW_API::FCFWMessage::kPlaybackWait), a_state->m_id);
                DispatchTimelineEventPapyrus("OnPlaybackWait", a_state->m_id);
                a_state->m_isCompletedAndWaiting = true;
            }
            // Keep playback running - user must manually call StopPlayback
        } else if (!a_state->m_timeline.IsPlaying()) {
            size_t timelineID = a_state->m_id;
            SKSE::PluginHandle ownerHandle = a_state->m_ownerHandle;
            StopPlayback(ownerHandle, timelineID);
        }
    }

    bool TimelineManager::ClearTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        if (state->m_isRecording) {
            return false;
        }
        
        if (state->m_isPlaybackRunning) {
            log::info("{}: Timeline modified during playback, stopping playback", __FUNCTION__);
            StopPlayback(a_pluginHandle, a_timelineID);
        }
        
        state->Reset();
        
        return true;
    }

    bool TimelineManager::StartPlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_speed, bool a_globalEaseIn, bool a_globalEaseOut, bool a_useDuration, float a_duration, float a_startTime) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        // Check if any timeline is already active
        if (m_activeTimelineID != 0) {
            log::error("{}: Timeline {} is already active", __FUNCTION__, m_activeTimelineID);
            return false;
        }
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        // Validation checks
        if (state->m_timeline.GetTranslationPointCount() == 0 && state->m_timeline.GetRotationPointCount() == 0) {
            log::error("{}: Timeline {} has no points", __FUNCTION__, a_timelineID);
            return false;
        }
        
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (!playerCamera) {
            log::error("{}: PlayerCamera not available", __FUNCTION__);
            return false;
        }
        
        if (playerCamera->IsInFreeCameraMode()) {
            log::error("{}: Already in free camera mode", __FUNCTION__);
            return false;
        }
        
        float timelineDuration = state->m_timeline.GetDuration();
        if (timelineDuration < 0.0f && !a_useDuration) {
            log::error("{}: Timeline duration is negative", __FUNCTION__);
            return false;
        }
        
        // Calculate playback speed
        if (a_useDuration) {
            if (a_duration < 0.0f) {
                log::warn("{}: Invalid duration {}, defaulting to timeline duration", __FUNCTION__, a_duration);
                state->m_playbackDuration = timelineDuration;
                state->m_playbackSpeed = 1.0f;
            } else {
                state->m_playbackDuration = a_duration;
                state->m_playbackSpeed = timelineDuration / state->m_playbackDuration;
            }
        } else {
            if (a_speed <= 0.0f) {
                log::warn("{}: Invalid speed {}, defaulting to 1.0", __FUNCTION__, a_speed);
                state->m_playbackDuration = timelineDuration;
                state->m_playbackSpeed = 1.0f;
            } else {
                state->m_playbackDuration = timelineDuration / a_speed;
                state->m_playbackSpeed = a_speed;
            }
        }
        
        if (state->m_playbackDuration < 0.0f) {
            log::error("{}: Playback duration is negative", __FUNCTION__);
            return false;
        }
        
        // Set playback parameters
        state->m_globalEaseIn = a_globalEaseIn;
        state->m_globalEaseOut = a_globalEaseOut;
        
        // Set as active timeline
        m_activeTimelineID = a_timelineID;
        state->m_isPlaybackRunning = true;
        state->m_rotationOffset = RE::NiPoint3{ 0.0f, 0.0f, 0.0f };  // Reset per-timeline rotation offset
        state->m_isCompletedAndWaiting = false;   // Reset completion event flag for kWait mode
        
        // Save pre-playback state
        if (playerCamera->currentState && 
            (playerCamera->currentState->id == RE::CameraState::kThirdPerson ||
             playerCamera->currentState->id == RE::CameraState::kMount ||
             playerCamera->currentState->id == RE::CameraState::kDragon)) {
            
            RE::ThirdPersonState* cameraState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
            if (cameraState) {
                m_lastFreeRotation = cameraState->freeRotation;
            }
        }
        
        // Save current FOV to restore after playback
        state->m_savedFOV = playerCamera->worldFOV;
        
        // Initialize timeline playback
        state->m_timeline.ResetPlayback();
        state->m_timeline.StartPlayback();
        
        // Set start time if specified (for save/load resume)
        if (a_startTime > 0.0f) {
            float clampedTime = std::clamp(a_startTime, 0.0f, state->m_timeline.GetDuration());
            if (clampedTime != a_startTime) {
                log::warn("{}: Start time {} exceeds timeline duration {}, clamping to {}",
                          __FUNCTION__, a_startTime, state->m_timeline.GetDuration(), clampedTime);
            }
            state->m_timeline.SetPlaybackTime(clampedTime);
        }
        
        // Handle UI visibility
        auto* ui = RE::UI::GetSingleton();
        if (ui) {
            m_isShowingMenus = ui->IsShowingMenus();
            ui->ShowMenus(state->m_showMenusDuringPlayback);
        }
/* Experimentation for handling camera movements across multiple cells
        // store initial player position
        auto player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            log::error("{}: PlayerCharacter not available", __FUNCTION__);
            return false;
        }

        m_initialPlayerPosition = player->GetPosition();
        auto cameraPos = _ts_SKSEFunctions::GetCameraPos();
        // Calculate cell coordinates manually (Skyrim cells are 4096 units)
        constexpr float CELL_SIZE = 4096.0f;
        m_initialCellX = static_cast<std::int32_t>(std::floor(cameraPos.x / CELL_SIZE));
        m_initialCellY = static_cast<std::int32_t>(std::floor(cameraPos.y / CELL_SIZE));
        m_currentCellX = m_initialCellX;
        m_currentCellY = m_initialCellY;
*/        
        // Capture camera rotation before entering free camera mode
        // This ensures free camera inherits the correct rotation even if the game doesn't do it automatically
        RE::NiPoint3 preSwitchRotation = _ts_SKSEFunctions::GetCameraRotation();
        
        // Enter free camera mode
        ToggleFreeCameraNotHooked();
        
        // Ensure free camera rotation is initialized correctly
        if (playerCamera->currentState && playerCamera->currentState->id == RE::CameraState::kFree) {
            RE::FreeCameraState* freeCamState = static_cast<RE::FreeCameraState*>(playerCamera->currentState.get());
            if (freeCamState) {
                // Set free camera rotation to match pre-switch camera state
                // FreeCameraState::rotation is NiPoint2 where .x=pitch, .y=yaw
                freeCamState->rotation.x = preSwitchRotation.x;  // pitch
                freeCamState->rotation.y = preSwitchRotation.z;  // yaw (from NiPoint3.z)
                log::info("{}: Initialized free camera rotation to pitch={}, yaw={}", 
                         __FUNCTION__, preSwitchRotation.x, preSwitchRotation.z);
            }
        }
        
        log::info("{}: Started playback on timeline {}", __FUNCTION__, a_timelineID);
        
        // Dispatch playback started event
        DispatchTimelineEvent(static_cast<uint32_t>(FCFW_API::FCFWMessage::kPlaybackStart), a_timelineID);
        DispatchTimelineEventPapyrus("OnPlaybackStart", a_timelineID);
        
        return true;
    }

    int TimelineManager::GetTranslationPointCount(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return -1;
        }
        
        return static_cast<int>(state->m_timeline.GetTranslationPointCount());
    }

    int TimelineManager::GetRotationPointCount(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return -1;
        }
        
        return static_cast<int>(state->m_timeline.GetRotationPointCount());
    }

    int TimelineManager::GetFOVPointCount(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return -1;
        }
        
        return static_cast<int>(state->m_timeline.GetFOVPointCount());
    }

    

    bool TimelineManager::StopPlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        if (!state->m_isPlaybackRunning) {
            log::warn("{}: Timeline {} is not playing", __FUNCTION__, a_timelineID);
            return false;
        }
        
        if (m_activeTimelineID != a_timelineID) {
            log::error("{}: Timeline {} is not the active timeline", __FUNCTION__, a_timelineID);
            return false;
        }
        
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (playerCamera && playerCamera->IsInFreeCameraMode()) {
            ToggleFreeCameraNotHooked();
            
            auto* ui = RE::UI::GetSingleton();
            if (ui) {
                ui->ShowMenus(m_isShowingMenus);
            }
            
            if (playerCamera->currentState && 
                (playerCamera->currentState->id == RE::CameraState::kThirdPerson ||
                 playerCamera->currentState->id == RE::CameraState::kMount ||
                 playerCamera->currentState->id == RE::CameraState::kDragon)) {
                
                RE::ThirdPersonState* cameraState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
                if (cameraState) {
                    cameraState->freeRotation = m_lastFreeRotation;
                }
            }
            
            // Restore FOV to pre-playback value
            playerCamera->worldFOV = state->m_savedFOV;

            // Reset camera roll
            Hooks::FreeCameraRollHook::SetFreeCameraRoll(0.0f);
        }
        
        // Clear active state
        m_activeTimelineID = 0;
        state->m_isPlaybackRunning = false;
        
        log::info("{}: Stopped playback on timeline {}", __FUNCTION__, a_timelineID);
        
        // Dispatch playback stopped event
        DispatchTimelineEvent(static_cast<uint32_t>(FCFW_API::FCFWMessage::kPlaybackStop), a_timelineID);
        DispatchTimelineEventPapyrus("OnPlaybackStop", a_timelineID);
        
        return true;
    }

    bool TimelineManager::SwitchPlayback(SKSE::PluginHandle a_pluginHandle, size_t a_fromTimelineID, size_t a_toTimelineID) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        // Validate target timeline exists and is owned by caller
        TimelineState* toState = GetTimeline(a_toTimelineID, a_pluginHandle);
        if (!toState) {
            log::error("{}: Target timeline {} not found or not owned by plugin handle {}", __FUNCTION__, a_toTimelineID, a_pluginHandle);
            return false;
        }
        
        // Find source timeline to switch from
        TimelineState* fromState = nullptr;
        
        if (a_fromTimelineID == 0) {
            // Switch from any timeline owned by this plugin that is currently playing
            for (auto& [id, state] : m_timelines) {
                if (state.m_ownerHandle == a_pluginHandle && state.m_isPlaybackRunning && m_activeTimelineID == id) {
                    fromState = &state;
                    a_fromTimelineID = id;  // Store for logging
                    break;
                }
            }
            
            if (!fromState) {
                log::warn("{}: No active timeline found for plugin handle {}", __FUNCTION__, a_pluginHandle);
                return false;
            }
        } else {
            fromState = GetTimeline(a_fromTimelineID, a_pluginHandle);
            if (!fromState) {
                log::warn("{}: Source timeline {} not found or not owned by plugin handle {}", __FUNCTION__, a_fromTimelineID, a_pluginHandle);
                return false;
            }
            
            // Verify source timeline is actively playing
            if (!fromState->m_isPlaybackRunning || m_activeTimelineID != a_fromTimelineID) {
                log::warn("{}: Source timeline {} is not actively playing", __FUNCTION__, a_fromTimelineID);
                return false;
            }
        }
        
        // Validate target timeline has points
        if (toState->m_timeline.GetTranslationPointCount() == 0 && toState->m_timeline.GetRotationPointCount() == 0) {
            log::error("{}: Target timeline {} has no points", __FUNCTION__, a_toTimelineID);
            return false;
        }
        
        // Validate camera is in free camera mode
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (!playerCamera || !playerCamera->IsInFreeCameraMode()) {
            log::error("{}: Not in free camera mode", __FUNCTION__);
            return false;
        }
        
        log::info("{}: Switching playback from timeline {} to timeline {}", 
                  __FUNCTION__, a_fromTimelineID, a_toTimelineID);
        
        // Stop source timeline WITHOUT exiting free camera mode
        fromState->m_isPlaybackRunning = false;
        m_activeTimelineID = 0;  // Temporarily clear to allow new timeline activation
        
        // Dispatch stop event for source timeline
        DispatchTimelineEvent(static_cast<uint32_t>(FCFW_API::FCFWMessage::kPlaybackStop), a_fromTimelineID);
        DispatchTimelineEventPapyrus("OnPlaybackStop", a_fromTimelineID);
        
        // Initialize target timeline (StartPlayback will call UpdateCameraPoints internally)
        toState->m_timeline.ResetPlayback();
        toState->m_timeline.StartPlayback();
        
        // Copy all runtime playback state from source to target timeline
        CopyPlaybackState(fromState, toState);
        
        // Activate target timeline (camera stays in free mode)
        m_activeTimelineID = a_toTimelineID;
        toState->m_isPlaybackRunning = true;
        toState->m_isCompletedAndWaiting = false;
        
        // Dispatch start event for target timeline
        DispatchTimelineEvent(static_cast<uint32_t>(FCFW_API::FCFWMessage::kPlaybackStart), a_toTimelineID);
        DispatchTimelineEventPapyrus("OnPlaybackStart", a_toTimelineID);

        return true;
    }

    void TimelineManager::SetUserTurning(bool a_turning) {
        m_userTurning = a_turning;
    }

    void TimelineManager::CopyPlaybackState(TimelineState* a_fromState, TimelineState* a_toState) {        
        a_toState->m_playbackSpeed = a_fromState->m_playbackSpeed;
        a_toState->m_showMenusDuringPlayback = a_fromState->m_showMenusDuringPlayback;
        a_toState->m_globalEaseIn = a_fromState->m_globalEaseIn;
        a_toState->m_globalEaseOut = a_fromState->m_globalEaseOut;
        a_toState->m_followGround = a_fromState->m_followGround;
        a_toState->m_minHeightAboveGround = a_fromState->m_minHeightAboveGround;
        
        // Only preserve rotation offset if target timeline allows user rotation
        // If target doesn't allow user rotation, reset to zero so timeline plays its intended rotation
        if (a_toState->m_allowUserRotation) {
            a_toState->m_rotationOffset = a_fromState->m_rotationOffset;
        } else {
            a_toState->m_rotationOffset = RE::NiPoint3{ 0.0f, 0.0f, 0.0f };
        }
    }

    bool TimelineManager::PausePlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        if (!state->m_isPlaybackRunning) {
            return false;
        }
        
        state->m_timeline.PausePlayback();
        return true;
    }

    bool TimelineManager::ResumePlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        if (!state->m_isPlaybackRunning) {
            return false;
        }
        
        state->m_timeline.ResumePlayback();
        return true;
    }

    bool TimelineManager::IsPlaybackRunning(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        return state->m_isPlaybackRunning;
    }

    float TimelineManager::GetPlaybackTime(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        

        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return -1.0f;
        }
        
        return state->m_timeline.GetPlaybackTime();
    }

    bool TimelineManager::IsRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        return state->m_isRecording;
    }

    bool TimelineManager::IsPlaybackPaused(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        return state->m_timeline.IsPaused();
    }

    RE::NiPoint3 TimelineManager::GetTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            log::error("{}: Timeline {} not found or not owned by plugin handle {}", __FUNCTION__, a_timelineID, a_pluginHandle);
            return RE::NiPoint3(0.0f, 0.0f, 0.0f);
        }
        
        if (a_index >= state->m_timeline.GetTranslationPointCount()) {
            log::error("{}: Index {} out of range (timeline {} has {} translation points)", __FUNCTION__, a_index, a_timelineID, state->m_timeline.GetTranslationPointCount());
            return RE::NiPoint3(0.0f, 0.0f, 0.0f);
        }
        
        return state->m_timeline.GetTranslationPoint(a_index);
    }

    RE::NiPoint3 TimelineManager::GetRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            log::error("{}: Timeline {} not found or not owned by plugin handle {}", __FUNCTION__, a_timelineID, a_pluginHandle);
            return RE::NiPoint3{0.0f, 0.0f, 0.0f};
        }
        
        if (a_index >= state->m_timeline.GetRotationPointCount()) {
            log::error("{}: Index {} out of range (timeline {} has {} rotation points)", __FUNCTION__, a_index, a_timelineID, state->m_timeline.GetRotationPointCount());
            return RE::NiPoint3{0.0f, 0.0f, 0.0f};
        }
        
        return state->m_timeline.GetRotationPoint(a_index);
    }

    float TimelineManager::GetFOVPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            log::error("{}: Timeline {} not found or not owned by plugin handle {}", __FUNCTION__, a_timelineID, a_pluginHandle);
            return 80.0f;
        }
        
        if (a_index >= state->m_timeline.GetFOVPointCount()) {
            log::error("{}: Index {} out of range (timeline {} has {} FOV points)", __FUNCTION__, a_index, a_timelineID, state->m_timeline.GetFOVPointCount());
            return 80.0f;
        }
        
        return state->m_timeline.GetFOVPoint(a_index);
    }

    bool TimelineManager::AllowUserRotation(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, bool a_allow) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        state->m_allowUserRotation = a_allow;
        return true;
    }

    bool TimelineManager::IsUserRotationAllowed(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        return state->m_allowUserRotation;
    }

    bool TimelineManager::SetFollowGround(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, bool a_follow, float a_minHeight) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        state->m_followGround = a_follow;
        state->m_minHeightAboveGround = a_minHeight;
        return true;
    }

    bool TimelineManager::IsGroundFollowingEnabled(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        return state->m_followGround;
    }

    float TimelineManager::GetMinHeightAboveGround(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return -1.0f;
        }
        
        return state->m_minHeightAboveGround;
    }

    bool TimelineManager::SetMenuVisibility(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, bool a_show) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        state->m_showMenusDuringPlayback = a_show;
        
        // Apply immediately if playback is active
        if (state->m_isPlaybackRunning && m_activeTimelineID == a_timelineID) {
            auto* ui = RE::UI::GetSingleton();
            if (ui) {
                ui->ShowMenus(a_show);
            }
        }
        
        return true;
    }

    bool TimelineManager::AreMenusVisible(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        return state->m_showMenusDuringPlayback;
    }

    bool TimelineManager::SetPlaybackMode(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, PlaybackMode a_playbackMode, float a_loopTimeOffset) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        state->m_timeline.SetPlaybackMode(a_playbackMode);
        state->m_timeline.SetLoopTimeOffset(a_loopTimeOffset);
        
        return true;
    }

    bool TimelineManager::AddTimelineFromFile(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, const char* a_filePath, float a_timeOffset) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        if (state->m_isPlaybackRunning) {
            log::info("{}: Timeline modified during playback, stopping playback", __FUNCTION__);
            StopPlayback(a_pluginHandle, a_timelineID);
        }
        
        std::filesystem::path fullPath = std::filesystem::current_path() / "Data" / a_filePath;
        
        if (!std::filesystem::exists(fullPath)) {
            log::error("{}: File does not exist: {}", __FUNCTION__, fullPath.string());
            return false;
        }
        
        YAML::Node root = YAML::LoadFile(fullPath.string());
        
        if (root["playbackMode"]) {
            std::string modeStr = root["playbackMode"].as<std::string>();
            PlaybackMode mode = StringToPlaybackMode(modeStr);
            state->m_timeline.SetPlaybackMode(mode);
        }
        
        if (root["loopTimeOffset"]) {
            float offset = root["loopTimeOffset"].as<float>();
            state->m_timeline.SetLoopTimeOffset(offset);
        }
        
        if (root["globalEaseIn"]) {
            bool easeIn = root["globalEaseIn"].as<bool>();
            state->m_globalEaseIn = easeIn;
        }
        
        if (root["globalEaseOut"]) {
            bool easeOut = root["globalEaseOut"].as<bool>();
            state->m_globalEaseOut = easeOut;
        }
        
        if (root["showMenusDuringPlayback"]) {
            bool showMenus = root["showMenusDuringPlayback"].as<bool>();
            state->m_showMenusDuringPlayback = showMenus;
        }
        
        if (root["allowUserRotation"]) {
            bool allowRotation = root["allowUserRotation"].as<bool>();
            state->m_allowUserRotation = allowRotation;
        }
        
        if (root["followGround"]) {
            bool followGround = root["followGround"].as<bool>();
            state->m_followGround = followGround;
        }
        
        if (root["minHeightAboveGround"]) {
            float minHeight = root["minHeightAboveGround"].as<float>();
            state->m_minHeightAboveGround = minHeight;
        }
        
        float rotationConversionFactor = 1.0f;  // Default: radians (no conversion)
        
        if (root["useDegrees"]) {
            bool useDegrees = root["useDegrees"].as<bool>();
            if (useDegrees) {
                rotationConversionFactor = PI / 180.0f;  // Convert degrees to radians
            }
        }
        
        bool importTranslationSuccess = state->m_timeline.AddTranslationPathFromFile(fullPath.string(), a_timeOffset);
        bool importRotationSuccess = state->m_timeline.AddRotationPathFromFile(fullPath.string(), a_timeOffset, rotationConversionFactor);
        bool importFOVSuccess = state->m_timeline.AddFOVPathFromFile(fullPath.string(), a_timeOffset);
        
        if (!importTranslationSuccess) {
            log::error("{}: Failed to import translation points from YAML file: {}", __FUNCTION__, a_filePath);
            return false;
        }
        
        if (!importRotationSuccess) {
            log::error("{}: Failed to import rotation points from YAML file: {}", __FUNCTION__, a_filePath);
            return false;
        }
        
        if (!importFOVSuccess) {
            log::error("{}: Failed to import FOV points from YAML file: {}", __FUNCTION__, a_filePath);
            return false;
        }
        
        return true;
    }
    
    bool TimelineManager::ExportTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, const char* a_filePath) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        std::filesystem::path fullPath = std::filesystem::current_path() / "Data" / a_filePath;
        
		log::info("{}: Exporting timeline to YAML file: {}", __FUNCTION__, a_filePath);
		
		std::ofstream file(fullPath);
		if (!file.is_open()) {
			log::error("{}: Failed to open file for writing: {}", __FUNCTION__, fullPath.string());
			return false;
		}
		
		file << "# FreeCameraFramework Timeline (YAML format)\n";
		file << "formatVersion: 1\n\n";
		
    	file << "playbackMode: " << PlaybackModeToString(state->m_timeline.GetPlaybackMode()) << "\n";
    	file << "loopTimeOffset: " << state->m_timeline.GetLoopTimeOffset() << "\n";
    	file << "globalEaseIn: " << (state->m_globalEaseIn ? "true" : "false") << "\n";
    	file << "globalEaseOut: " << (state->m_globalEaseOut ? "true" : "false") << "\n";
		file << "showMenusDuringPlayback: " << (state->m_showMenusDuringPlayback ? "true" : "false") << "\n";
		file << "allowUserRotation: " << (state->m_allowUserRotation ? "true" : "false") << "\n";
		file << "followGround: " << (state->m_followGround ? "true" : "false") << "\n";
		file << "minHeightAboveGround: " << state->m_minHeightAboveGround << "\n";
		file << "useDegrees: true\n\n";
		
		// Export translation, rotation, and FOV paths to same file
		bool exportTranslationSuccess = state->m_timeline.ExportTranslationPath(file);
		file << "\n";  // Separate the sections
		bool exportRotationSuccess = state->m_timeline.ExportRotationPath(file, 180.0f / PI);
		file << "\n";  // Separate the sections
		bool exportFOVSuccess = state->m_timeline.ExportFOVPath(file);
		
		file.close();
		
		if (!exportTranslationSuccess || !exportRotationSuccess || !exportFOVSuccess) {
			log::error("{}: Failed to export timeline to YAML file: {}", __FUNCTION__, a_filePath);
			return false;
		}
		
        return true;
    }

    bool TimelineManager::RegisterPlugin(SKSE::PluginHandle a_pluginHandle) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        // Check if plugin was already registered
        if (m_registeredPlugins.contains(a_pluginHandle)) {
            log::info("{}: Plugin {} re-registering, cleaning up orphaned timelines", __FUNCTION__, a_pluginHandle);
            CleanupPluginTimelines(a_pluginHandle);
        }

        m_registeredPlugins.insert(a_pluginHandle);
        return true;
    }

    void TimelineManager::CleanupPluginTimelines(SKSE::PluginHandle a_pluginHandle) {
        std::vector<size_t> timelinesToRemove;
        
        // Collect timeline IDs to remove
        for (const auto& [timelineID, state] : m_timelines) {
            if (state.m_ownerHandle == a_pluginHandle) {
                timelinesToRemove.push_back(timelineID);
            }
        }
        
        // Remove timelines
        for (size_t timelineID : timelinesToRemove) {
            auto it = m_timelines.find(timelineID);
            if (it != m_timelines.end()) {
                TimelineState& state = it->second;
                
                // Stop any active operations
                if (state.m_isPlaybackRunning) {
                    log::info("{}: Stopping playback for orphaned timeline {} before cleanup", __FUNCTION__, timelineID);
                    if (m_activeTimelineID == timelineID) {
                        // Exit free camera and restore UI state
                        auto* playerCamera = RE::PlayerCamera::GetSingleton();
                        if (playerCamera && playerCamera->currentState && playerCamera->currentState->id == RE::CameraState::kFree) {
                            ToggleFreeCameraNotHooked();
                            
                            auto* ui = RE::UI::GetSingleton();
                            if (ui && !state.m_showMenusDuringPlayback) {
                                ui->ShowMenus(m_isShowingMenus);  // Use global member
                            }
                        }
                        m_activeTimelineID = 0;
                    }
                    state.m_isPlaybackRunning = false;
                }
                
                if (state.m_isRecording) {
                    log::info("{}: Stopping recording for orphaned timeline {} before cleanup", __FUNCTION__, timelineID);
                    if (m_activeTimelineID == timelineID) {
                        m_activeTimelineID = 0;
                    }
                    state.m_isRecording = false;
                }
                
                m_timelines.erase(it);
            }
        }
    }

    size_t TimelineManager::RegisterTimeline(SKSE::PluginHandle a_pluginHandle) {        
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        // Require plugin registration first
        if (!m_registeredPlugins.contains(a_pluginHandle)) {
            log::error("{}: Plugin {} must call RegisterPlugin() before RegisterTimeline()", __FUNCTION__, a_pluginHandle);
            return 0;
        }
        
        size_t newID = m_nextTimelineID.fetch_add(1);
        
        TimelineState state;
        state.Initialize(newID, a_pluginHandle);
        
        log::info("{}: Timeline {} registered by plugin '{}' (handle {})", __FUNCTION__, newID, state.m_ownerName, a_pluginHandle);
        
        m_timelines[newID] = std::move(state);
        
        return newID;
    }

    bool TimelineManager::UnregisterTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        // Stop any active operations before unregistering
        if (m_activeTimelineID == a_timelineID) {
            if (state->m_isPlaybackRunning) {
                log::info("{}: Stopping playback before unregistering timeline {}", __FUNCTION__, a_timelineID);
                StopPlayback(a_pluginHandle, a_timelineID);
            } else if (state->m_isRecording) {
                log::info("{}: Stopping recording before unregistering timeline {}", __FUNCTION__, a_timelineID);
                StopRecording(a_pluginHandle, a_timelineID);
            }
        }
        
        log::info("{}: Timeline {} unregistered (owner: {})", __FUNCTION__, a_timelineID, state->m_ownerName);
        m_timelines.erase(a_timelineID);
        return true;
    }

    TimelineState* TimelineManager::GetTimeline(size_t a_timelineID, SKSE::PluginHandle a_pluginHandle) {
        if (a_pluginHandle == 0 || a_timelineID == 0) {
            return nullptr;
        }

        auto it = m_timelines.find(a_timelineID);
        if (it == m_timelines.end()) {
            log::error("{}: Timeline {} not found", __FUNCTION__, a_timelineID);
            return nullptr;
        }
        
        if (it->second.m_ownerHandle != a_pluginHandle) {
            log::error("{}: Plugin handle {} does not own timeline {} (owned by handle {})", 
                       __FUNCTION__, a_pluginHandle, a_timelineID, it->second.m_ownerHandle);
            return nullptr;
        }
        
        return &it->second;
    }

    const TimelineState* TimelineManager::GetTimeline(size_t a_timelineID, SKSE::PluginHandle a_pluginHandle) const {
        return const_cast<TimelineManager*>(this)->GetTimeline(a_timelineID, a_pluginHandle);
    }

    // Overloads for internal use (no ownership validation)
    bool TimelineManager::IsPlaybackRunning(size_t a_timelineID) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        auto it = m_timelines.find(a_timelineID);
        if (it == m_timelines.end()) {
            return false;
        }
        
        return it->second.m_isPlaybackRunning;
    }

    bool TimelineManager::IsUserRotationAllowed(size_t a_timelineID) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        auto it = m_timelines.find(a_timelineID);
        if (it == m_timelines.end()) {
            return false;
        }
        
        return it->second.m_allowUserRotation;
    }

    void TimelineManager::RecordTimeline(TimelineState* a_state) {
        if (!a_state || !a_state->m_isRecording) {
            return;
        }
        
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (!playerCamera) {
            return;
        }
        
        if (!(playerCamera->currentState && (playerCamera->currentState->id == RE::CameraState::kFree))) {
            // Auto-stop if no longer in free camera
            m_activeTimelineID = 0;
            a_state->m_isRecording = false;
            return;
        }
        
        a_state->m_currentRecordingTime += _ts_SKSEFunctions::GetRealTimeDeltaTime();
        
        if (a_state->m_recordingInterval == 0.0f || 
            (a_state->m_currentRecordingTime - a_state->m_lastRecordedPointTime >= a_state->m_recordingInterval)) {
            // Capture camera position/rotation/FOV as kWorld points
            RE::NiPoint3 cameraPos = _ts_SKSEFunctions::GetCameraPos();
            RE::NiPoint3 cameraRot = _ts_SKSEFunctions::GetCameraRotation(); // currently does not provide roll
            float fov = playerCamera ? playerCamera->worldFOV : 80.0f;
            
            Transition transTranslation(a_state->m_currentRecordingTime, InterpolationMode::kCubicHermite, false, false);
            TranslationPoint translationPoint(transTranslation, PointType::kWorld, cameraPos);
            a_state->m_timeline.AddTranslationPoint(translationPoint);
            
            Transition transRotation(a_state->m_currentRecordingTime, InterpolationMode::kCubicHermite, false, false);
            float cameraRoll = Hooks::FreeCameraRollHook::GetFreeCameraRoll(); // obtain roll via hook since _ts_SKSEFunctions::GetCameraRotation() doesn't provide it
            RotationPoint rotationPoint(transRotation, PointType::kWorld, RE::NiPoint3{cameraRot.x, cameraRoll, cameraRot.z});
            a_state->m_timeline.AddRotationPoint(rotationPoint);
            
            Transition transFOV(a_state->m_currentRecordingTime, InterpolationMode::kCubicHermite, false, false);
            FOVPoint fovPoint(transFOV, fov);
            a_state->m_timeline.AddFOVPoint(fovPoint);
            
            a_state->m_lastRecordedPointTime = a_state->m_currentRecordingTime;
        }
    }

    void TimelineManager::OnPreSaveGame() {
        if (!m_activeTimelineID) {
            return;
        }

        // Temporarily exit free camera
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (playerCamera && playerCamera->IsInFreeCameraMode()) {
            ToggleFreeCameraNotHooked();
        }
        
        m_isSaveInProgress = true;
    }

    void TimelineManager::OnPostSaveGame() {

        if (!m_isSaveInProgress) {
            return;
        }

        m_isSaveInProgress = false;

        if (!m_activeTimelineID) {
            return;
        }

        // Re-enter free camera mode
        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (playerCamera && !playerCamera->IsInFreeCameraMode()) {
            ToggleFreeCameraNotHooked();
        }
    }

} // namespace FCFW
