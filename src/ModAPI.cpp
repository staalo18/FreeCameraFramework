#include "ModAPI.h"
#include "TimelineManager.h"
#include "CameraPath.h"
#include "CameraTypes.h"

Messaging::FCFWInterface::FCFWInterface() noexcept {
	apiTID = GetCurrentThreadId();
}

Messaging::FCFWInterface::~FCFWInterface() noexcept {}

unsigned long Messaging::FCFWInterface::GetFCFWThreadId() const noexcept {
	return apiTID;
}

int Messaging::FCFWInterface::GetFCFWPluginVersion() const noexcept {
	// Encode version as: major * 10000 + minor * 100 + patch
	return static_cast<int>(Plugin::VERSION[0]) * 10000 + 
	       static_cast<int>(Plugin::VERSION[1]) * 100 + 
	       static_cast<int>(Plugin::VERSION[2]);
}

size_t Messaging::FCFWInterface::RegisterTimeline(SKSE::PluginHandle a_pluginHandle) const noexcept {
	size_t result = FCFW::TimelineManager::GetSingleton().RegisterTimeline(a_pluginHandle);
	log::info("{}: API wrapper returning timeline ID {}", __FUNCTION__, result);
	return result;
}

bool Messaging::FCFWInterface::UnregisterTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept {
	return FCFW::TimelineManager::GetSingleton().UnregisterTimeline(a_pluginHandle, a_timelineID);
}

int Messaging::FCFWInterface::AddTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, const RE::NiPoint3& a_position, bool a_easeIn, bool a_easeOut, int a_interpolationMode) const noexcept {
	return FCFW::TimelineManager::GetSingleton().AddTranslationPoint(a_pluginHandle, a_timelineID, a_time, a_position, a_easeIn, a_easeOut, FCFW::ToInterpolationMode(a_interpolationMode));
}

int Messaging::FCFWInterface::AddTranslationPointAtRef(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, RE::TESObjectREFR* a_reference, const RE::NiPoint3& a_offset, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, int a_interpolationMode) const noexcept {
	return FCFW::TimelineManager::GetSingleton().AddTranslationPointAtRef(a_pluginHandle, a_timelineID, a_time, a_reference, a_offset, a_isOffsetRelative, a_easeIn, a_easeOut, FCFW::ToInterpolationMode(a_interpolationMode));
}

int Messaging::FCFWInterface::AddTranslationPointAtCamera(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, bool a_easeIn, bool a_easeOut, int a_interpolationMode) const noexcept {
	return FCFW::TimelineManager::GetSingleton().AddTranslationPointAtCamera(a_pluginHandle, a_timelineID, a_time, a_easeIn, a_easeOut, FCFW::ToInterpolationMode(a_interpolationMode));
}

int Messaging::FCFWInterface::AddRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, const RE::BSTPoint2<float>& a_rotation, bool a_easeIn, bool a_easeOut, int a_interpolationMode) const noexcept {
	return FCFW::TimelineManager::GetSingleton().AddRotationPoint(a_pluginHandle, a_timelineID, a_time, a_rotation, a_easeIn, a_easeOut, FCFW::ToInterpolationMode(a_interpolationMode));
}

int Messaging::FCFWInterface::AddRotationPointAtRef(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, RE::TESObjectREFR* a_reference, const RE::BSTPoint2<float>& a_offset, bool a_isOffsetRelative, bool a_easeIn, bool a_easeOut, int a_interpolationMode) const noexcept {
	return FCFW::TimelineManager::GetSingleton().AddRotationPointAtRef(a_pluginHandle, a_timelineID, a_time, a_reference, a_offset, a_isOffsetRelative, a_easeIn, a_easeOut, FCFW::ToInterpolationMode(a_interpolationMode));
}

int Messaging::FCFWInterface::AddRotationPointAtCamera(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, bool a_easeIn, bool a_easeOut, int a_interpolationMode) const noexcept {
    return FCFW::TimelineManager::GetSingleton().AddRotationPointAtCamera(a_pluginHandle, a_timelineID, a_time, a_easeIn, a_easeOut, FCFW::ToInterpolationMode(a_interpolationMode));
}

bool Messaging::FCFWInterface::StartRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_recordingInterval, bool a_append, float a_timeOffset) const noexcept {
    return FCFW::TimelineManager::GetSingleton().StartRecording(a_pluginHandle, a_timelineID, a_recordingInterval, a_append, a_timeOffset);
}

bool Messaging::FCFWInterface::StopRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept {
    return FCFW::TimelineManager::GetSingleton().StopRecording(a_pluginHandle, a_timelineID);
}

