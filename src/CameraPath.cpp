#include "CameraPath.h"
#include "FCFW_Utils.h"


namespace FCFW {
    
    // ===== TranslationPath implementations =====

    TranslationPoint TranslationPath::GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const {
        Transition transition(a_time, InterpolationMode::kCubicHermite, a_easeIn, a_easeOut);
        return TranslationPoint(transition, PointType::kCamera);
    }

    bool TranslationPath::AddPathFromFile(std::ifstream& a_file, float a_timeOffset, float a_conversionFactor) {
        if (!a_file.is_open()) {
            log::error("{}: File is not open", __FUNCTION__);
            return false;
        }
        
        return ParseFCFWTimelineFileSections(a_file, "TranslatePoint", [this, a_timeOffset](const auto& data) {
            if (data.find("Time") != data.end()) {
                try {
                    float time = std::stof(data.at("Time")) + a_timeOffset;
                    bool easeIn = data.count("EaseIn") ? (std::stoi(data.at("EaseIn")) != 0) : false;
                    bool easeOut = data.count("EaseOut") ? (std::stoi(data.at("EaseOut")) != 0) : false;
                    
                    int modeValue = data.count("InterpolationMode") ? std::stoi(data.at("InterpolationMode")) : 2;
                    InterpolationMode mode = ToInterpolationMode(modeValue);
                    
                    Transition translationTrans(time, mode, easeIn, easeOut);
                    
                    // Read PointType
                    int pointTypeValue = data.count("PointType") ? std::stoi(data.at("PointType")) : 0;
                    PointType pointType = ToPointType(pointTypeValue);
                    
                    if (pointType == PointType::kWorld) {
                        // Static position-based point - read PositionX/Y/Z
                        float posX = data.count("PositionX") ? std::stof(data.at("PositionX")) : 0.0f;
                        float posY = data.count("PositionY") ? std::stof(data.at("PositionY")) : 0.0f;
                        float posZ = data.count("PositionZ") ? std::stof(data.at("PositionZ")) : 0.0f;
                        RE::NiPoint3 position(posX, posY, posZ);
                        TranslationPoint point(translationTrans, PointType::kWorld, position, RE::NiPoint3{});
                        AddPoint(point);
                    } else if (pointType == PointType::kCamera) {
                        // Camera point - read OffsetX/Y/Z
                        float offsetX = data.count("OffsetX") ? std::stof(data.at("OffsetX")) : 0.0f;
                        float offsetY = data.count("OffsetY") ? std::stof(data.at("OffsetY")) : 0.0f;
                        float offsetZ = data.count("OffsetZ") ? std::stof(data.at("OffsetZ")) : 0.0f;
                        RE::NiPoint3 offset(offsetX, offsetY, offsetZ);
                        TranslationPoint point(translationTrans, PointType::kCamera, RE::NiPoint3{}, offset);
                        AddPoint(point);
                    } else if (pointType == PointType::kReference) {
                        // Reference-based point - read OffsetX/Y/Z
                        float offsetX = data.count("OffsetX") ? std::stof(data.at("OffsetX")) : 0.0f;
                        float offsetY = data.count("OffsetY") ? std::stof(data.at("OffsetY")) : 0.0f;
                        float offsetZ = data.count("OffsetZ") ? std::stof(data.at("OffsetZ")) : 0.0f;
                        RE::NiPoint3 offset(offsetX, offsetY, offsetZ);
                        bool isOffsetRelative = data.count("isOffsetRelative") ? (std::stoi(data.at("isOffsetRelative")) != 0) : false;
                        // Reference-based point
                        RE::TESObjectREFR* reference = nullptr;
                        uint32_t formID = 0;
                    
                    // Try EditorID first (load-order independent)
                    if (data.count("RefEditorID")) {
                        std::string editorID = data.at("RefEditorID");
                        reference = RE::TESForm::LookupByEditorID<RE::TESObjectREFR>(editorID);
                        
                        if (reference) {
                            // Validate plugin name if available
                            if (data.count("RefPlugin")) {
                                auto* file = reference->GetFile(0);
                                if (file && std::string(file->fileName) != data.at("RefPlugin")) {
                                    log::warn("{}: Reference '{}' found but from different plugin (expected: {}, got: {})", 
                                             __FUNCTION__, editorID, data.at("RefPlugin"), file->fileName);
                                }
                            }
                        } else {
                            log::warn("{}: Failed to resolve reference EditorID: {}", __FUNCTION__, editorID);
                        }
                    }
                    
                    // Fallback to FormID if EditorID lookup failed
                    if (!reference && data.count("RefFormID")) {
                        formID = std::stoul(data.at("RefFormID"), nullptr, 16);
                        if (formID != 0) {
                            auto* form = RE::TESForm::LookupByID(formID);
                            reference = form ? form->As<RE::TESObjectREFR>() : nullptr;
                            if (!reference) {
                                log::warn("{}: Failed to resolve reference FormID: 0x{:X}", __FUNCTION__, formID);
                            }
                        }
                    }
                    
                    if (reference) {
                        TranslationPoint point(translationTrans, PointType::kReference, RE::NiPoint3{}, offset, reference, isOffsetRelative);
                        AddPoint(point);
                    } else {
                        log::warn("{}: Failed to resolve reference FormID 0x{:X}, using offset as absolute position", __FUNCTION__, formID);
                        TranslationPoint point(translationTrans, PointType::kWorld, offset, RE::NiPoint3{});
                        AddPoint(point);
                    }
                }
                } catch (const std::exception& e) {
                    log::warn("{}: Skipping invalid TranslatePoint entry: {}", __FUNCTION__, e.what());
                }
            }
        });
    }

