#pragma once

#include "TimelineTrack.h"

namespace FCFW
{	
	class Timeline
	{
	public:
		Timeline() = default;
		~Timeline() = default;

		size_t AddTranslationPoint(const TranslationPoint& a_point);
		size_t AddRotationPoint(const RotationPoint& a_point);
		void RemoveTranslationPoint(size_t a_index);
		void RemoveRotationPoint(size_t a_index);

		void UpdatePlayback(float a_deltaTime);
		void StartPlayback();
		void ResetPlayback();
		void PausePlayback();
		void ResumePlayback();

		RE::NiPoint3 GetTranslation(float a_time) const;
		RE::BSTPoint2<float> GetRotation(float a_time) const;

		size_t GetTranslationPointCount() const;
		size_t GetRotationPointCount() const;
		float GetDuration() const;
		float GetPlaybackTime() const;
		bool IsPlaying() const;
		bool IsPaused() const;

		void ClearPoints();

		void SetPlaybackMode(PlaybackMode a_mode);
		void SetLoopTimeOffset(float a_offset);
		PlaybackMode GetPlaybackMode() const;
		float GetLoopTimeOffset() const;

		TranslationPoint GetTranslationPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const;
		RotationPoint GetRotationPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const;
		bool AddTranslationPathFromFile(std::ifstream& a_file, float a_timeOffset = 0.0f, float a_conversionFactor = 1.0f);
		bool AddRotationPathFromFile(std::ifstream& a_file, float a_timeOffset = 0.0f, float a_conversionFactor = 1.0f);
		bool ExportTranslationPath(std::ofstream& a_file, float a_conversionFactor = 1.0f) const;
		bool ExportRotationPath(std::ofstream& a_file, float a_conversionFactor = 1.0f) const;

		RE::NiPoint3 GetTranslationPoint(size_t a_index) const;
		RE::BSTPoint2<float> GetRotationPoint(size_t a_index) const;		uint32_t GetTimelineID() const { return m_timelineID; }
		void SetTimelineID(uint32_t a_id) { m_timelineID = a_id; }

		float GetPlaybackSpeed() const { return m_playbackSpeed; }
		void SetPlaybackSpeed(float a_speed) { m_playbackSpeed = a_speed; }

		bool GetGlobalEaseIn() const { return m_globalEaseIn; }
		void SetGlobalEaseIn(bool a_enable) { m_globalEaseIn = a_enable; }

		bool GetGlobalEaseOut() const { return m_globalEaseOut; }
		void SetGlobalEaseOut(bool a_enable) { m_globalEaseOut = a_enable; }

	private:
		TranslationTrack& GetTranslationTrack() { return m_translationTrack; }
		RotationTrack& GetRotationTrack() { return m_rotationTrack; }
		const TranslationTrack& GetTranslationTrack() const { return m_translationTrack; }
		const RotationTrack& GetRotationTrack() const { return m_rotationTrack; }

		TranslationTrack m_translationTrack;  // Position keyframes
		RotationTrack m_rotationTrack;        // Rotation keyframes

		uint32_t m_timelineID{ 0 };           // Unique identifier
		float m_playbackSpeed{ 1.0f };        // Time multiplier
		bool m_globalEaseIn{ false };         // Apply easing to entire timeline
		bool m_globalEaseOut{ false };        // Apply easing to entire timeline
	};

}  // namespace FCFW
