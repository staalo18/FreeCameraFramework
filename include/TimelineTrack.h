#pragma once

#include "CameraPath.h"
#include "CameraTypes.h"
#include "FCFW_Utils.h"

namespace FCFW
{
	template <typename PathType>
	class TimelineTrack
	{
	public:
		using TransitionPoint = typename PathType::TransitionPoint;

		TimelineTrack() = default;
		~TimelineTrack() = default;

		void AddPoint(const TransitionPoint& a_point);
		void RemovePoint(size_t a_index);
		void ClearPoints();

		void UpdateTimeline(float a_deltaTime);
		void StartPlayback();
		void ResetTimeline();
		void PausePlayback();
		void ResumePlayback();

		typename PathType::ValueType GetPointAtTime(float a_time) const;

		size_t GetPointCount() const;
		float GetDuration() const;
		float GetPlaybackTime() const { return m_playbackTime; }
		void SetPlaybackTime(float a_time) { m_playbackTime = std::clamp(a_time, 0.0f, GetDuration()); }
		bool IsPlaying() const { return m_isPlaying; }
		bool IsPaused() const { return m_isPaused; }

		void SetPlaybackMode(PlaybackMode a_mode);
		PlaybackMode GetPlaybackMode() const { return m_playbackMode; }

		void SetLoopTimeOffset(float a_offset);
		float GetLoopTimeOffset() const { return m_loopTimeOffset; }

		void UpdateCameraPoints();

		TransitionPoint GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const;
		const TransitionPoint& GetPoint(size_t a_index) const;
		
		bool AddPathFromFile(const std::string& a_filePath, float a_timeOffset = 0.0f, float a_conversionFactor = 1.0f);
		bool ExportPath(std::ofstream& a_file, float a_conversionFactor = 1.0f) const;

	private:
		typename PathType::ValueType GetInterpolatedPoint(size_t a_index, float a_progress) const;
		typename PathType::ValueType GetPointLinear(size_t a_index, float a_progress) const;
		typename PathType::ValueType GetPointCubicHermite(size_t a_index, float a_progress) const;

		PathType m_path;                          // CameraPath<TransitionPoint> - stores ordered points
		float m_playbackTime{ 0.0f };             // Current position in timeline (seconds)
		bool m_isPlaying{ false };                // Playback active
		bool m_isPaused{ false };                 // Playback paused
		PlaybackMode m_playbackMode{ PlaybackMode::kEnd };  // kEnd (stop) or kLoop (wrap)
		float m_loopTimeOffset{ 0.0f };           // Extra time for loop interpolation (last→first)
	};

	using TranslationTrack = TimelineTrack<TranslationPath>;
	using RotationTrack = TimelineTrack<RotationPath>;
	using FOVTrack = TimelineTrack<FOVPath>;

	template <typename PathType>
	void TimelineTrack<PathType>::AddPoint(const TransitionPoint& a_point)
	{
		m_path.AddPoint(a_point);
		ResetTimeline();
	}

	template <typename PathType>
	void TimelineTrack<PathType>::RemovePoint(size_t a_index)
	{
		m_path.RemovePoint(a_index);
		ResetTimeline();
	}

	template <typename PathType>
	void TimelineTrack<PathType>::ClearPoints()
	{
		m_path.ClearPath();
		ResetTimeline();
		m_playbackMode = PlaybackMode::kEnd;
	}

	template <typename PathType>
	void TimelineTrack<PathType>::UpdateTimeline(float a_deltaTime)
	{
		if (m_isPaused || !m_isPlaying) {
			return;
		}

		size_t pointCount = GetPointCount();
		if (pointCount == 0) {
			m_isPlaying = false;
			return;
		}

		m_playbackTime += a_deltaTime;

		// Get timeline duration (includes loop offset if in loop mode)
		float timelineDuration = GetDuration();

		// Check for completion and handle based on playback mode
		if (m_playbackTime >= timelineDuration) {
			if (m_playbackMode == PlaybackMode::kLoop) {
				// Loop: use modulo to wrap time seamlessly
				m_playbackTime = std::fmod(m_playbackTime, timelineDuration);
			} else if (m_playbackMode == PlaybackMode::kWait) {
				m_playbackTime = timelineDuration;
			} else {
				// End: clamp to final position and stop
				m_playbackTime = timelineDuration;
				m_isPlaying = false;
			}
		}
	}

	template <typename PathType>
	void TimelineTrack<PathType>::StartPlayback()
	{
		m_path.UpdateCameraPoints();  // Store current camera values for kCamera points
		m_isPlaying = true;
		m_isPaused = false;
	}

	template <typename PathType>
	void TimelineTrack<PathType>::ResetTimeline()
	{
		m_playbackTime = 0.0f;
		m_isPlaying = false;
		m_isPaused = false;
	}

