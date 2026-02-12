#include "Hooks.h"
#include "TimelineManager.h"
#include "ModAPI.h"
#include "CameraTypes.h"
#include "APIManager.h"

namespace FCFW {
    namespace Interface {
        int GetFCFWPluginVersion(RE::StaticFunctionTag*) {
            // Encode version as: major * 10000 + minor * 100 + patch
            return static_cast<int>(Plugin::VERSION[0]) * 10000 + 
                   static_cast<int>(Plugin::VERSION[1]) * 100 + 
                   static_cast<int>(Plugin::VERSION[2]);
        }

        bool RegisterPlugin(RE::StaticFunctionTag*, RE::BSFixedString a_modName) {
            if (a_modName.empty()) {
                log::error("{}: Empty mod name provided", __FUNCTION__);
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().RegisterPlugin(handle);
        }

        int RegisterTimeline(RE::StaticFunctionTag*, RE::BSFixedString a_modName) {
            if (a_modName.empty()) {
                log::error("{}: Empty mod name provided", __FUNCTION__);
                return -1;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return -1;
            }

            return static_cast<int>(FCFW::TimelineManager::GetSingleton().RegisterTimeline(handle));
        }

        bool UnregisterTimeline(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().UnregisterTimeline(handle, static_cast<size_t>(a_timelineID));
        }
        
        int AddTranslationPointAtCamera(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, float a_time, bool a_easeIn, bool a_easeOut, int a_interpolationMode) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return -1;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return -1;
            }

            return FCFW::TimelineManager::GetSingleton().AddTranslationPointAtCamera(handle, static_cast<size_t>(a_timelineID), a_time, a_easeIn, a_easeOut, ToInterpolationMode(a_interpolationMode));
        }

