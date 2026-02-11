#pragma once

#include "CameraTypes.h"

namespace FCFW {
    constexpr float EPSILON_COMPARISON = 0.0001f;
    
    void ComputeHermiteBasis(float t, float& h00, float& h10, float& h01, float& h11);

    float CubicHermiteInterpolate(float a0, float a1, float a2, float a3, float t);

    float CubicHermiteInterpolateAngular(float a0, float a1, float a2, float a3, float t);

    bool ParseFCFWTimelineFileSections(
        std::ifstream& a_file,
        const std::string& a_sectionName,
        std::function<void(const std::map<std::string, std::string>&)> a_processSection
    );

    SKSE::PluginHandle ModNameToHandle(const char* a_modName);
    bool IsPluginHandleValid(SKSE::PluginHandle a_handle);

    // ===== YAML Enum Conversion Helpers =====
    std::string PointTypeToString(PointType type);
    PointType StringToPointType(const std::string& str);
    
    std::string InterpolationModeToString(InterpolationMode mode);
    InterpolationMode StringToInterpolationMode(const std::string& str);
    
    std::string PlaybackModeToString(PlaybackMode mode);
    PlaybackMode StringToPlaybackMode(const std::string& str);
    
    std::string BodyPartToString(BodyPart part);
    BodyPart StringToBodyPart(const std::string& str);

    // Direct camera toggle that bypasses hooks
    void ToggleFreeCameraNotHooked(bool a_freezeTime = false);
    void InitializeFreeCameraTrampoline(std::uintptr_t a_trampolineAddr);
} // namespace FCFW