	template <typename PathType>
	void TimelineTrack<PathType>::PausePlayback()
	{
		m_isPaused = true;
	}

	template <typename PathType>
	void TimelineTrack<PathType>::ResumePlayback()
	{
		m_isPaused = false;
	}

	template <typename PathType>
	typename PathType::ValueType TimelineTrack<PathType>::GetPointAtTime(float a_time) const
	{
		auto pointCount = GetPointCount();
		if (pointCount == 0) {
			return TransitionPoint{}.GetPoint();
		}

		// Calculate state for requested time
		size_t index = 0;
		float progress = 0.0f;

		// Calculate segment state (index and progress)
		float lastPointTime = m_path.GetPoint(pointCount - 1).m_transition.m_time;

		// Check if we're in the virtual loop segment (after last point)
		if (m_playbackMode == PlaybackMode::kLoop && m_loopTimeOffset > 0.0f && a_time > lastPointTime) {
			index = pointCount;  // Virtual index beyond last point
			progress = (a_time - lastPointTime) / m_loopTimeOffset;
			progress = std::clamp(progress, 0.0f, 1.0f);
		} else {  // Find the segment containing this time
			size_t targetIndex = 0;
			for (size_t i = 0; i < pointCount; ++i) {
				const auto& point = m_path.GetPoint(i);
				if (a_time <= point.m_transition.m_time) {
					targetIndex = i;
					break;
				}
				targetIndex = i + 1;
			}

			if (targetIndex >= pointCount) {
				targetIndex = pointCount - 1;
				index = targetIndex;
				progress = 1.0f;
			} else {
				// Calculate progress within this segment
				index = targetIndex;

				if (targetIndex > 0) {
					const auto& prevPoint = m_path.GetPoint(targetIndex - 1);
					const auto& currentPoint = m_path.GetPoint(targetIndex);
					float segmentDuration = currentPoint.m_transition.m_time - prevPoint.m_transition.m_time;
					if (segmentDuration > 0.0f) {
						progress = (a_time - prevPoint.m_transition.m_time) / segmentDuration;
						progress = std::clamp(progress, 0.0f, 1.0f);
					} else {
						progress = 1.0f;
					}
				} else {
					progress = 0.0f;
				}
			}
		}

		// Get interpolated point for requested time
		return GetInterpolatedPoint(index, progress);
	}

	template <typename PathType>
	size_t TimelineTrack<PathType>::GetPointCount() const
	{
		return m_path.GetPointCount();
	}

	template <typename PathType>
	float TimelineTrack<PathType>::GetDuration() const
	{
		size_t pointCount = GetPointCount();
		if (pointCount == 0) {
			return 0.0f;
		}
		float lastPointTime = m_path.GetPoint(pointCount - 1).m_transition.m_time;
		// In loop mode, add offset to create interpolation time from last to first point
		if (m_playbackMode == PlaybackMode::kLoop) {
			return lastPointTime + m_loopTimeOffset;
		}
		return lastPointTime;
	}

	template <typename PathType>
	void TimelineTrack<PathType>::SetPlaybackMode(PlaybackMode a_mode)
	{
		m_playbackMode = a_mode;
	}

	template <typename PathType>
	void TimelineTrack<PathType>::SetLoopTimeOffset(float a_offset)
	{
		m_loopTimeOffset = a_offset;
	}

	template <typename PathType>
	void TimelineTrack<PathType>::UpdateCameraPoints()
	{
		m_path.UpdateCameraPoints();
	}

	template <typename PathType>
	typename PathType::ValueType TimelineTrack<PathType>::GetInterpolatedPoint(size_t a_index, float a_progress) const
	{
		if (GetPointCount() == 0) {
			return TransitionPoint{}.GetPoint();
		}

		size_t currentIdx = a_index;
		if (currentIdx >= GetPointCount()) {
			currentIdx = GetPointCount() - 1;
		}

		const auto& currentPoint = m_path.GetPoint(currentIdx);

		switch (currentPoint.m_transition.m_mode) {
			case InterpolationMode::kNone:
				return currentPoint.GetPoint();
			case InterpolationMode::kLinear:
				return GetPointLinear(a_index, a_progress);
			case InterpolationMode::kCubicHermite:
				return GetPointCubicHermite(a_index, a_progress);
			default:
				return GetPointLinear(a_index, a_progress);
		}
	}

