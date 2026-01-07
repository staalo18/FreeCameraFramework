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

	void Timeline::RemoveTranslationPoint(size_t a_index)
	{
		m_translationTrack.RemovePoint(a_index);
	}

	void Timeline::RemoveRotationPoint(size_t a_index)
	{
		m_rotationTrack.RemovePoint(a_index);
	}

	void Timeline::UpdatePlayback(float a_deltaTime)
	{
		m_translationTrack.UpdateTimeline(a_deltaTime);
		m_rotationTrack.UpdateTimeline(a_deltaTime);
	}

	void Timeline::StartPlayback()
	{
		m_translationTrack.StartPlayback();
		m_rotationTrack.StartPlayback();
	}

	void Timeline::ResetPlayback()
	{
		m_translationTrack.ResetTimeline();
		m_rotationTrack.ResetTimeline();
	}

	void Timeline::PausePlayback()
	{
		m_translationTrack.PausePlayback();
		m_rotationTrack.PausePlayback();
	}

	void Timeline::ResumePlayback()
	{
		m_translationTrack.ResumePlayback();
		m_rotationTrack.ResumePlayback();
	}

	RE::NiPoint3 Timeline::GetTranslation(float a_time) const
	{
		return m_translationTrack.GetPointAtTime(a_time);
	}

	RE::BSTPoint2<float> Timeline::GetRotation(float a_time) const
	{
		return m_rotationTrack.GetPointAtTime(a_time);
	}

	size_t Timeline::GetTranslationPointCount() const
	{
		return m_translationTrack.GetPointCount();
	}

	size_t Timeline::GetRotationPointCount() const
	{
		return m_rotationTrack.GetPointCount();
	}

	float Timeline::GetDuration() const
	{
		return std::max(
			m_translationTrack.GetDuration(),
			m_rotationTrack.GetDuration()
		);
	}

	void Timeline::SetPlaybackMode(PlaybackMode a_mode)
	{
		m_translationTrack.SetPlaybackMode(a_mode);
		m_rotationTrack.SetPlaybackMode(a_mode);
	}

	void Timeline::SetLoopTimeOffset(float a_offset)
	{
		m_translationTrack.SetLoopTimeOffset(a_offset);
		m_rotationTrack.SetLoopTimeOffset(a_offset);
	}

	float Timeline::GetPlaybackTime() const
	{
		return m_translationTrack.GetPlaybackTime();
	}

	bool Timeline::IsPlaying() const
	{
		return m_translationTrack.IsPlaying() || m_rotationTrack.IsPlaying();
	}

	bool Timeline::IsPaused() const
	{
		return m_translationTrack.IsPaused() || m_rotationTrack.IsPaused();
	}

	void Timeline::ClearPoints()
	{
		m_translationTrack.ClearPoints();
		m_rotationTrack.ClearPoints();
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

	bool Timeline::AddTranslationPathFromFile(std::ifstream& a_file, float a_timeOffset, float a_conversionFactor)
	{
		return m_translationTrack.AddPathFromFile(a_file, a_timeOffset, a_conversionFactor);
	}

	bool Timeline::AddRotationPathFromFile(std::ifstream& a_file, float a_timeOffset, float a_conversionFactor)
	{
		return m_rotationTrack.AddPathFromFile(a_file, a_timeOffset, a_conversionFactor);
	}

	bool Timeline::ExportTranslationPath(std::ofstream& a_file, float a_conversionFactor) const
	{
		return m_translationTrack.ExportPath(a_file, a_conversionFactor);
	}

	bool Timeline::ExportRotationPath(std::ofstream& a_file, float a_conversionFactor) const
	{
		return m_rotationTrack.ExportPath(a_file, a_conversionFactor);
	}

	RE::NiPoint3 Timeline::GetTranslationPoint(size_t a_index) const
	{
		return m_translationTrack.GetPoint(a_index).m_point;
	}

	RE::BSTPoint2<float> Timeline::GetRotationPoint(size_t a_index) const
	{
		return m_rotationTrack.GetPoint(a_index).m_point;
	}

}  // namespace FCFW
