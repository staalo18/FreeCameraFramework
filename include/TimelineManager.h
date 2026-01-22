#pragma once

#include "Timeline.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace FCFW {
    // Per-timeline state container
    struct TimelineState {
        // ===== IDENTITY & OWNERSHIP (immutable after creation) =====
        size_t m_id;                           // Timeline unique identifier
        SKSE::PluginHandle m_ownerHandle;      // Plugin that owns this timeline
        std::string m_ownerName;               // Plugin name (for logging)
        
        // ===== TIMELINE DATA & STATIC CONFIGURATION (persisted in YAML) =====
        Timeline m_timeline;                   // Paired translation + rotation tracks
        bool m_globalEaseIn{ false };          // Apply easing to timeline start (user preference)
        bool m_globalEaseOut{ false };         // Apply easing to timeline end (user preference)
        bool m_showMenusDuringPlayback{ false }; // UI visibility during playback (user preference)
        bool m_allowUserRotation{ false };     // Allow user camera control during playback (user preference)
        
        // ===== RECORDING STATE (runtime, reset on StopRecording) =====
        bool m_isRecording{ false };           // Currently capturing camera
        float m_currentRecordingTime{ 0.0f };  // Elapsed time during recording
        float m_lastRecordedPointTime{ 0.0f }; // Last sample timestamp
        float m_recordingInterval{ 1.0f };     // Sample interval for this recording session (0.0 = every frame)
        
        // ===== PLAYBACK STATE (runtime, reset on StopPlayback) =====
        bool m_isPlaybackRunning{ false };     // Active playback
        float m_playbackSpeed{ 1.0f };         // Computed time multiplier (runtime only, NOT persisted)
        float m_playbackDuration{ 0.0f };      // Computed total duration (runtime only, NOT persisted)
        bool m_isCompletedAndWaiting{ false }; // kWait mode completion flag (runtime only)
        RE::BSTPoint2<float> m_rotationOffset{ 0.0f, 0.0f }; // Accumulated user rotation (runtime only)
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

            bool StartRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_recordingInterval = 1.0f, bool a_append = false, float a_timeOffset = 0.0f);
            bool StopRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID);

            int AddTranslationPointAtCamera(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode);
            int AddTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, const RE::NiPoint3& a_position, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode);
            int AddTranslationPointAtRef(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, RE::TESObjectREFR* a_reference, const RE::NiPoint3& a_offset, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode);
            int AddRotationPointAtCamera(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode);
            int AddRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, const RE::BSTPoint2<float>& a_rotation, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode);
            int AddRotationPointAtRef(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, RE::TESObjectREFR* a_reference, const RE::BSTPoint2<float>& a_offset, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, InterpolationMode a_interpolationMode);
            
            bool RemoveTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index);
            bool RemoveRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index);

            bool ClearTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID);
            
            int GetTranslationPointCount(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const;
            int GetRotationPointCount(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const;
            
            RE::NiPoint3 GetTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const;
            RE::BSTPoint2<float> GetRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const;
            
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
            bool SetPlaybackMode(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, int a_playbackMode, float a_loopTimeOffset = 0.0f);
            
            // Overloads for internal use (no ownership validation - for hooks)
            bool IsPlaybackRunning(size_t a_timelineID) const;
            bool IsUserRotationAllowed(size_t a_timelineID) const;
            size_t GetActiveTimelineID() const { return m_activeTimelineID; }    
                                    
            bool AddTimelineFromFile(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, const char* a_filePath, float a_timeOffset = 0.0f); // Requires ownership
            bool ExportTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, const char* a_filePath) const;

            bool RegisterPlugin(SKSE::PluginHandle a_pluginHandle);
            size_t RegisterTimeline(SKSE::PluginHandle a_pluginHandle);
            bool UnregisterTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID);
            
            // Papyrus event registration
            void RegisterForTimelineEvents(RE::TESForm* a_form);
            void UnregisterForTimelineEvents(RE::TESForm* a_form);
            
            // Save/load handlers
            void OnPreSaveGame();
            void OnPostSaveGame();

        private:
            TimelineManager() = default;
            ~TimelineManager() = default;
            
           void DispatchTimelineEvent(uint32_t a_messageType, size_t a_timelineID);
           void DispatchTimelineEventPapyrus(const char* a_eventName, size_t a_timelineID);

            void RecordTimeline(TimelineState* a_state);
            void PlayTimeline(TimelineState* a_state);

            TimelineState* GetTimeline(size_t a_timelineID, SKSE::PluginHandle a_pluginHandle);
            const TimelineState* GetTimeline(size_t a_timelineID, SKSE::PluginHandle a_pluginHandle) const;
            
            void CleanupPluginTimelines(SKSE::PluginHandle a_pluginHandle);

            void RecenterGridAroundCameraIfNeeded();

            std::unordered_set<SKSE::PluginHandle> m_registeredPlugins; // Track registered plugins
            std::unordered_map<size_t, TimelineState> m_timelines;
            mutable std::recursive_mutex m_timelineMutex;  // Protect map operations (recursive for reentrant safety)
            std::atomic<size_t> m_nextTimelineID = 1;     // ID generator
            size_t m_activeTimelineID = 0;            // Which timeline is active (0 = none)
                        
            // Playback
            bool m_isShowingMenus = true;         // Whether menus were showing before playback started
            bool m_userTurning = false;           // Whether user is manually controlling camera during playback
            RE::NiPoint2 m_lastFreeRotation;           // camera free rotation before playback started (third-person only)
            
            // Papyrus event registration
            std::vector<RE::TESForm*> m_eventReceivers;  // Forms registered for timeline events
            
            // Savegame handling
            bool m_isSaveInProgress = false; // Flag to indicate save is in progress
    }; // class TimelineManager
} // namespace FCFW
