#include "Timeline.h"

namespace FCFW
{
	size_t Timeline::AddTranslationPoint(const TranslationPoint& a_point)
	{
		m_translationTrack.AddPoint(a_point);
		return m_translationTrack.GetPointCount();
	}

	size_t Timeline::AddRotationPoint(const RotationPoint& a_point)
	{
		m_rotationTrack.AddPoint(a_point);
		return m_rotationTrack.GetPointCount();
	}

	size_t Timeline::AddFOVPoint(const FOVPoint& a_point)
	{
		m_fovTrack.AddPoint(a_point);
		return m_fovTrack.GetPointCount();
	}

	void Timeline::RemoveTranslationPoint(size_t a_index)
	{
		m_translationTrack.RemovePoint(a_index);
	}

	void Timeline::RemoveRotationPoint(size_t a_index)
	{
		m_rotationTrack.RemovePoint(a_index);
	}

	void Timeline::RemoveFOVPoint(size_t a_index)
	{
		m_fovTrack.RemovePoint(a_index);
	}

	void Timeline::UpdatePlayback(float a_deltaTime)
	{
		m_translationTrack.UpdateTimeline(a_deltaTime);
		m_rotationTrack.UpdateTimeline(a_deltaTime);
		m_fovTrack.UpdateTimeline(a_deltaTime);
	}

	void Timeline::StartPlayback()
	{
		m_translationTrack.StartPlayback();
		m_rotationTrack.StartPlayback();
		m_fovTrack.StartPlayback();
	}

	void Timeline::ResetPlayback()
	{
		m_translationTrack.ResetTimeline();
		m_rotationTrack.ResetTimeline();
		m_fovTrack.ResetTimeline();
	}

	void Timeline::PausePlayback()
	{
		m_translationTrack.PausePlayback();
		m_rotationTrack.PausePlayback();
		m_fovTrack.PausePlayback();
	}

	void Timeline::ResumePlayback()
	{
		m_translationTrack.ResumePlayback();
		m_rotationTrack.ResumePlayback();
		m_fovTrack.ResumePlayback();
	}

	RE::NiPoint3 Timeline::GetTranslation(float a_time) const
	{
		return m_translationTrack.GetPointAtTime(a_time);
	}

	RE::NiPoint3 Timeline::GetRotation(float a_time) const
	{
		return m_rotationTrack.GetPointAtTime(a_time);
	}

	float Timeline::GetFOV(float a_time) const
	{
		return m_fovTrack.GetPointAtTime(a_time);
	}

	size_t Timeline::GetTranslationPointCount() const
	{
		return m_translationTrack.GetPointCount();
	}

	size_t Timeline::GetRotationPointCount() const
	{
		return m_rotationTrack.GetPointCount();
	}

	size_t Timeline::GetFOVPointCount() const
	{
		return m_fovTrack.GetPointCount();
	}

	float Timeline::GetDuration() const
	{
		return std::max({
			m_translationTrack.GetDuration(),
			m_rotationTrack.GetDuration(),
			m_fovTrack.GetDuration()
		});
	}

	void Timeline::SetPlaybackMode(PlaybackMode a_mode)
	{
		m_translationTrack.SetPlaybackMode(a_mode);
		m_rotationTrack.SetPlaybackMode(a_mode);
		m_fovTrack.SetPlaybackMode(a_mode);
	}

	void Timeline::SetLoopTimeOffset(float a_offset)
	{
		m_translationTrack.SetLoopTimeOffset(a_offset);
		m_rotationTrack.SetLoopTimeOffset(a_offset);
		m_fovTrack.SetLoopTimeOffset(a_offset);
	}

	float Timeline::GetPlaybackTime() const
	{
		return m_translationTrack.GetPlaybackTime();
	}

	void Timeline::SetPlaybackTime(float a_time)
	{
		m_translationTrack.SetPlaybackTime(a_time);
		m_rotationTrack.SetPlaybackTime(a_time);
		m_fovTrack.SetPlaybackTime(a_time);
	}

	bool Timeline::IsPlaying() const
	{
		return m_translationTrack.IsPlaying() || m_rotationTrack.IsPlaying() || m_fovTrack.IsPlaying();
	}

	bool Timeline::IsPaused() const
	{
		return m_translationTrack.IsPaused() || m_rotationTrack.IsPaused() || m_fovTrack.IsPaused();
	}

	void Timeline::ClearPoints()
	{
		m_translationTrack.ClearPoints();
		m_rotationTrack.ClearPoints();
		m_fovTrack.ClearPoints();
	}

	void Timeline::Reset()
	{
		ClearPoints();
		SetPlaybackMode(PlaybackMode::kEnd);
		SetLoopTimeOffset(0.0f);
	}

	PlaybackMode Timeline::GetPlaybackMode() const
	{
		return m_translationTrack.GetPlaybackMode();
	}

	float Timeline::GetLoopTimeOffset() const
	{
		return m_translationTrack.GetLoopTimeOffset();
	}

	TranslationPoint Timeline::GetTranslationPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const
	{
		return m_translationTrack.GetPointAtCamera(a_time, a_easeIn, a_easeOut);
	}

	RotationPoint Timeline::GetRotationPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const
	{
		return m_rotationTrack.GetPointAtCamera(a_time, a_easeIn, a_easeOut);
	}

	// YAML import/export wrappers
	bool Timeline::AddTranslationPathFromFile(const std::string& a_filePath, float a_timeOffset)
	{
		return m_translationTrack.AddPathFromFile(a_filePath, a_timeOffset);
	}

	bool Timeline::AddRotationPathFromFile(const std::string& a_filePath, float a_timeOffset, float a_conversionFactor)
	{
		return m_rotationTrack.AddPathFromFile(a_filePath, a_timeOffset, a_conversionFactor);
	}

	bool Timeline::AddFOVPathFromFile(const std::string& a_filePath, float a_timeOffset)
	{
		return m_fovTrack.AddPathFromFile(a_filePath, a_timeOffset);
	}

	bool Timeline::ExportTranslationPath(std::ofstream& a_file) const
	{
		return m_translationTrack.ExportPath(a_file);
	}

	bool Timeline::ExportRotationPath(std::ofstream& a_file, float a_conversionFactor) const
	{
		return m_rotationTrack.ExportPath(a_file, a_conversionFactor);
	}

	bool Timeline::ExportFOVPath(std::ofstream& a_file) const
	{
		return m_fovTrack.ExportPath(a_file);
	}

	RE::NiPoint3 Timeline::GetTranslationPoint(size_t a_index) const
	{
		return m_translationTrack.GetPoint(a_index).m_point;
	}

	RE::NiPoint3 Timeline::GetRotationPoint(size_t a_index) const
	{
		return m_rotationTrack.GetPoint(a_index).m_point;
	}

	float Timeline::GetFOVPoint(size_t a_index) const
	{
		return m_fovTrack.GetPoint(a_index).m_point;
	}

}  // namespace FCFW
