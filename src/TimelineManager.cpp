#include "TimelineManager.h"
#include "FCFW_Utils.h"
#include <yaml-cpp/yaml.h>

namespace FCFW {

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
            
log::info("{}: Queued Papyrus event '{}' to form 0x{:X}", __FUNCTION__, a_eventName, receiver->GetFormID());
        }
        
log::info("{}: Sent Papyrus event '{}' for timeline {} to {} receivers", __FUNCTION__, a_eventName, a_timelineID, m_eventReceivers.size());
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

    bool TimelineManager::StartRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) {
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

        // Enter free camera mode
        playerCamera->ToggleFreeCameraMode(false);

        // Set as active timeline
        m_activeTimelineID = a_timelineID;
        state->m_isRecording = true;
        state->m_currentRecordingTime = 0.0f;
        state->m_lastRecordedPointTime = -m_recordingInterval;  // Ensure first point is captured immediately
        
        // Clear existing points
        state->m_timeline.ClearPoints();
        
        // Add initial point
        RE::NiPoint3 cameraPos = _ts_SKSEFunctions::GetCameraPos();
        RE::NiPoint3 cameraRot = _ts_SKSEFunctions::GetCameraRotation();
        
        Transition transTranslation(0.0f, InterpolationMode::kCubicHermite, true, false);
        TranslationPoint translationPoint(transTranslation, PointType::kWorld, cameraPos);
        state->m_timeline.AddTranslationPoint(translationPoint);
        
        Transition transRotation(0.0f, InterpolationMode::kCubicHermite, true, false);
        RotationPoint rotationPoint(transRotation, PointType::kWorld, RE::BSTPoint2<float>({cameraRot.x, cameraRot.z}));
        state->m_timeline.AddRotationPoint(rotationPoint);
        
        RE::DebugNotification("Starting camera path recording...");
        log::info("{}: Started recording on timeline {}", __FUNCTION__, a_timelineID);
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
        
        Transition transTranslation(state->m_currentRecordingTime, InterpolationMode::kCubicHermite, false, true);
        TranslationPoint translationPoint(transTranslation, PointType::kWorld, cameraPos);
        state->m_timeline.AddTranslationPoint(translationPoint);
        
        Transition transRotation(state->m_currentRecordingTime, InterpolationMode::kCubicHermite, false, true);
        RotationPoint rotationPoint(transRotation, PointType::kWorld, RE::BSTPoint2<float>({cameraRot.x, cameraRot.z}));
        state->m_timeline.AddRotationPoint(rotationPoint);
        
        playerCamera->ToggleFreeCameraMode(false);
        
        // Clear recording state
        m_activeTimelineID = 0;
        state->m_isRecording = false;
        
        RE::DebugNotification("Camera path recording stopped.");
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

    int TimelineManager::AddTranslationPointAtRef(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, RE::TESObjectREFR* a_reference, const RE::NiPoint3& a_offset, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode) {
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
        TranslationPoint point(transition, PointType::kReference, RE::NiPoint3{}, a_offset, a_reference, a_isOffsetRelative);
        
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

    int TimelineManager::AddRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, const RE::BSTPoint2<float>& a_rotation, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode) {
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

    int TimelineManager::AddRotationPointAtRef(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, RE::TESObjectREFR* a_reference, const RE::BSTPoint2<float>& a_offset, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode) {
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
        RotationPoint point(transition, PointType::kReference, RE::BSTPoint2<float>{}, a_offset, a_reference, a_isOffsetRelative);
        
        return static_cast<int>(state->m_timeline.AddRotationPoint(point));
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
        cameraState->translation = a_state->m_timeline.GetTranslation(sampleTime);
        RE::BSTPoint2<float> rotation = a_state->m_timeline.GetRotation(sampleTime);
        
        // Handle user rotation
        if (m_userTurning && a_state->m_allowUserRotation) {
            a_state->m_rotationOffset.x = _ts_SKSEFunctions::NormalRelativeAngle(cameraState->rotation.x - rotation.x);
            a_state->m_rotationOffset.y = _ts_SKSEFunctions::NormalRelativeAngle(cameraState->rotation.y - rotation.y);
            m_userTurning = false;
        } else {
            cameraState->rotation.x = _ts_SKSEFunctions::NormalRelativeAngle(rotation.x + a_state->m_rotationOffset.x);
            cameraState->rotation.y = _ts_SKSEFunctions::NormalRelativeAngle(rotation.y + a_state->m_rotationOffset.y);
        }
        
        if (a_state->m_timeline.GetPlaybackMode() == PlaybackMode::kWait) {
            float playbackTime = a_state->m_timeline.GetPlaybackTime();
            float timelineDuration = a_state->m_timeline.GetDuration();
            if ((playbackTime >= timelineDuration) && !a_state->m_isCompletedAndWaiting) {
                DispatchTimelineEvent(static_cast<uint32_t>(FCFW_API::FCFWMessage::kTimelinePlaybackCompleted), a_state->m_id);
                DispatchTimelineEventPapyrus("OnTimelinePlaybackCompleted", a_state->m_id);
                a_state->m_isCompletedAndWaiting = true;
            }
            // Keep playback running - user must manually call StopPlayback
        } else if (!a_state->m_timeline.IsPlaying()) {
            size_t timelineID = a_state->m_id;
            SKSE::PluginHandle ownerHandle = a_state->m_ownerHandle;
            StopPlayback(ownerHandle, timelineID);
        }
    }

    bool TimelineManager::ClearTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, bool a_notifyUser) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        if (state->m_isRecording) {
            return false;
        }
        
        if (a_notifyUser) {
            RE::DebugNotification("Clearing camera path...");
        }
        
        if (state->m_isPlaybackRunning) {
            log::info("{}: Timeline modified during playback, stopping playback", __FUNCTION__);
            StopPlayback(a_pluginHandle, a_timelineID);
        }
        
        state->m_timeline.ClearPoints();
        
        // Reset timeline metadata to defaults
        state->m_timeline.SetLoopTimeOffset(0.0f);
        state->m_timeline.SetPlaybackMode(PlaybackMode::kEnd);
        state->m_globalEaseIn = false;
        state->m_globalEaseOut = false;
        state->m_showMenusDuringPlayback = false;
        state->m_allowUserRotation = false;
        
        return true;
    }

    bool TimelineManager::StartPlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_speed, bool a_globalEaseIn, bool a_globalEaseOut, bool a_useDuration, float a_duration) {
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
        state->m_rotationOffset = { 0.0f, 0.0f };  // Reset per-timeline rotation offset
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
        
        // Initialize timeline playback
        state->m_timeline.ResetPlayback();
        state->m_timeline.StartPlayback();
        
        // Handle UI visibility
        auto* ui = RE::UI::GetSingleton();
        if (ui) {
            m_isShowingMenus = ui->IsShowingMenus();
            ui->ShowMenus(state->m_showMenusDuringPlayback);
        }
        
        // Enter free camera mode
        playerCamera->ToggleFreeCameraMode(false);
        
        log::info("{}: Started playback on timeline {}", __FUNCTION__, a_timelineID);
        
        // Dispatch playback started event
        DispatchTimelineEvent(static_cast<uint32_t>(FCFW_API::FCFWMessage::kTimelinePlaybackStarted), a_timelineID);
        DispatchTimelineEventPapyrus("OnTimelinePlaybackStarted", a_timelineID);
        
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
            if (playerCamera->IsInFreeCameraMode()) {
                playerCamera->ToggleFreeCameraMode(false);
            } else {
                log::warn("{}: Not in free camera mode", __FUNCTION__);
            }

            
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
        }
        
        // Clear active state
        m_activeTimelineID = 0;
        state->m_isPlaybackRunning = false;
        
        log::info("{}: Stopped playback on timeline {}", __FUNCTION__, a_timelineID);
        
        // Dispatch playback stopped event
        DispatchTimelineEvent(static_cast<uint32_t>(FCFW_API::FCFWMessage::kTimelinePlaybackStopped), a_timelineID);
        DispatchTimelineEventPapyrus("OnTimelinePlaybackStopped", a_timelineID);
        
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
        DispatchTimelineEvent(static_cast<uint32_t>(FCFW_API::FCFWMessage::kTimelinePlaybackStopped), a_fromTimelineID);
        DispatchTimelineEventPapyrus("OnTimelinePlaybackStopped", a_fromTimelineID);
        
        // Initialize target timeline (StartPlayback will call UpdateCameraPoints internally)
        toState->m_timeline.ResetPlayback();
        toState->m_timeline.StartPlayback();
        
        // Copy playback settings from source timeline
        toState->m_playbackSpeed = fromState->m_playbackSpeed;
        
        // Only preserve rotation offset if target timeline allows user rotation
        // If target doesn't allow user rotation, reset to zero so timeline plays its intended rotation
        if (toState->m_allowUserRotation) {
            toState->m_rotationOffset = fromState->m_rotationOffset;
        } else {
            toState->m_rotationOffset = { 0.0f, 0.0f };
        }
        
        toState->m_showMenusDuringPlayback = fromState->m_showMenusDuringPlayback;
        toState->m_globalEaseIn = fromState->m_globalEaseIn;
        toState->m_globalEaseOut = fromState->m_globalEaseOut;
        
        // Activate target timeline (camera stays in free mode)
        m_activeTimelineID = a_toTimelineID;
        toState->m_isPlaybackRunning = true;
        toState->m_isCompletedAndWaiting = false;
        
        // Dispatch start event for target timeline
        DispatchTimelineEvent(static_cast<uint32_t>(FCFW_API::FCFWMessage::kTimelinePlaybackStarted), a_toTimelineID);
        DispatchTimelineEventPapyrus("OnTimelinePlaybackStarted", a_toTimelineID);
        
        log::info("{}: Successfully switched to timeline {}", __FUNCTION__, a_toTimelineID);
        return true;
    }

    void TimelineManager::SetUserTurning(bool a_turning) {
        m_userTurning = a_turning;
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

    RE::BSTPoint2<float> TimelineManager::GetRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        const TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            log::error("{}: Timeline {} not found or not owned by plugin handle {}", __FUNCTION__, a_timelineID, a_pluginHandle);
            return RE::BSTPoint2<float>{0.0f, 0.0f};
        }
        
        if (a_index >= state->m_timeline.GetRotationPointCount()) {
            log::error("{}: Index {} out of range (timeline {} has {} rotation points)", __FUNCTION__, a_index, a_timelineID, state->m_timeline.GetRotationPointCount());
            return RE::BSTPoint2<float>{0.0f, 0.0f};
        }
        
        return state->m_timeline.GetRotationPoint(a_index);
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

    bool TimelineManager::SetPlaybackMode(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, int a_playbackMode) {
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        TimelineState* state = GetTimeline(a_timelineID, a_pluginHandle);
        if (!state) {
            return false;
        }
        
        if (a_playbackMode < 0 || a_playbackMode > 2) {
            log::error("{}: Invalid playback mode {} for timeline {}", __FUNCTION__, a_playbackMode, a_timelineID);
            return false;
        }
        
        PlaybackMode mode = static_cast<PlaybackMode>(a_playbackMode);
        state->m_timeline.SetPlaybackMode(mode);
        
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
        
log::info("{}: Loading timeline from YAML file: {}", __FUNCTION__, a_filePath);
        
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
        
        float rotationConversionFactor = 1.0f;  // Default: radians (no conversion)
        
        if (root["useDegrees"]) {
            bool useDegrees = root["useDegrees"].as<bool>();
            if (useDegrees) {
                rotationConversionFactor = PI / 180.0f;  // Convert degrees to radians
            }
        }
        
        size_t translationPointCount = state->m_timeline.GetTranslationPointCount();
        size_t rotationPointCount = state->m_timeline.GetRotationPointCount();
        
        bool importTranslationSuccess = state->m_timeline.AddTranslationPathFromFile(fullPath.string(), a_timeOffset);
        bool importRotationSuccess = state->m_timeline.AddRotationPathFromFile(fullPath.string(), a_timeOffset, rotationConversionFactor);
        
        if (!importTranslationSuccess) {
            log::error("{}: Failed to import translation points from YAML file: {}", __FUNCTION__, a_filePath);
            return false;
        }
        
        if (!importRotationSuccess) {
            log::error("{}: Failed to import rotation points from YAML file: {}", __FUNCTION__, a_filePath);
            return false;
        }
        
log::info("{}: Loaded {} translation and {} rotation points from {} to timeline {}", 
__FUNCTION__, state->m_timeline.GetTranslationPointCount() - translationPointCount, 
state->m_timeline.GetRotationPointCount() - rotationPointCount, a_filePath, a_timelineID);

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
		
    	file << "playbackMode: " << PlaybackModeToString(state->m_timeline.GetPlaybackMode()) << "  # end, loop, or wait\n";
    	file << "loopTimeOffset: " << state->m_timeline.GetLoopTimeOffset() << "\n";
    	file << "globalEaseIn: " << (state->m_globalEaseIn ? "true" : "false") << "\n";
    	file << "globalEaseOut: " << (state->m_globalEaseOut ? "true" : "false") << "\n";
		file << "showMenusDuringPlayback: " << (state->m_showMenusDuringPlayback ? "true" : "false") << "\n";
		file << "allowUserRotation: " << (state->m_allowUserRotation ? "true" : "false") << "\n";
		file << "useDegrees: true\n\n";
		
		// Export both translation and rotation paths to same file
		bool exportTranslationSuccess = state->m_timeline.ExportTranslationPath(file);
		file << "\n";  // Separate the two sections
		bool exportRotationSuccess = state->m_timeline.ExportRotationPath(file, 180.0f / PI);
		
		file.close();
		
		if (!exportTranslationSuccess || !exportRotationSuccess) {
			log::error("{}: Failed to export timeline to YAML file: {}", __FUNCTION__, a_filePath);
			return false;
		}
		
log::info("{}: Exported {} translation and {} rotation points from timeline {} to {}", 
__FUNCTION__, state->m_timeline.GetTranslationPointCount(), 
state->m_timeline.GetRotationPointCount(), a_timelineID, a_filePath);
        return true;
    }

    size_t TimelineManager::RegisterTimeline(SKSE::PluginHandle a_pluginHandle) {
        log::info("{}: ENTER - Plugin handle {}", __FUNCTION__, a_pluginHandle);
        
        std::lock_guard<std::recursive_mutex> lock(m_timelineMutex);
        
        size_t newID = m_nextTimelineID.fetch_add(1);
        log::info("{}: Generated new ID {} (counter now at {})", __FUNCTION__, newID, m_nextTimelineID.load());
        
        TimelineState state;
        state.m_id = newID;
        state.m_timeline = Timeline();
        state.m_ownerHandle = a_pluginHandle;
        
        state.m_ownerName = std::format("Plugin_{}", a_pluginHandle);
        log::info("{}: Created state with owner name '{}'", __FUNCTION__, state.m_ownerName);
        
        // Log before move to avoid use-after-move undefined behavior
        log::info("{}: Timeline {} registered by plugin '{}' (handle {})", __FUNCTION__, newID, state.m_ownerName, a_pluginHandle);
        
        m_timelines[newID] = std::move(state);
        log::info("{}: Inserted into map, returning ID {}", __FUNCTION__, newID);
        
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
        
        if (a_state->m_currentRecordingTime - a_state->m_lastRecordedPointTime >= m_recordingInterval) {
            // Capture camera position/rotation as kWorld points
            RE::NiPoint3 cameraPos = _ts_SKSEFunctions::GetCameraPos();
            RE::NiPoint3 cameraRot = _ts_SKSEFunctions::GetCameraRotation();
            
            Transition transTranslation(a_state->m_currentRecordingTime, InterpolationMode::kCubicHermite, false, false);
            TranslationPoint translationPoint(transTranslation, PointType::kWorld, cameraPos);
            a_state->m_timeline.AddTranslationPoint(translationPoint);
            
            Transition transRotation(a_state->m_currentRecordingTime, InterpolationMode::kCubicHermite, false, false);
            RotationPoint rotationPoint(transRotation, PointType::kWorld, RE::BSTPoint2<float>({cameraRot.x, cameraRot.z}));
            a_state->m_timeline.AddRotationPoint(rotationPoint);
            
            a_state->m_lastRecordedPointTime = a_state->m_currentRecordingTime;
        }
    }

} // namespace FCFW