bool Messaging::FCFWInterface::RemoveTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const noexcept {
	return FCFW::TimelineManager::GetSingleton().RemoveTranslationPoint(a_pluginHandle, a_timelineID, a_index);
}

bool Messaging::FCFWInterface::RemoveRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const noexcept {
	return FCFW::TimelineManager::GetSingleton().RemoveRotationPoint(a_pluginHandle, a_timelineID, a_index);
}

bool Messaging::FCFWInterface::ClearTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept {
	return FCFW::TimelineManager::GetSingleton().ClearTimeline(a_pluginHandle, a_timelineID);
}

int Messaging::FCFWInterface::GetTranslationPointCount(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept {
    return FCFW::TimelineManager::GetSingleton().GetTranslationPointCount(a_pluginHandle, a_timelineID);
}

int Messaging::FCFWInterface::GetRotationPointCount(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept {
	return FCFW::TimelineManager::GetSingleton().GetRotationPointCount(a_pluginHandle, a_timelineID);
}

RE::NiPoint3 Messaging::FCFWInterface::GetTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const noexcept {
	return FCFW::TimelineManager::GetSingleton().GetTranslationPoint(a_pluginHandle, a_timelineID, a_index);
}

RE::BSTPoint2<float> Messaging::FCFWInterface::GetRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const noexcept {
	return FCFW::TimelineManager::GetSingleton().GetRotationPoint(a_pluginHandle, a_timelineID, a_index);
}

bool Messaging::FCFWInterface::StartPlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_speed, bool a_globalEaseIn, bool a_globalEaseOut, bool a_useDuration, float a_duration) const noexcept {
	return FCFW::TimelineManager::GetSingleton().StartPlayback(a_pluginHandle, a_timelineID, a_speed, a_globalEaseIn, a_globalEaseOut, a_useDuration, a_duration);
}

bool Messaging::FCFWInterface::StopPlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept {
	return FCFW::TimelineManager::GetSingleton().StopPlayback(a_pluginHandle, a_timelineID);
}

bool Messaging::FCFWInterface::SwitchPlayback(SKSE::PluginHandle a_pluginHandle, size_t a_fromTimelineID, size_t a_toTimelineID) const noexcept {
	return FCFW::TimelineManager::GetSingleton().SwitchPlayback(a_pluginHandle, a_fromTimelineID, a_toTimelineID);
}

bool Messaging::FCFWInterface::PausePlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept {
	return FCFW::TimelineManager::GetSingleton().PausePlayback(a_pluginHandle, a_timelineID);
}

bool Messaging::FCFWInterface::ResumePlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept {
	return FCFW::TimelineManager::GetSingleton().ResumePlayback(a_pluginHandle, a_timelineID);
}

bool Messaging::FCFWInterface::IsPlaybackRunning(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept {
	return FCFW::TimelineManager::GetSingleton().IsPlaybackRunning(a_pluginHandle, a_timelineID);
}

bool Messaging::FCFWInterface::IsPlaybackPaused(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept {
    return FCFW::TimelineManager::GetSingleton().IsPlaybackPaused(a_pluginHandle, a_timelineID);
}

bool Messaging::FCFWInterface::IsRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept {
    return FCFW::TimelineManager::GetSingleton().IsRecording(a_pluginHandle, a_timelineID);
}

size_t Messaging::FCFWInterface::GetActiveTimelineID() const noexcept {
    return FCFW::TimelineManager::GetSingleton().GetActiveTimelineID();
}

void Messaging::FCFWInterface::AllowUserRotation(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, bool a_allow) const noexcept {
    FCFW::TimelineManager::GetSingleton().AllowUserRotation(a_pluginHandle, a_timelineID, a_allow);
}

bool Messaging::FCFWInterface::IsUserRotationAllowed(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept {
	return FCFW::TimelineManager::GetSingleton().IsUserRotationAllowed(a_pluginHandle, a_timelineID);
}

bool Messaging::FCFWInterface::SetPlaybackMode(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, int a_playbackMode, float a_loopTimeOffset) const noexcept {
	return FCFW::TimelineManager::GetSingleton().SetPlaybackMode(a_pluginHandle, a_timelineID, a_playbackMode, a_loopTimeOffset);
}bool Messaging::FCFWInterface::AddTimelineFromFile(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, const char* a_filePath, float a_timeOffset) const noexcept {
    return FCFW::TimelineManager::GetSingleton().AddTimelineFromFile(a_pluginHandle, a_timelineID, a_filePath, a_timeOffset);
}

bool Messaging::FCFWInterface::ExportTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, const char* a_filePath) const noexcept {
    return FCFW::TimelineManager::GetSingleton().ExportTimeline(a_pluginHandle, a_timelineID, a_filePath);
}