    bool TranslationPath::ExportPath(std::ofstream& a_file, float a_conversionFactor) const {
        if (!a_file.is_open()) {
            log::error("{}: File is not open", __FUNCTION__);
            return false;
        }

        for (const auto& point : m_points) {
            a_file << "[TranslatePoint]\n";
            a_file << "PointType=" << static_cast<int>(point.m_pointType) << "\n";
            
            // Write position values with appropriate field names
            if (point.m_pointType == PointType::kWorld) {
                // kWorld: write as absolute PositionX/Y/Z
                a_file << "PositionX=" << point.m_point.x << "\n";
                a_file << "PositionY=" << point.m_point.y << "\n";
                a_file << "PositionZ=" << point.m_point.z << "\n";
            } else {
                // kCamera/kReference: write as OffsetX/Y/Z
                a_file << "OffsetX=" << point.m_offset.x << "\n";
                a_file << "OffsetY=" << point.m_offset.y << "\n";
                a_file << "OffsetZ=" << point.m_offset.z << "\n";
            }
            
            if (point.m_pointType == PointType::kReference && point.m_reference) {
                // Reference-based point - write reference info
                const char* editorID = point.m_reference->GetFormEditorID();
                
                if (editorID && editorID[0] != '\0') {
                    a_file << "RefEditorID=" << editorID << "\n";
                } else {
                    log::warn("{}: Reference 0x{:X} has no EditorID - timeline may not be portable across load orders. Install po3's Tweaks for improved EditorID support.", 
                             __FUNCTION__, point.m_reference->GetFormID());
                }
                
                auto* file = point.m_reference->GetFile(0);
                if (file) {
                    a_file << "RefPlugin=" << file->fileName << "\n";
                }
                a_file << "RefFormID=0x" << std::hex << std::uppercase << point.m_reference->GetFormID() << std::dec << "\n";
                a_file << "isOffsetRelative=" << (point.m_isOffsetRelative ? 1 : 0) << "\n";
            }
            
            a_file << "Time=" << point.m_transition.m_time << "\n";
            a_file << "InterpolationMode=" << static_cast<int>(point.m_transition.m_mode) << "\n";
            a_file << "EaseIn=" << (point.m_transition.m_easeIn ? 1 : 0) << "\n";
            a_file << "EaseOut=" << (point.m_transition.m_easeOut ? 1 : 0) << "\n";
            a_file << "\n";
            
            if (!a_file.good()) {
                log::error("{}: Write error", __FUNCTION__);
                return false;
            }
        }
        
        return true;
    }

