#include "FCFW_Utils.h"
#include "Offsets.h"
#include "_ts_SKSEFunctions.h"
#include "CLIBUtil/EditorID.hpp"

namespace FCFW {
    // ===== YAML Enum Conversion Helpers =====
    
    // Convert string to PointType enum
    PointType StringToPointType(const std::string& str) {
        if (str == "world") return PointType::kWorld;
        if (str == "reference") return PointType::kReference;
        if (str == "camera") return PointType::kCamera;
        log::warn("Unknown PointType '{}', defaulting to 'world'", str);
        return PointType::kWorld;
    }
    
    // Convert PointType enum to string
    std::string PointTypeToString(PointType type) {
        switch (type) {
            case PointType::kWorld: return "world";
            case PointType::kReference: return "reference";
            case PointType::kCamera: return "camera";
            default: return "world";
        }
    }
    
    // Convert string to InterpolationMode enum
    InterpolationMode StringToInterpolationMode(const std::string& str) {
        if (str == "none") return InterpolationMode::kNone;
        if (str == "linear") return InterpolationMode::kLinear;
        if (str == "cubicHermite" || str == "cubic") return InterpolationMode::kCubicHermite;
        log::warn("Unknown InterpolationMode '{}', defaulting to 'cubicHermite'", str);
        return InterpolationMode::kCubicHermite;
    }
    
    // Convert InterpolationMode enum to string
    std::string InterpolationModeToString(InterpolationMode mode) {
        switch (mode) {
            case InterpolationMode::kNone: return "none";
            case InterpolationMode::kLinear: return "linear";
            case InterpolationMode::kCubicHermite: return "cubicHermite";
            default: return "cubicHermite";
        }
    }
    
    // Convert string to PlaybackMode enum
    PlaybackMode StringToPlaybackMode(const std::string& str) {
        if (str == "end") return PlaybackMode::kEnd;
        if (str == "loop") return PlaybackMode::kLoop;
        if (str == "wait") return PlaybackMode::kWait;
        log::warn("Unknown PlaybackMode '{}', defaulting to 'end'", str);
        return PlaybackMode::kEnd;
    }
    
    // Convert PlaybackMode enum to string
    std::string PlaybackModeToString(PlaybackMode mode) {
        switch (mode) {
            case PlaybackMode::kEnd: return "end";
            case PlaybackMode::kLoop: return "loop";
            case PlaybackMode::kWait: return "wait";
            default: return "end";
        }
    }
    
    // Convert string to BodyPart enum
    BodyPart StringToBodyPart(const std::string& str) {
        if (str == "none") return BodyPart::kNone;
        if (str == "head") return BodyPart::kHead;
        if (str == "torso") return BodyPart::kTorso;
        log::warn("Unknown BodyPart '{}', defaulting to 'none'", str);
        return BodyPart::kNone;
    }
    
    // Convert BodyPart enum to string
    std::string BodyPartToString(BodyPart part) {
        switch (part) {
            case BodyPart::kNone: return "none";
            case BodyPart::kHead: return "head";
            case BodyPart::kTorso: return "torso";
            default: return "none";
        }
    }

    SKSE::PluginHandle ModNameToHandle(const char* a_modName) {
        if (!a_modName || strlen(a_modName) == 0) {
            log::error("{}: Invalid mod name (null or empty)", __FUNCTION__);
            return 0;
        }
        
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            log::error("{}: TESDataHandler not available", __FUNCTION__);
            return 0;
        }
        
        // Search through loaded files for matching mod name
        for (const auto& file : dataHandler->files) {
            if (file && file->fileName && std::string(file->fileName) == a_modName) {
                // Use compile index as plugin handle (unique per mod in load order)
                auto handle = static_cast<SKSE::PluginHandle>(file->compileIndex);
                return handle;
            }
        }
        
