#pragma once

#include "Timeline.h"
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace FCFW {
    // Per-timeline state container
    struct TimelineState {
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

    class TimelineManager {
        public:
            static TimelineManager& GetSingleton() {
                static TimelineManager instance;
                return instance;
            }
            TimelineManager(const TimelineManager&) = delete;
            TimelineManager& operator=(const TimelineManager&) = delete;

            void Update();

            bool StartRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID);
            bool StopRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID);

            int AddTranslationPointAtCamera(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode);
            int AddTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, float a_posX, float a_posY, float a_posZ, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode);
            int AddTranslationPointAtRef(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, RE::TESObjectREFR* a_reference, float a_offsetX, float a_offsetY, float a_offsetZ, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode);
            int AddRotationPointAtCamera(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode);
            int AddRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, float a_pitch, float a_yaw, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode);
            int AddRotationPointAtRef(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, RE::TESObjectREFR* a_reference, float a_offsetPitch, float a_offsetYaw, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode);
            
            bool RemoveTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index);
            bool RemoveRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index);

            bool ClearTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, bool a_notifyUser = true);
            
            int GetTranslationPointCount(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const;
            int GetRotationPointCount(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const;
            
            bool StartPlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_speed = 1.0f, bool a_globalEaseIn = false, bool a_globalEaseOut = false, bool a_useDuration = false, float a_duration = 0.0f);
            bool StopPlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID);
            bool SwitchPlayback(SKSE::PluginHandle a_pluginHandle, size_t a_fromTimelineID, size_t a_toTimelineID);
            bool IsPlaybackRunning(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const;
            bool IsRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const;
            bool PausePlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID);
            bool ResumePlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID);
            bool IsPlaybackPaused(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const;
            void SetUserTurning(bool a_turning);
            bool AllowUserRotation(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, bool a_allow);
            bool IsUserRotationAllowed(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const;
            bool SetPlaybackMode(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, int a_playbackMode);
            
            // Overloads for internal use (no ownership validation - for hooks)
            bool IsPlaybackRunning(size_t a_timelineID) const;
            bool IsUserRotationAllowed(size_t a_timelineID) const;
            size_t GetActiveTimelineID() const { return m_activeTimelineID; }    
                                    
            bool AddTimelineFromFile(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, const char* a_filePath, float a_timeOffset = 0.0f); // Requires ownership
            bool ExportTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, const char* a_filePath) const;

            size_t RegisterTimeline(SKSE::PluginHandle a_pluginHandle);
            bool UnregisterTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID);
            
            // Papyrus event registration
            void RegisterForTimelineEvents(RE::TESForm* a_form);
            void UnregisterForTimelineEvents(RE::TESForm* a_form);
            
        private:
            TimelineManager() = default;
            ~TimelineManager() = default;
            
           void DispatchTimelineEvent(uint32_t a_messageType, size_t a_timelineID);
           void DispatchTimelineEventPapyrus(const char* a_eventName, size_t a_timelineID);

            void DrawTimeline(const TimelineState* a_state);
            void RecordTimeline(TimelineState* a_state);
            void PlayTimeline(TimelineState* a_state);

            TimelineState* GetTimeline(size_t a_timelineID, SKSE::PluginHandle a_pluginHandle);
            const TimelineState* GetTimeline(size_t a_timelineID, SKSE::PluginHandle a_pluginHandle) const;

            std::unordered_map<size_t, TimelineState> m_timelines;
            mutable std::recursive_mutex m_timelineMutex;  // Protect map operations (recursive for reentrant safety)
            std::atomic<size_t> m_nextTimelineID{ 1 };     // ID generator
            size_t m_activeTimelineID{ 0 };            // Which timeline is active (0 = none)
            
            // Recording (shared across all timelines)
            float m_recordingInterval = 1.0f;         // Sample rate (1 point per second)
            
            // Playback
            bool m_isShowingMenus = true;         // Whether menus were showing before playback started
            bool m_showMenusDuringPlayback = false; // Whether to show menus during playback
            bool m_userTurning = false;           // Whether user is manually controlling camera during playback
            RE::NiPoint2 m_lastFreeRotation;           // camera free rotation before playback started (third-person only)
            
            // Papyrus event registration
            std::vector<RE::TESForm*> m_eventReceivers;  // Forms registered for timeline events
    }; // class TimelineManager
} // namespace FCFW