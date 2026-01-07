#pragma once


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

     RE::NiPointer<RE::NiAVObject> GetTargetPoint(RE::Actor* a_actor);

    SKSE::PluginHandle ModNameToHandle(const char* a_modName);
} // namespace FCFW