    // ===== RotationPath implementations =====
    
    RotationPoint RotationPath::GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const {
        Transition transition(a_time, InterpolationMode::kCubicHermite, a_easeIn, a_easeOut);
        return RotationPoint(transition, PointType::kCamera);
    }

    bool RotationPath::AddPathFromFile(std::ifstream& a_file, float a_timeOffset, float a_conversionFactor) {
        if (!a_file.is_open()) {
            log::error("{}: File is not open", __FUNCTION__);
            return false;
        }
        
        return ParseFCFWTimelineFileSections(a_file, "RotatePoint", [this, a_conversionFactor, a_timeOffset](const auto& data) {
            if (data.find("Time") != data.end()) {
                try {
                    float time = std::stof(data.at("Time")) + a_timeOffset;
                    bool easeIn = data.count("EaseIn") ? (std::stoi(data.at("EaseIn")) != 0) : false;
                    bool easeOut = data.count("EaseOut") ? (std::stoi(data.at("EaseOut")) != 0) : false;
                    
                    int modeValue = data.count("InterpolationMode") ? std::stoi(data.at("InterpolationMode")) : 2;
                    InterpolationMode mode =ToInterpolationMode(modeValue);
                    
                    Transition rotationTrans(time, mode, easeIn, easeOut);
                    
                    int pointTypeValue = data.count("PointType") ? std::stoi(data.at("PointType")) : 0;
                    PointType pointType = ToPointType(pointTypeValue);
                    
                    // Read rotation/offset values based on point type
                    RE::BSTPoint2<float> rotation;
                    RE::BSTPoint2<float> offset;
                    
                    if (pointType == PointType::kWorld) {
                        // kWorld: read Pitch/Yaw
                        rotation.x = (data.count("Pitch") ? std::stof(data.at("Pitch")) : 0.0f) * a_conversionFactor;
                        rotation.y = (data.count("Yaw") ? std::stof(data.at("Yaw")) : 0.0f) * a_conversionFactor;
                    } else {
                        // kCamera/kReference: read OffsetPitch/Yaw
                        offset.x = (data.count("OffsetPitch") ? std::stof(data.at("OffsetPitch")) : 0.0f) * a_conversionFactor;
                        offset.y = (data.count("OffsetYaw") ? std::stof(data.at("OffsetYaw")) : 0.0f) * a_conversionFactor;
                    }
                    
                    bool isOffsetRelative = data.count("isOffsetRelative") ? (std::stoi(data.at("isOffsetRelative")) != 0) : false;
                    
                    if (pointType == PointType::kCamera) {
                        // Camera point with offset
                        RotationPoint point(rotationTrans, PointType::kCamera, RE::BSTPoint2<float>{}, offset);
                        AddPoint(point);
                    } else if (pointType == PointType::kReference) {
                        // Reference-based point
                        RE::TESObjectREFR* reference = nullptr;
                        uint32_t formID = 0;
                        
                        // Try EditorID first (load-order independent)
                        if (data.count("RefEditorID")) {
                            std::string editorID = data.at("RefEditorID");
                            reference = RE::TESForm::LookupByEditorID<RE::TESObjectREFR>(editorID);
                            
                            if (reference) {
                                // Validate plugin name if available
                                if (data.count("RefPlugin")) {
                                    auto* file = reference->GetFile(0);
                                    if (file && std::string(file->fileName) != data.at("RefPlugin")) {
                                        log::warn("{}: Reference '{}' found but from different plugin (expected: {}, got: {})", 
                                                 __FUNCTION__, editorID, data.at("RefPlugin"), file->fileName);
                                    }
                                }
                            } else {
                                log::warn("{}: Failed to resolve reference EditorID: {}", __FUNCTION__, editorID);
                            }
                        }
                        
                        // Fallback to FormID if EditorID lookup failed
                        if (!reference && data.count("RefFormID")) {
                            formID = std::stoul(data.at("RefFormID"), nullptr, 16);
                            if (formID != 0) {
                                auto* form = RE::TESForm::LookupByID(formID);
                                reference = form ? form->As<RE::TESObjectREFR>() : nullptr;
                                if (!reference) {
                                    log::warn("{}: Failed to resolve reference FormID: 0x{:X}", __FUNCTION__, formID);
                                }
                            }
                        }
                        
                        if (reference) {
                            RotationPoint point(rotationTrans, PointType::kReference, RE::BSTPoint2<float>{}, offset, reference, isOffsetRelative);
                            AddPoint(point);
                        } else {
                            log::warn("{}: Failed to resolve reference FormID 0x{:X}, using offset as absolute rotation", __FUNCTION__, formID);
                            RotationPoint point(rotationTrans, PointType::kWorld, offset, RE::BSTPoint2<float>{});
                            AddPoint(point);
                        }
                    } else {
                        // Static rotation-based point (kWorld)
                        RotationPoint point(rotationTrans, PointType::kWorld, rotation, RE::BSTPoint2<float>{});
                        AddPoint(point);
                    }
                } catch (const std::exception& e) {
                    log::warn("{}: Skipping invalid RotatePoint entry: {}", __FUNCTION__, e.what());
                }
            }
        });
    }