        int AddTranslationPoint(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, float a_time, float a_posX, float a_posY, float a_posZ, bool a_easeIn, bool a_easeOut, int a_interpolationMode) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return -1;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return -1;
            }

            RE::NiPoint3 position(a_posX, a_posY, a_posZ);
            return FCFW::TimelineManager::GetSingleton().AddTranslationPoint(handle, static_cast<size_t>(a_timelineID), a_time, position, a_easeIn, a_easeOut, ToInterpolationMode(a_interpolationMode));
        }

        int AddTranslationPointAtRef(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, float a_time, RE::TESObjectREFR* a_reference, int a_bodyPart, float a_offsetX, float a_offsetY, float a_offsetZ, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, int a_interpolationMode) {
            if (a_modName.empty() || a_timelineID <= 0) {
log::error("{}: Invalid mod name '{}' or timeline ID {}", __FUNCTION__, a_modName.c_str(), a_timelineID);
                return -1;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return -1;
            }

            RE::NiPoint3 offset(a_offsetX, a_offsetY, a_offsetZ);
            return FCFW::TimelineManager::GetSingleton().AddTranslationPointAtRef(handle, static_cast<size_t>(a_timelineID), a_time, a_reference, ToBodyPart(a_bodyPart), offset, a_isOffsetRelative, a_easeIn, a_easeOut, ToInterpolationMode(a_interpolationMode));
        }

        int AddRotationPointAtCamera(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, float a_time, bool a_easeIn, bool a_easeOut, int a_interpolationMode) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return -1;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return -1;
            }

            return FCFW::TimelineManager::GetSingleton().AddRotationPointAtCamera(handle, static_cast<size_t>(a_timelineID), a_time, a_easeIn, a_easeOut, ToInterpolationMode(a_interpolationMode));
        }

        int AddRotationPoint(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, float a_time, float a_pitch, float a_yaw, bool a_easeIn, bool a_easeOut, int a_interpolationMode) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return -1;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return -1;
            }

            RE::BSTPoint2<float> rotation{a_pitch, a_yaw};
            return FCFW::TimelineManager::GetSingleton().AddRotationPoint(handle, static_cast<size_t>(a_timelineID), a_time, rotation, a_easeIn, a_easeOut, ToInterpolationMode(a_interpolationMode));
        }

        int AddRotationPointAtRef(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, float a_time, RE::TESObjectREFR* a_reference, int a_bodyPart, float a_offsetPitch, float a_offsetYaw, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, int a_interpolationMode) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return -1;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return -1;
            }

            RE::BSTPoint2<float> offset{a_offsetPitch, a_offsetYaw};
            return FCFW::TimelineManager::GetSingleton().AddRotationPointAtRef(handle, static_cast<size_t>(a_timelineID), a_time, a_reference, ToBodyPart(a_bodyPart), offset, a_isOffsetRelative, a_easeIn, a_easeOut, ToInterpolationMode(a_interpolationMode));
        }

        bool StartRecording(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, float a_recordingInterval, bool a_append, float a_timeOffset) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().StartRecording(handle, static_cast<size_t>(a_timelineID), a_recordingInterval, a_append, a_timeOffset);
        }

        bool StopRecording(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().StopRecording(handle, static_cast<size_t>(a_timelineID));
        }
        
        bool RemoveTranslationPoint(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, int a_index) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().RemoveTranslationPoint(handle, static_cast<size_t>(a_timelineID), static_cast<size_t>(a_index));
        }

        bool RemoveRotationPoint(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, int a_index) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().RemoveRotationPoint(handle, static_cast<size_t>(a_timelineID), static_cast<size_t>(a_index));
        }

        bool ClearTimeline(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
                if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().ClearTimeline(handle, static_cast<size_t>(a_timelineID));
        }

        int GetTranslationPointCount(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return 0;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return 0;
            }
            
            return FCFW::TimelineManager::GetSingleton().GetTranslationPointCount(handle, static_cast<size_t>(a_timelineID));
        }

        int GetRotationPointCount(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return 0;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return 0;
            }
            
            return FCFW::TimelineManager::GetSingleton().GetRotationPointCount(handle, static_cast<size_t>(a_timelineID));
        }

        float GetTranslationPointX(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, int a_index) {
            if (a_modName.empty() || a_timelineID <= 0 || a_index < 0) {
                return 0.0f;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return 0.0f;
            }

            RE::NiPoint3 point = FCFW::TimelineManager::GetSingleton().GetTranslationPoint(handle, static_cast<size_t>(a_timelineID), static_cast<size_t>(a_index));
            return point.x;
        }

        float GetTranslationPointY(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, int a_index) {
            if (a_modName.empty() || a_timelineID <= 0 || a_index < 0) {
                return 0.0f;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return 0.0f;
            }

            RE::NiPoint3 point = FCFW::TimelineManager::GetSingleton().GetTranslationPoint(handle, static_cast<size_t>(a_timelineID), static_cast<size_t>(a_index));
            return point.y;
        }

        float GetTranslationPointZ(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, int a_index) {
            if (a_modName.empty() || a_timelineID <= 0 || a_index < 0) {
                return 0.0f;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return 0.0f;
            }

            RE::NiPoint3 point = FCFW::TimelineManager::GetSingleton().GetTranslationPoint(handle, static_cast<size_t>(a_timelineID), static_cast<size_t>(a_index));
            return point.z;
        }

        float GetRotationPointPitch(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, int a_index) {
            if (a_modName.empty() || a_timelineID <= 0 || a_index < 0) {
                return 0.0f;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return 0.0f;
            }

            RE::BSTPoint2<float> point = FCFW::TimelineManager::GetSingleton().GetRotationPoint(handle, static_cast<size_t>(a_timelineID), static_cast<size_t>(a_index));
            return point.x;
        }

        float GetRotationPointYaw(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, int a_index) {
            if (a_modName.empty() || a_timelineID <= 0 || a_index < 0) {
                return 0.0f;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return 0.0f;
            }

            RE::BSTPoint2<float> point = FCFW::TimelineManager::GetSingleton().GetRotationPoint(handle, static_cast<size_t>(a_timelineID), static_cast<size_t>(a_index));
            return point.y;
        }

        bool StartPlayback(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, float a_speed, bool a_globalEaseIn, bool a_globalEaseOut, bool a_useDuration, float a_duration, bool a_followGround, float a_minHeightAboveGround, bool a_showMenusDuringPlayback, float a_startTime) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().StartPlayback(handle, static_cast<size_t>(a_timelineID), a_speed, a_globalEaseIn, a_globalEaseOut, a_useDuration, a_duration, a_followGround, a_minHeightAboveGround, a_showMenusDuringPlayback, a_startTime);
        }
        
        bool StopPlayback(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().StopPlayback(handle, static_cast<size_t>(a_timelineID));
        }
        
        bool SwitchPlayback(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_fromTimelineID, int a_toTimelineID) {
            if (a_modName.empty() || a_toTimelineID <= 0 || a_fromTimelineID < 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().SwitchPlayback(handle, static_cast<size_t>(a_fromTimelineID), static_cast<size_t>(a_toTimelineID));
        }
        
        bool PausePlayback(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().PausePlayback(handle, static_cast<size_t>(a_timelineID));
        }
        
        bool ResumePlayback(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().ResumePlayback(handle, static_cast<size_t>(a_timelineID));
        }

        bool IsPlaybackPaused(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }
        
            return FCFW::TimelineManager::GetSingleton().IsPlaybackPaused(handle, static_cast<size_t>(a_timelineID));
        }
        
        bool IsPlaybackRunning(RE::StaticFunctionTag*, RE::BSFixedString a_modName, std::int32_t a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }
        
            return FCFW::TimelineManager::GetSingleton().IsPlaybackRunning(handle, static_cast<size_t>(a_timelineID));
        }

        bool IsRecording(RE::StaticFunctionTag*, RE::BSFixedString a_modName, std::int32_t a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }
        
            return FCFW::TimelineManager::GetSingleton().IsRecording(handle, static_cast<size_t>(a_timelineID));
        }

        std::int32_t GetActiveTimelineID(RE::StaticFunctionTag*) {
            return static_cast<std::int32_t>(FCFW::TimelineManager::GetSingleton().GetActiveTimelineID());
        }

        float GetPlaybackTime(RE::StaticFunctionTag*, RE::BSFixedString a_modName, std::int32_t a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return -1.0f;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return -1.0f;
            }
        
            return FCFW::TimelineManager::GetSingleton().GetPlaybackTime(handle, static_cast<size_t>(a_timelineID));
        }

        bool AllowUserRotation(RE::StaticFunctionTag*, RE::BSFixedString a_modName, std::int32_t a_timelineID, bool a_allow) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().AllowUserRotation(handle, static_cast<size_t>(a_timelineID), a_allow);
        }

        bool IsUserRotationAllowed(RE::StaticFunctionTag*, RE::BSFixedString a_modName, std::int32_t a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }
        
            return FCFW::TimelineManager::GetSingleton().IsUserRotationAllowed(handle, static_cast<size_t>(a_timelineID));
        }

        bool SetPlaybackMode(RE::StaticFunctionTag*, RE::BSFixedString a_modName, std::int32_t a_timelineID, std::int32_t a_playbackMode, float a_loopTimeOffset) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            // Validate and convert int to PlaybackMode enum
            if (a_playbackMode < 0 || a_playbackMode > 2) {
                log::error("{}: Invalid playback mode {} for timeline {}", __FUNCTION__, a_playbackMode, a_timelineID);
                return false;
            }
            FCFW::PlaybackMode mode = static_cast<FCFW::PlaybackMode>(a_playbackMode);

            return FCFW::TimelineManager::GetSingleton().SetPlaybackMode(handle, static_cast<size_t>(a_timelineID), mode, a_loopTimeOffset);
        }

        bool AddTimelineFromFile(RE::StaticFunctionTag*, RE::BSFixedString a_modName, std::int32_t a_timelineID, RE::BSFixedString a_filePath, float a_timeOffset) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }
            
            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }
            
            return FCFW::TimelineManager::GetSingleton().AddTimelineFromFile(handle, static_cast<size_t>(a_timelineID), a_filePath.c_str(), a_timeOffset);
        }

        bool ExportTimeline(RE::StaticFunctionTag*, RE::BSFixedString a_modName, std::int32_t a_timelineID, RE::BSFixedString a_filePath) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }            

            return FCFW::TimelineManager::GetSingleton().ExportTimeline(handle, static_cast<size_t>(a_timelineID), a_filePath.c_str());
        }
        
        // Camera utility functions
        float GetCameraPosX(RE::StaticFunctionTag*) {
            RE::NiPoint3 pos = _ts_SKSEFunctions::GetCameraPos();
            return pos.x;
        }
        
        float GetCameraPosY(RE::StaticFunctionTag*) {
            RE::NiPoint3 pos = _ts_SKSEFunctions::GetCameraPos();
            return pos.y;
        }
        
        float GetCameraPosZ(RE::StaticFunctionTag*) {
            RE::NiPoint3 pos = _ts_SKSEFunctions::GetCameraPos();
            return pos.z;
        }
        
        float GetCameraPitch(RE::StaticFunctionTag*) {
            RE::NiPoint3 rot = _ts_SKSEFunctions::GetCameraRotation();
            return rot.x;  // Pitch
        }
        
        float GetCameraYaw(RE::StaticFunctionTag*) {
            RE::NiPoint3 rot = _ts_SKSEFunctions::GetCameraRotation();
            return rot.z;  // Yaw
        }
        
        void RegisterForTimelineEvents(RE::StaticFunctionTag*, RE::TESForm* a_form) {
            if (!a_form) {
                log::error("{}: Null form provided", __FUNCTION__);
                return;
            }
            
            FCFW::TimelineManager::GetSingleton().RegisterForTimelineEvents(a_form);
        }
        
        void UnregisterForTimelineEvents(RE::StaticFunctionTag*, RE::TESForm* a_form) {
            if (!a_form) {
                log::error("{}: Null form provided", __FUNCTION__);
                return;
            }
            
            FCFW::TimelineManager::GetSingleton().UnregisterForTimelineEvents(a_form);
        }

        void ToggleBodyPartRotationMatrixDisplay(RE::StaticFunctionTag*, RE::Actor* a_actor, int a_bodyPart) {
            FCFW::TimelineManager::GetSingleton().ToggleBodyPartRotationMatrixDisplay(a_actor, static_cast<BodyPart>(a_bodyPart));
        }

        bool FCFWFunctions(RE::BSScript::Internal::VirtualMachine * a_vm){
            a_vm->RegisterFunction("ToggleBodyPartRotationMatrixDisplay", "FCFW_SKSEFunctions", ToggleBodyPartRotationMatrixDisplay);
            a_vm->RegisterFunction("GetPluginVersion", "FCFW_SKSEFunctions", GetFCFWPluginVersion);
            a_vm->RegisterFunction("RegisterPlugin", "FCFW_SKSEFunctions", RegisterPlugin);
            a_vm->RegisterFunction("RegisterTimeline", "FCFW_SKSEFunctions", RegisterTimeline);
            a_vm->RegisterFunction("UnregisterTimeline", "FCFW_SKSEFunctions", UnregisterTimeline);
            a_vm->RegisterFunction("AddTranslationPointAtCamera", "FCFW_SKSEFunctions", AddTranslationPointAtCamera);
            a_vm->RegisterFunction("AddTranslationPoint", "FCFW_SKSEFunctions", AddTranslationPoint);
            a_vm->RegisterFunction("AddTranslationPointAtRef", "FCFW_SKSEFunctions", AddTranslationPointAtRef);
            a_vm->RegisterFunction("AddRotationPointAtCamera", "FCFW_SKSEFunctions", AddRotationPointAtCamera);
            a_vm->RegisterFunction("AddRotationPoint", "FCFW_SKSEFunctions", AddRotationPoint);
            a_vm->RegisterFunction("AddRotationPointAtRef", "FCFW_SKSEFunctions", AddRotationPointAtRef);
            a_vm->RegisterFunction("StartRecording", "FCFW_SKSEFunctions", StartRecording);
            a_vm->RegisterFunction("StopRecording", "FCFW_SKSEFunctions", StopRecording);
            a_vm->RegisterFunction("RemoveTranslationPoint", "FCFW_SKSEFunctions", RemoveTranslationPoint);
            a_vm->RegisterFunction("RemoveRotationPoint", "FCFW_SKSEFunctions", RemoveRotationPoint);
            a_vm->RegisterFunction("ClearTimeline", "FCFW_SKSEFunctions", ClearTimeline);
            a_vm->RegisterFunction("GetTranslationPointCount", "FCFW_SKSEFunctions", GetTranslationPointCount);
            a_vm->RegisterFunction("GetRotationPointCount", "FCFW_SKSEFunctions", GetRotationPointCount);
            a_vm->RegisterFunction("GetTranslationPointX", "FCFW_SKSEFunctions", GetTranslationPointX);
            a_vm->RegisterFunction("GetTranslationPointY", "FCFW_SKSEFunctions", GetTranslationPointY);
            a_vm->RegisterFunction("GetTranslationPointZ", "FCFW_SKSEFunctions", GetTranslationPointZ);
            a_vm->RegisterFunction("GetRotationPointPitch", "FCFW_SKSEFunctions", GetRotationPointPitch);
            a_vm->RegisterFunction("GetRotationPointYaw", "FCFW_SKSEFunctions", GetRotationPointYaw);
            a_vm->RegisterFunction("StartPlayback", "FCFW_SKSEFunctions", StartPlayback);
            a_vm->RegisterFunction("StopPlayback", "FCFW_SKSEFunctions", StopPlayback);
            a_vm->RegisterFunction("SwitchPlayback", "FCFW_SKSEFunctions", SwitchPlayback);
            a_vm->RegisterFunction("PausePlayback", "FCFW_SKSEFunctions", PausePlayback);
            a_vm->RegisterFunction("ResumePlayback", "FCFW_SKSEFunctions", ResumePlayback);
            a_vm->RegisterFunction("IsPlaybackPaused", "FCFW_SKSEFunctions", IsPlaybackPaused);
            a_vm->RegisterFunction("IsPlaybackRunning", "FCFW_SKSEFunctions", IsPlaybackRunning);
            a_vm->RegisterFunction("IsRecording", "FCFW_SKSEFunctions", IsRecording);
            a_vm->RegisterFunction("GetActiveTimelineID", "FCFW_SKSEFunctions", GetActiveTimelineID);
            a_vm->RegisterFunction("GetPlaybackTime", "FCFW_SKSEFunctions", GetPlaybackTime);
            a_vm->RegisterFunction("AllowUserRotation", "FCFW_SKSEFunctions", AllowUserRotation);
            a_vm->RegisterFunction("IsUserRotationAllowed", "FCFW_SKSEFunctions", IsUserRotationAllowed);
            a_vm->RegisterFunction("SetPlaybackMode", "FCFW_SKSEFunctions", SetPlaybackMode);
            a_vm->RegisterFunction("AddTimelineFromFile", "FCFW_SKSEFunctions", AddTimelineFromFile);
            a_vm->RegisterFunction("ExportTimeline", "FCFW_SKSEFunctions", ExportTimeline);
            a_vm->RegisterFunction("RegisterForTimelineEvents", "FCFW_SKSEFunctions", RegisterForTimelineEvents);
            a_vm->RegisterFunction("UnregisterForTimelineEvents", "FCFW_SKSEFunctions", UnregisterForTimelineEvents);
            
            // Camera utility functions
            a_vm->RegisterFunction("GetCameraPosX", "FCFW_SKSEFunctions", GetCameraPosX);
            a_vm->RegisterFunction("GetCameraPosY", "FCFW_SKSEFunctions", GetCameraPosY);
            a_vm->RegisterFunction("GetCameraPosZ", "FCFW_SKSEFunctions", GetCameraPosZ);
            a_vm->RegisterFunction("GetCameraPitch", "FCFW_SKSEFunctions", GetCameraPitch);
            a_vm->RegisterFunction("GetCameraYaw", "FCFW_SKSEFunctions", GetCameraYaw);
            
            return true;
        }
    } // namespace Interface
} // namespace FCFW

