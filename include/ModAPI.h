#pragma once
#include "FCFW_API.h"

namespace Messaging
{
	using InterfaceVersion1 = ::FCFW_API::IVFCFW1;

	class FCFWInterface : public InterfaceVersion1
	{
	private:
		FCFWInterface() noexcept;
		virtual ~FCFWInterface() noexcept;

	public:
		static FCFWInterface* GetSingleton() noexcept
		{
			static FCFWInterface singleton;
			return std::addressof(singleton);
		}

		// InterfaceVersion1
        virtual unsigned long GetFCFWThreadId(void) const noexcept override;
		virtual int GetFCFWPluginVersion() const noexcept override;
		virtual bool RegisterPlugin(SKSE::PluginHandle a_pluginHandle) const noexcept override;
		virtual size_t RegisterTimeline(SKSE::PluginHandle a_pluginHandle) const noexcept override;
		virtual bool UnregisterTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
        virtual int AddTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, const RE::NiPoint3& a_position, bool a_easeIn = false, bool a_easeOut = false, FCFW_API::InterpolationMode a_interpolationMode = FCFW_API::InterpolationMode::kCubicHermite) const noexcept override;
        virtual int AddTranslationPointAtRef(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, RE::TESObjectREFR* a_reference, FCFW_API::BodyPart a_bodyPart = FCFW_API::BodyPart::kNone, const RE::NiPoint3& a_offset = RE::NiPoint3(), bool a_isOffsetRelative = false, bool a_easeIn = false, bool a_easeOut = false, FCFW_API::InterpolationMode a_interpolationMode = FCFW_API::InterpolationMode::kCubicHermite) const noexcept override;
		virtual int AddTranslationPointAtCamera(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, bool a_easeIn = false, bool a_easeOut = false, FCFW_API::InterpolationMode a_interpolationMode = FCFW_API::InterpolationMode::kCubicHermite) const noexcept override;
		virtual int AddRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, const RE::NiPoint3& a_rotation, bool a_easeIn = false, bool a_easeOut = false, FCFW_API::InterpolationMode a_interpolationMode = FCFW_API::InterpolationMode::kCubicHermite) const noexcept override;
		virtual int AddRotationPointAtRef(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, RE::TESObjectREFR* a_reference, FCFW_API::BodyPart a_bodyPart = FCFW_API::BodyPart::kNone, const RE::NiPoint3& a_offset = RE::NiPoint3(), bool a_isOffsetRelative = false, bool a_easeIn = false, bool a_easeOut = false, FCFW_API::InterpolationMode a_interpolationMode = FCFW_API::InterpolationMode::kCubicHermite) const noexcept override;
		virtual int AddRotationPointAtCamera(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, bool a_easeIn = false, bool a_easeOut = false, FCFW_API::InterpolationMode a_interpolationMode = FCFW_API::InterpolationMode::kCubicHermite) const noexcept override;
		virtual int AddFOVPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_time, float a_fov, bool a_easeIn = false, bool a_easeOut = false, FCFW_API::InterpolationMode a_interpolationMode = FCFW_API::InterpolationMode::kCubicHermite) const noexcept override;
		virtual bool RemoveTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const noexcept override;
		virtual bool StartRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_recordingInterval = 1.0f, bool a_append = false, float a_timeOffset = 0.0f) const noexcept override;
		virtual bool StopRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual bool RemoveRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const noexcept override;
		virtual bool RemoveFOVPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const noexcept override;
		virtual bool ClearTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual int GetTranslationPointCount(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual int GetRotationPointCount(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual int GetFOVPointCount(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual RE::NiPoint3 GetTranslationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const noexcept override;
		virtual RE::NiPoint3 GetRotationPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const noexcept override;
		virtual float GetFOVPoint(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, size_t a_index) const noexcept override;
		virtual bool StartPlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, float a_speed = 1.0f, bool a_globalEaseIn = false, bool a_globalEaseOut = false, bool a_useDuration = false, float a_duration = 0.0f, float a_startTime = 0.0f) const noexcept override;
		virtual bool StopPlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual bool SwitchPlayback(SKSE::PluginHandle a_pluginHandle, size_t a_fromTimelineID, size_t a_toTimelineID) const noexcept override;
		virtual bool PausePlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual bool ResumePlayback(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual bool IsPlaybackRunning(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual bool IsRecording(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual bool IsPlaybackPaused(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual float GetPlaybackTime(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual size_t GetActiveTimelineID() const noexcept override;
		virtual void AllowUserRotation(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, bool a_allow) const noexcept override;
		virtual bool IsUserRotationAllowed(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual bool SetFollowGround(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, bool a_follow, float a_minHeight = 0.0f) const noexcept override;
		virtual bool IsGroundFollowingEnabled(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual float GetMinHeightAboveGround(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual bool SetMenuVisibility(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, bool a_show) const noexcept override;
		virtual bool AreMenusVisible(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID) const noexcept override;
		virtual bool SetPlaybackMode(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, FCFW_API::PlaybackMode a_playbackMode, float a_loopTimeOffset = 0.0f) const noexcept override;
		virtual bool AddTimelineFromFile(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, const char* a_filePath, float a_timeOffset = 0.0f) const noexcept override;
		virtual bool ExportTimeline(SKSE::PluginHandle a_pluginHandle, size_t a_timelineID, const char* a_filePath) const noexcept override;

	private:
		unsigned long apiTID = 0;
	};
}
