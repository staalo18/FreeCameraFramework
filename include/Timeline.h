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
	size_t AddFOVPoint(const FOVPoint& a_point);
	void RemoveTranslationPoint(size_t a_index);
	void RemoveRotationPoint(size_t a_index);
	void RemoveFOVPoint(size_t a_index);		void UpdatePlayback(float a_deltaTime);
		void StartPlayback();
		void ResetPlayback();
		void PausePlayback();
		void ResumePlayback();

	RE::NiPoint3 GetTranslation(float a_time) const;
	RE::BSTPoint2<float> GetRotation(float a_time) const;
	float GetFOV(float a_time) const;

	size_t GetTranslationPointCount() const;
	size_t GetRotationPointCount() const;
	size_t GetFOVPointCount() const;
		float GetDuration() const;
		float GetPlaybackTime() const;
		void SetPlaybackTime(float a_time);
		bool IsPlaying() const;
		bool IsPaused() const;

		void ClearPoints();
		void Reset();

		void SetPlaybackMode(PlaybackMode a_mode);
		void SetLoopTimeOffset(float a_offset);
		PlaybackMode GetPlaybackMode() const;
		float GetLoopTimeOffset() const;

		TranslationPoint GetTranslationPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const;
		RotationPoint GetRotationPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const;
		
	bool AddTranslationPathFromFile(const std::string& a_filePath, float a_timeOffset = 0.0f);
	bool AddRotationPathFromFile(const std::string& a_filePath, float a_timeOffset = 0.0f, float a_conversionFactor = 1.0f);
	bool AddFOVPathFromFile(const std::string& a_filePath, float a_timeOffset = 0.0f);
	bool ExportTranslationPath(std::ofstream& a_file) const;
	bool ExportRotationPath(std::ofstream& a_file, float a_conversionFactor = 1.0f) const;
	bool ExportFOVPath(std::ofstream& a_file) const;

	RE::NiPoint3 GetTranslationPoint(size_t a_index) const;
	RE::BSTPoint2<float> GetRotationPoint(size_t a_index) const;
	float GetFOVPoint(size_t a_index) const;private:
	TranslationTrack m_translationTrack;  // Position keyframes
	RotationTrack m_rotationTrack;        // Rotation keyframes
	FOVTrack m_fovTrack;                  // FOV keyframes
};}  // namespace FCFW