SKSEPluginInfo(
    .Version = Plugin::VERSION,
    .Name = Plugin::NAME,
    .Author = "Staalo",
    .RuntimeCompatibility = SKSE::PluginDeclaration::RuntimeCompatibility(SKSE::VersionIndependence::AddressLibrary),
    .MinimumSKSEVersion = { 2, 2, 3 } // or 0 if you want to support all
)

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* skse) {
    long logLevel = _ts_SKSEFunctions::GetValueFromINI(nullptr, 0, "LogLevel:Log", "SKSE/Plugins/FreeCameraFramework.ini", 3L);
    bool isLogLevelValid = true;
    if (logLevel < 0 || logLevel > 6) {
        logLevel = 2L; // info
        isLogLevelValid = false;
    }

	_ts_SKSEFunctions::InitializeLogging(static_cast<spdlog::level::level_enum>(logLevel));

    Init(skse);

    if (!isLogLevelValid) {
        log::warn("{}: LogLevel in INI file is invalid. Defaulting to info level.", __FUNCTION__);
    }
    log::info("{}: LogLevel: {}, FCFW Plugin version: {}", __FUNCTION__, logLevel, FCFW::Interface::GetFCFWPluginVersion(nullptr));

    if (!SKSE::GetPapyrusInterface()->Register(FCFW::Interface::FCFWFunctions)) {
        log::warn("{}: Failed to register Papyrus functions.", __FUNCTION__);
        return false;
    } else {
        log::info("{}: Registered Papyrus functions", __FUNCTION__);
    }

    SKSE::AllocTrampoline(64);
        
    log::info("{}: Calling Install Hooks", __FUNCTION__);

    Hooks::Install();

    auto* messaging = SKSE::GetMessagingInterface();
    if (messaging) {
        messaging->RegisterListener([](SKSE::MessagingInterface::Message* msg) {
            switch (msg->type) {
            case SKSE::MessagingInterface::kSaveGame:
                // Save about to start - handle ongoing playback / recording
                FCFW::TimelineManager::GetSingleton().OnPreSaveGame();
                break;
            case SKSE::MessagingInterface::kDataLoaded:
                APIs::RequestAPIs();
                break;
            case SKSE::MessagingInterface::kPostLoad:
                APIs::RequestAPIs();
                break;
            case SKSE::MessagingInterface::kPostPostLoad:
                APIs::RequestAPIs();
                break;
            case SKSE::MessagingInterface::kPreLoadGame:
                break;
            case SKSE::MessagingInterface::kPostLoadGame:
            case SKSE::MessagingInterface::kNewGame:
                APIs::RequestAPIs();
                if (!GetModuleHandleA("po3_Tweaks.dll")) {
                    log::warn("{}: po3_Tweaks.dll not found.", __FUNCTION__);
                    RE::DebugMessageBox("powerofthree's Tweaks not installed? FCFW bodypart support will not function properly.");
                }                
                break;
            }
        });
    } else {
        log::warn("{}: Failed to get messaging interface for save handler", __FUNCTION__);
    }

    return true;
}

extern "C" DLLEXPORT void* SKSEAPI RequestPluginAPI(const FCFW_API::InterfaceVersion a_interfaceVersion)
{
	auto api = Messaging::FCFWInterface::GetSingleton();

	log::info("{} called, InterfaceVersion {}", __FUNCTION__, static_cast<uint8_t>(a_interfaceVersion));

	switch (a_interfaceVersion) {
	case FCFW_API::InterfaceVersion::V1:
		log::info("{} returned the API singleton", __FUNCTION__);
		return static_cast<void*>(api);
	}

	log::info("{} requested the wrong interface version", __FUNCTION__);
	return nullptr;
}