        // Mod not found in load order
        log::warn("{}: Mod '{}' not found in load order", __FUNCTION__, a_modName);
        return 0;
    }

    bool IsPluginHandleValid(SKSE::PluginHandle a_handle) {
        if (a_handle == 0) {
            return false;
        }
        
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            return false;
        }
        
        // Check if any loaded file has matching compile index
        for (const auto& file : dataHandler->files) {
            if (file && file->compileIndex == a_handle) {
                return true;
            }
        }
        
        return false;
    }

    void ComputeHermiteBasis(float t, float& h00, float& h10, float& h01, float& h11) {
        float t2 = t * t;
        float t3 = t2 * t;
        
        h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;   // basis for p1
        h10 = t3 - 2.0f * t2 + t;              // basis for m1
        h01 = -2.0f * t3 + 3.0f * t2;          // basis for p2
        h11 = t3 - t2;                          // basis for m2
    }

    float CubicHermiteInterpolate(float a0, float a1, float a2, float a3, float t) {
        // Compute Catmull-Rom tangents
        float m1 = (a2 - a0) * 0.5f;
        float m2 = (a3 - a1) * 0.5f;

        float h00, h10, h01, h11;
        ComputeHermiteBasis(t, h00, h10, h01, h11);

        return a1 * h00 + m1 * h10 + a2 * h01 + m2 * h11;
    };

    float CubicHermiteInterpolateAngular(float a0, float a1, float a2, float a3, float t) {
        // Convert to sin/cos (unit circle) representation
        float sin0 = std::sin(a0), cos0 = std::cos(a0);
        float sin1 = std::sin(a1), cos1 = std::cos(a1);
        float sin2 = std::sin(a2), cos2 = std::cos(a2);
        float sin3 = std::sin(a3), cos3 = std::cos(a3);
        
        // Compute Catmull-Rom tangents in sin/cos space
        float m1_sin = (sin2 - sin0) * 0.5f;
        float m1_cos = (cos2 - cos0) * 0.5f;
        float m2_sin = (sin3 - sin1) * 0.5f;
        float m2_cos = (cos3 - cos1) * 0.5f;
        
        float h00, h10, h01, h11;
        ComputeHermiteBasis(t, h00, h10, h01, h11);
        
        // Interpolate in sin/cos space
        float result_sin = sin1 * h00 + m1_sin * h10 + sin2 * h01 + m2_sin * h11;
        float result_cos = cos1 * h00 + m1_cos * h10 + cos2 * h01 + m2_cos * h11;
        
        // Convert back to angle
        return std::atan2(result_sin, result_cos);
    };

    bool ParseFCFWTimelineFileSections(
        std::ifstream& a_file,
        const std::string& a_sectionName,
        std::function<void(const std::map<std::string, std::string>&)> a_processSection
    ) {
        if (!a_file.is_open()) {
            return false;
        }

        std::string line;
        std::string currentSection;
        std::map<std::string, std::string> currentData;

        while (std::getline(a_file, line)) {
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue;
            }
            
            // Check for section header
            if (line[0] == '[' && line.back() == ']') {
                // Process previous section if it matches
                if (currentSection == a_sectionName) {
                    a_processSection(currentData);
                }

                // Start new section
                currentSection = line.substr(1, line.length() - 2);
                currentData.clear();
                continue;
            }
            
            // Parse key=value pairs
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                std::string value = line.substr(eqPos + 1);
                
                // Trim key and value
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                // Remove comments from value
                size_t commentPos = value.find(';');
                if (commentPos != std::string::npos) {
                    value = value.substr(0, commentPos);
                    value.erase(value.find_last_not_of(" \t") + 1);
                }
                
                currentData[key] = value;
            }
        }
        
        // Process last section if it matches
        if (currentSection == a_sectionName) {
            a_processSection(currentData);
        }

        return true;
    }

    // ===== Free Camera Direct Toggle (bypasses hooks) =====
    static std::uintptr_t g_freeCameraTrampoline = 0;

    void InitializeFreeCameraTrampoline(std::uintptr_t a_trampolineAddr) {
        g_freeCameraTrampoline = a_trampolineAddr;
        log::info("FCFW_Utils: Initialized free camera trampoline at 0x{:X}", a_trampolineAddr);
    }

    void ToggleFreeCameraNotHooked(bool a_freezeTime) {
        if (g_freeCameraTrampoline == 0) {
            log::error("FCFW_Utils: Free camera trampoline not initialized!");
            return;
        }

        auto* playerCamera = RE::PlayerCamera::GetSingleton();
        if (!playerCamera) {
            log::error("FCFW_Utils: PlayerCamera singleton not available");
            return;
        }

        // Call original function via trampoline, bypassing hook
        using FuncType = void(*)(RE::PlayerCamera*, bool);
        auto func = reinterpret_cast<FuncType>(g_freeCameraTrampoline);
        func(playerCamera, a_freezeTime);
    }
} // namespace FCFW
