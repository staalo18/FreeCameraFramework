#include "Hooks.h"
#include "TimelineManager.h"
#include "ModAPI.h"
#include "CameraTypes.h"

namespace FCFW {
    namespace Interface {
        int GetFCFWPluginVersion(RE::StaticFunctionTag*) {
            // Encode version as: major * 10000 + minor * 100 + patch
            return static_cast<int>(Plugin::VERSION[0]) * 10000 + 
                   static_cast<int>(Plugin::VERSION[1]) * 100 + 
                   static_cast<int>(Plugin::VERSION[2]);
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
                return false;
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
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return -1;
            }

            RE::NiPoint3 position(a_posX, a_posY, a_posZ);
            return FCFW::TimelineManager::GetSingleton().AddTranslationPoint(handle, static_cast<size_t>(a_timelineID), a_time, position, a_easeIn, a_easeOut, ToInterpolationMode(a_interpolationMode));
        }

        int AddTranslationPointAtRef(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, float a_time, RE::TESObjectREFR* a_reference, float a_offsetX, float a_offsetY, float a_offsetZ, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, int a_interpolationMode) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return -1;
            }

            RE::NiPoint3 offset(a_offsetX, a_offsetY, a_offsetZ);
            return FCFW::TimelineManager::GetSingleton().AddTranslationPointAtRef(handle, static_cast<size_t>(a_timelineID), a_time, a_reference, offset, a_isOffsetRelative, a_easeIn, a_easeOut, ToInterpolationMode(a_interpolationMode));
        }

        int AddRotationPointAtCamera(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, float a_time, bool a_easeIn, bool a_easeOut, int a_interpolationMode) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return -1;
            }

            return FCFW::TimelineManager::GetSingleton().AddRotationPointAtCamera(static_cast<size_t>(a_timelineID), handle, a_time, a_easeIn, a_easeOut, ToInterpolationMode(a_interpolationMode));
        }

        int AddRotationPoint(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, float a_time, float a_pitch, float a_yaw, bool a_easeIn, bool a_easeOut, int a_interpolationMode) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return -1;
            }

            RE::BSTPoint2<float> rotation{a_pitch, a_yaw};
            return FCFW::TimelineManager::GetSingleton().AddRotationPoint(handle, static_cast<size_t>(a_timelineID), a_time, rotation, a_easeIn, a_easeOut, ToInterpolationMode(a_interpolationMode));
        }

        int AddRotationPointAtRef(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, float a_time, RE::TESObjectREFR* a_reference, float a_offsetPitch, float a_offsetYaw, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, int a_interpolationMode) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return -1;
            }

            RE::BSTPoint2<float> offset{a_offsetPitch, a_offsetYaw};
            return FCFW::TimelineManager::GetSingleton().AddRotationPointAtRef(handle, static_cast<size_t>(a_timelineID), a_time, a_reference, offset, a_isOffsetRelative, a_easeIn, a_easeOut, ToInterpolationMode(a_interpolationMode));
        }

        bool StartRecording(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().StartRecording(handle, static_cast<size_t>(a_timelineID));
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

        bool ClearTimeline(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, bool a_notifyUser) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
                if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().ClearTimeline(handle, static_cast<size_t>(a_timelineID), a_notifyUser);
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

        bool StartPlayback(RE::StaticFunctionTag*, RE::BSFixedString a_modName, int a_timelineID, float a_speed, bool a_globalEaseIn, bool a_globalEaseOut, bool a_useDuration, float a_duration) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().StartPlayback(handle, static_cast<size_t>(a_timelineID), a_speed, a_globalEaseIn, a_globalEaseOut, a_useDuration, a_duration);
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

        bool SetPlaybackMode(RE::StaticFunctionTag*, RE::BSFixedString a_modName, std::int32_t a_timelineID, std::int32_t a_playbackMode) {
            if (a_modName.empty() || a_timelineID <= 0) {
                return false;
            }

            SKSE::PluginHandle handle = FCFW::ModNameToHandle(a_modName.c_str());
            if (handle == 0) {
                log::error("{}: Invalid mod name '{}' - mod not loaded or doesn't exist", __FUNCTION__, a_modName.c_str());
                return false;
            }

            return FCFW::TimelineManager::GetSingleton().SetPlaybackMode(handle, static_cast<size_t>(a_timelineID), a_playbackMode);
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

        bool FCFWFunctions(RE::BSScript::Internal::VirtualMachine * a_vm){
            a_vm->RegisterFunction("FCFW_GetPluginVersion", "FCFW_SKSEFunctions", GetFCFWPluginVersion);
            a_vm->RegisterFunction("FCFW_RegisterTimeline", "FCFW_SKSEFunctions", RegisterTimeline);
            a_vm->RegisterFunction("FCFW_UnregisterTimeline", "FCFW_SKSEFunctions", UnregisterTimeline);
            a_vm->RegisterFunction("FCFW_AddTranslationPointAtCamera", "FCFW_SKSEFunctions", AddTranslationPointAtCamera);
            a_vm->RegisterFunction("FCFW_AddTranslationPoint", "FCFW_SKSEFunctions", AddTranslationPoint);
            a_vm->RegisterFunction("FCFW_AddTranslationPointAtRef", "FCFW_SKSEFunctions", AddTranslationPointAtRef);
            a_vm->RegisterFunction("FCFW_AddRotationPointAtCamera", "FCFW_SKSEFunctions", AddRotationPointAtCamera);
            a_vm->RegisterFunction("FCFW_AddRotationPoint", "FCFW_SKSEFunctions", AddRotationPoint);
            a_vm->RegisterFunction("FCFW_AddRotationPointAtRef", "FCFW_SKSEFunctions", AddRotationPointAtRef);
            a_vm->RegisterFunction("FCFW_StartRecording", "FCFW_SKSEFunctions", StartRecording);
            a_vm->RegisterFunction("FCFW_StopRecording", "FCFW_SKSEFunctions", StopRecording);
            a_vm->RegisterFunction("FCFW_RemoveTranslationPoint", "FCFW_SKSEFunctions", RemoveTranslationPoint);
            a_vm->RegisterFunction("FCFW_RemoveRotationPoint", "FCFW_SKSEFunctions", RemoveRotationPoint);
            a_vm->RegisterFunction("FCFW_ClearTimeline", "FCFW_SKSEFunctions", ClearTimeline);
            a_vm->RegisterFunction("FCFW_GetTranslationPointCount", "FCFW_SKSEFunctions", GetTranslationPointCount);
            a_vm->RegisterFunction("FCFW_GetRotationPointCount", "FCFW_SKSEFunctions", GetRotationPointCount);
            a_vm->RegisterFunction("FCFW_GetTranslationPointX", "FCFW_SKSEFunctions", GetTranslationPointX);
            a_vm->RegisterFunction("FCFW_GetTranslationPointY", "FCFW_SKSEFunctions", GetTranslationPointY);
            a_vm->RegisterFunction("FCFW_GetTranslationPointZ", "FCFW_SKSEFunctions", GetTranslationPointZ);
            a_vm->RegisterFunction("FCFW_GetRotationPointPitch", "FCFW_SKSEFunctions", GetRotationPointPitch);
            a_vm->RegisterFunction("FCFW_GetRotationPointYaw", "FCFW_SKSEFunctions", GetRotationPointYaw);
            a_vm->RegisterFunction("FCFW_StartPlayback", "FCFW_SKSEFunctions", StartPlayback);
            a_vm->RegisterFunction("FCFW_StopPlayback", "FCFW_SKSEFunctions", StopPlayback);
            a_vm->RegisterFunction("FCFW_SwitchPlayback", "FCFW_SKSEFunctions", SwitchPlayback);
            a_vm->RegisterFunction("FCFW_PausePlayback", "FCFW_SKSEFunctions", PausePlayback);
            a_vm->RegisterFunction("FCFW_ResumePlayback", "FCFW_SKSEFunctions", ResumePlayback);
            a_vm->RegisterFunction("FCFW_IsPlaybackPaused", "FCFW_SKSEFunctions", IsPlaybackPaused);
            a_vm->RegisterFunction("FCFW_IsPlaybackRunning", "FCFW_SKSEFunctions", IsPlaybackRunning);
            a_vm->RegisterFunction("FCFW_IsRecording", "FCFW_SKSEFunctions", IsRecording);
            a_vm->RegisterFunction("FCFW_GetActiveTimelineID", "FCFW_SKSEFunctions", GetActiveTimelineID);
            a_vm->RegisterFunction("FCFW_AllowUserRotation", "FCFW_SKSEFunctions", AllowUserRotation);
            a_vm->RegisterFunction("FCFW_IsUserRotationAllowed", "FCFW_SKSEFunctions", IsUserRotationAllowed);
            a_vm->RegisterFunction("FCFW_SetPlaybackMode", "FCFW_SKSEFunctions", SetPlaybackMode);
            a_vm->RegisterFunction("FCFW_AddTimelineFromFile", "FCFW_SKSEFunctions", AddTimelineFromFile);
            a_vm->RegisterFunction("FCFW_ExportTimeline", "FCFW_SKSEFunctions", ExportTimeline);
            a_vm->RegisterFunction("FCFW_RegisterForTimelineEvents", "FCFW_SKSEFunctions", RegisterForTimelineEvents);
            a_vm->RegisterFunction("FCFW_UnregisterForTimelineEvents", "FCFW_SKSEFunctions", UnregisterForTimelineEvents);
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