	template <typename PathType>
	typename PathType::ValueType TimelineTrack<PathType>::GetPointLinear(size_t a_index, float a_progress) const
	{
		const size_t pointCount = GetPointCount();

		if (pointCount == 0) {
			return TransitionPoint{}.GetPoint();
		}

		size_t currentIdx = a_index;

		// Handle virtual loop segment: treat as interpolating to point 0
		bool isVirtualSegment = (m_playbackMode == PlaybackMode::kLoop && currentIdx == pointCount);
		if (isVirtualSegment) {
			currentIdx = 0;
		}

		if (currentIdx >= pointCount) {
			currentIdx = pointCount - 1;
		}

		const auto& currentPoint = m_path.GetPoint(currentIdx);

		// For virtual segment or normal segments starting after point 0
		if (currentIdx == 0 && !isVirtualSegment) {
			return currentPoint.GetPoint();
		}

		// Get previous point (for virtual segment, it's the last point)
		const auto& prevPoint = isVirtualSegment ? m_path.GetPoint(pointCount - 1) : m_path.GetPoint(currentIdx - 1);

		if (prevPoint.IsNearlyEqual(currentPoint)) {
			return currentPoint.GetPoint();
		}

		float t = _ts_SKSEFunctions::ApplyEasing(a_progress,
			currentPoint.m_transition.m_easeIn,
			currentPoint.m_transition.m_easeOut);

		// Use the LinearInterpolate method which handles angular wrapping for rotation
		TransitionPoint result = prevPoint.LinearInterpolate(prevPoint, currentPoint, t);
		return result.GetPoint();
	}

	template <typename PathType>
	typename PathType::ValueType TimelineTrack<PathType>::GetPointCubicHermite(size_t a_index, float a_progress) const
	{
		size_t pointCount = GetPointCount();

		if (pointCount == 0) {
			return TransitionPoint{}.GetPoint();
		}

		if (pointCount == 1) {
			return m_path.GetPoint(0).GetPoint();
		}

		size_t currentIdx = a_index;

		// Handle virtual loop segment: treat as interpolating to point 0
		bool isVirtualSegment = (m_playbackMode == PlaybackMode::kLoop && currentIdx == pointCount);
		if (isVirtualSegment) {
			currentIdx = 0;
		}

		if (currentIdx >= pointCount) {
			return m_path.GetPoint(pointCount - 1).GetPoint();
		}

		// For virtual segment or normal segments starting after point 0
		if (currentIdx == 0 && !isVirtualSegment) {
			return m_path.GetPoint(0).GetPoint();
		}

		const auto& currentPoint = m_path.GetPoint(currentIdx);
		// Get previous point (for virtual segment, it's the last point)
		const auto& prevPoint = isVirtualSegment ? m_path.GetPoint(pointCount - 1) : m_path.GetPoint(currentIdx - 1);

		if (prevPoint.IsNearlyEqual(currentPoint)) {
			return prevPoint.GetPoint();
		}

		// Get neighboring points for tangent computation
		TransitionPoint pt0, pt1, pt2, pt3;

		if (m_playbackMode == PlaybackMode::kLoop) {
			// Loop mode: modulo all indices to wrap around
			pt0 = m_path.GetPoint((currentIdx - 2 + pointCount) % pointCount);
			pt1 = m_path.GetPoint((currentIdx - 1 + pointCount) % pointCount);
			pt2 = m_path.GetPoint(currentIdx % pointCount);
			pt3 = m_path.GetPoint((currentIdx + 1) % pointCount);
		} else {
			// End mode: use boundary clamping
			bool hasP0 = currentIdx >= 2;
			bool hasP3 = currentIdx + 1 < pointCount;

			pt0 = hasP0 ? m_path.GetPoint(currentIdx - 2) : prevPoint;
			pt1 = prevPoint;
			pt2 = currentPoint;
			pt3 = hasP3 ? m_path.GetPoint(currentIdx + 1) : currentPoint;
		}

		float t = _ts_SKSEFunctions::ApplyEasing(a_progress,
			currentPoint.m_transition.m_easeIn,
			currentPoint.m_transition.m_easeOut);

		TransitionPoint result = pt1.CubicHermite(pt0, pt1, pt2, pt3, t);
		return result.GetPoint();
	}

	template <typename PathType>
	typename PathType::TransitionPoint TimelineTrack<PathType>::GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const
	{
		return m_path.GetPointAtCamera(a_time, a_easeIn, a_easeOut);
	}

	template <typename PathType>
	const typename PathType::TransitionPoint& TimelineTrack<PathType>::GetPoint(size_t a_index) const
	{
		return m_path.GetPoint(a_index);
	}

	template <typename PathType>
	bool TimelineTrack<PathType>::AddPathFromFile(const std::string& a_filePath, float a_timeOffset, float a_conversionFactor)
	{
		return m_path.AddPathFromFile(a_filePath, a_timeOffset, a_conversionFactor);
	}

	template <typename PathType>
	bool TimelineTrack<PathType>::ExportPath(std::ofstream& a_file, float a_conversionFactor) const
	{
		return m_path.ExportPath(a_file, a_conversionFactor);
	}

}  // namespace FCFW