    bool RotationPath::ExportPath(std::ofstream& a_file, float a_conversionFactor) const {
        if (!a_file.is_open()) {
            log::error("{}: File is not open", __FUNCTION__);
            return false;
        }
     
        for (const auto& point : m_points) {
            a_file << "[RotatePoint]\n";
            a_file << "PointType=" << static_cast<int>(point.m_pointType) << "\n";
            
            // Write rotation values with appropriate field names
            if (point.m_pointType == PointType::kWorld) {
                // kWorld: write as absolute Pitch/Yaw
                a_file << "Pitch=" << (point.m_point.x * a_conversionFactor) << "\n";
                a_file << "Yaw=" << (point.m_point.y * a_conversionFactor) << "\n";
            } else {
                // kCamera/kReference: write as OffsetPitch/Yaw
                a_file << "OffsetPitch=" << (point.m_offset.x * a_conversionFactor) << "\n";
                a_file << "OffsetYaw=" << (point.m_offset.y * a_conversionFactor) << "\n";
            }
            
            if (point.m_pointType == PointType::kReference && point.m_reference) {
                // Reference-based point - write reference info
                a_file << "isOffsetRelative=" << (point.m_isOffsetRelative ? 1 : 0) << "\n";
                
                // Try to get EditorID for load-order independence
                // Note: GetFormEditorID() may not work reliably without po3's Tweaks installed
                const char* editorID = point.m_reference->GetFormEditorID();
                
                if (editorID && editorID[0] != '\0') {
                    a_file << "RefEditorID=" << editorID << "\n";
                } else {
                    log::warn("{}: Reference 0x{:X} has no EditorID - timeline may not be portable across load orders. Install po3's Tweaks for improved EditorID support.", 
                             __FUNCTION__, point.m_reference->GetFormID());
                }
                
                // Always write plugin name and FormID for debugging/fallback
                auto* file = point.m_reference->GetFile(0);
                if (file) {
                    a_file << "RefPlugin=" << file->fileName << "\n";
                }
                a_file << "RefFormID=0x" << std::hex << std::uppercase << point.m_reference->GetFormID() << std::dec << "\n";
            }
            
            a_file << "Time=" << point.m_transition.m_time << "\n";
            a_file << "InterpolationMode=" << static_cast<int>(point.m_transition.m_mode) << "\n";
            a_file << "EaseIn=" << (point.m_transition.m_easeIn ? 1 : 0) << "\n";
            a_file << "EaseOut=" << (point.m_transition.m_easeOut ? 1 : 0) << "\n";
            a_file << "\n";
            
            if (!a_file.good()) {
                log::error("{}: Write error", __FUNCTION__);
                return false;
            }
        }

        return true;
    }

} // namespace FCFW
