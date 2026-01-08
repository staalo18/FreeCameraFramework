#include "CameraPath.h"
#include "FCFW_Utils.h"
#include <yaml-cpp/yaml.h>


namespace FCFW {
    
    // ===== Helper Templates for YAML Import/Export =====
    
    namespace {
        // Template traits for point type specialization
        template<typename T> struct PointTraits;
        
        template<> struct PointTraits<TranslationPoint> {
            using ValueType = RE::NiPoint3;
            static constexpr const char* SectionName = "translationPoints";
            static constexpr const char* ValueKey = "position";
            static constexpr size_t ValueSize = 3;
            
            static ValueType ReadValue(const YAML::Node& node, float conversionFactor) {
                return ValueType{
                    node[0].as<float>() * conversionFactor,
                    node[1].as<float>() * conversionFactor,
                    node[2].as<float>() * conversionFactor
                };
            }
            
            static void WriteValue(YAML::Emitter& out, const ValueType& value, float conversionFactor) {
                out << YAML::Flow << YAML::BeginSeq 
                    << (value.x * conversionFactor) 
                    << (value.y * conversionFactor) 
                    << (value.z * conversionFactor) 
                    << YAML::EndSeq;
            }
        };
        
        template<> struct PointTraits<RotationPoint> {
            using ValueType = RE::BSTPoint2<float>;
            static constexpr const char* SectionName = "rotationPoints";
            static constexpr const char* ValueKey = "rotation";
            static constexpr size_t ValueSize = 2;
            
            static ValueType ReadValue(const YAML::Node& node, float conversionFactor) {
                return ValueType{
                    node[0].as<float>() * conversionFactor,
                    node[1].as<float>() * conversionFactor
                };
            }
            
            static void WriteValue(YAML::Emitter& out, const ValueType& value, float conversionFactor) {
                out << YAML::Flow << YAML::BeginSeq 
                    << (value.x * conversionFactor) 
                    << (value.y * conversionFactor) 
                    << YAML::EndSeq;
            }
        };
    }
    
    // Template helper for importing points from YAML
    template<typename PointType, typename PathType>
    bool ImportPathFromYAML(PathType* path, const std::string& a_filePath, float a_timeOffset, float a_conversionFactor) {
        using Traits = PointTraits<PointType>;
        
        try {
            YAML::Node root = YAML::LoadFile(a_filePath);
            
            // Check format version (default to 1 for legacy files)
            int formatVersion = 1;
            if (root["formatVersion"]) {
                formatVersion = root["formatVersion"].as<int>();
                if (formatVersion != 1) {
                    log::warn("{}: Unknown formatVersion {} in file, attempting to parse as version 1", 
                             __FUNCTION__, formatVersion);
                }
            } else {
                log::info("{}: No formatVersion specified, assuming version 1", __FUNCTION__);
            }
            
            if (!root[Traits::SectionName]) {
                log::info("{}: No '{}' section in YAML file", __FUNCTION__, Traits::SectionName);
                return true;
            }
            
            for (const auto& pointNode : root[Traits::SectionName]) {
                if (!pointNode["time"]) {
                    log::warn("{}: Skipping point without 'time' field", __FUNCTION__);
                    continue;
                }
                
                float time = pointNode["time"].as<float>() + a_timeOffset;
                bool easeIn = pointNode["easeIn"] ? pointNode["easeIn"].as<bool>() : false;
                bool easeOut = pointNode["easeOut"] ? pointNode["easeOut"].as<bool>() : false;
                
                InterpolationMode mode = InterpolationMode::kCubicHermite;
                if (pointNode["interpolationMode"]) {
                    mode = StringToInterpolationMode(pointNode["interpolationMode"].as<std::string>());
                }
                
                Transition transition(time, mode, easeIn, easeOut);
                
                FCFW::PointType pointType = FCFW::PointType::kWorld;
                if (pointNode["type"]) {
                    pointType = StringToPointType(pointNode["type"].as<std::string>());
                }
                
                if (pointType == FCFW::PointType::kWorld) {
                    if (pointNode[Traits::ValueKey] && pointNode[Traits::ValueKey].IsSequence() && 
                        pointNode[Traits::ValueKey].size() == Traits::ValueSize) {
                        auto value = Traits::ReadValue(pointNode[Traits::ValueKey], a_conversionFactor);
                        PointType point(transition, FCFW::PointType::kWorld, value);
                        path->AddPoint(point);
                    } else {
                        log::warn("{}: World point at time {} missing or invalid '{}' array", 
                                 __FUNCTION__, time, Traits::ValueKey);
                        continue;
                    }
                    
                } else if (pointType == FCFW::PointType::kCamera) {
                    typename Traits::ValueType offset{};
                    if (pointNode["offset"] && pointNode["offset"].IsSequence() && 
                        pointNode["offset"].size() == Traits::ValueSize) {
                        offset = Traits::ReadValue(pointNode["offset"], a_conversionFactor);
                    }
                    PointType point(transition, FCFW::PointType::kCamera, typename Traits::ValueType{}, offset);
                    path->AddPoint(point);
                    
                } else if (pointType == FCFW::PointType::kReference) {
                    typename Traits::ValueType offset{};
                    if (pointNode["offset"] && pointNode["offset"].IsSequence() && 
                        pointNode["offset"].size() == Traits::ValueSize) {
                        offset = Traits::ReadValue(pointNode["offset"], a_conversionFactor);
                    }
                    
                    bool isOffsetRelative = pointNode["isOffsetRelative"] ? pointNode["isOffsetRelative"].as<bool>() : false;
                    
                    RE::TESObjectREFR* reference = nullptr;
                    
                    if (!pointNode["reference"]) {
                        log::warn("{}: Reference point at time {} missing 'reference' section", __FUNCTION__, time);
                        continue;
                    }
                    
                    auto refNode = pointNode["reference"];
                    
                    // Try EditorID first (load-order independent)
                    if (refNode["editorID"]) {
                        std::string editorID = refNode["editorID"].as<std::string>();
                        reference = RE::TESForm::LookupByEditorID<RE::TESObjectREFR>(editorID);
                        
                        if (reference && refNode["plugin"]) {
                            auto* file = reference->GetFile(0);
                            if (file && std::string(file->fileName) != refNode["plugin"].as<std::string>()) {
                                log::warn("{}: Reference '{}' found but from different plugin (expected: {}, got: {})", 
                                         __FUNCTION__, editorID, refNode["plugin"].as<std::string>(), file->fileName);
                            }
                        } else if (!reference) {
                            log::warn("{}: Failed to resolve reference EditorID: {}", __FUNCTION__, editorID);
                        }
                    }
                    
                    // Fallback to FormID if EditorID lookup failed
                    if (!reference && refNode["formID"]) {
                        std::string formIDStr = refNode["formID"].as<std::string>();
                        uint32_t formID = std::stoul(formIDStr, nullptr, 16);
                        if (formID != 0) {
                            auto* form = RE::TESForm::LookupByID(formID);
                            reference = form ? form->As<RE::TESObjectREFR>() : nullptr;
                            if (!reference) {
                                log::warn("{}: Failed to resolve reference FormID: {}", __FUNCTION__, formIDStr);
                            }
                        }
                    }
                    
                    if (reference) {
                        PointType point(transition, FCFW::PointType::kReference, typename Traits::ValueType{}, 
                                      offset, reference, isOffsetRelative);
                        path->AddPoint(point);
                    } else {
                        log::warn("{}: Failed to resolve reference at time {}, using offset as absolute value", __FUNCTION__, time);
                        PointType point(transition, FCFW::PointType::kWorld, offset, typename Traits::ValueType{});
                        path->AddPoint(point);
                    }
                }
            }
            
            return true;
            
        } catch (const YAML::Exception& e) {
            log::error("{}: YAML parsing error: {}", __FUNCTION__, e.what());
            return false;
        } catch (const std::exception& e) {
            log::error("{}: Error loading YAML file: {}", __FUNCTION__, e.what());
            return false;
        }
    }
    
    // Template helper for exporting points to YAML
    template<typename PointType, typename PathType>
    bool ExportPathToYAML(const PathType* path, std::ofstream& a_file, float a_conversionFactor) {
        using Traits = PointTraits<PointType>;
        
        if (!a_file.is_open()) {
            log::error("{}: File is not open", __FUNCTION__);
            return false;
        }

        try {
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << Traits::SectionName;
            out << YAML::Value << YAML::BeginSeq;
            
            for (const auto& point : path->m_points) {
                out << YAML::BeginMap;
                out << YAML::Key << "time" << YAML::Value << point.m_transition.m_time;
                out << YAML::Key << "type" << YAML::Value << PointTypeToString(point.m_pointType);
                
                // Write value/offset based on point type
                if (point.m_pointType == FCFW::PointType::kWorld) {
                    out << YAML::Key << Traits::ValueKey << YAML::Value;
                    Traits::WriteValue(out, point.m_point, a_conversionFactor);
                } else {
                    out << YAML::Key << "offset" << YAML::Value;
                    Traits::WriteValue(out, point.m_offset, a_conversionFactor);
                }
                
                // Write reference info if applicable
                if (point.m_pointType == FCFW::PointType::kReference && point.m_reference) {
                    out << YAML::Key << "reference" << YAML::Value << YAML::BeginMap;
                    
                    const char* editorID = point.m_reference->GetFormEditorID();
                    if (editorID && editorID[0] != '\0') {
                        out << YAML::Key << "editorID" << YAML::Value << editorID;
                    } else {
                        log::warn("{}: Reference 0x{:X} has no EditorID - timeline may not be portable across load orders", 
                                 __FUNCTION__, point.m_reference->GetFormID());
                    }
                    
                    auto* file = point.m_reference->GetFile(0);
                    if (file) {
                        out << YAML::Key << "plugin" << YAML::Value << file->fileName;
                    } else {
                        log::warn("{}: Reference 0x{:X} has no associated plugin file", 
                                 __FUNCTION__, point.m_reference->GetFormID());
                    }
                    
                    out << YAML::Key << "formID" << YAML::Value << fmt::format("0x{:X}", point.m_reference->GetFormID());
                    out << YAML::EndMap;
                    
                    out << YAML::Key << "isOffsetRelative" << YAML::Value << (point.m_isOffsetRelative ? true : false);
                }
                
                out << YAML::Key << "interpolationMode" << YAML::Value << InterpolationModeToString(point.m_transition.m_mode);
                out << YAML::Key << "easeIn" << YAML::Value << (point.m_transition.m_easeIn ? true : false);
                out << YAML::Key << "easeOut" << YAML::Value << (point.m_transition.m_easeOut ? true : false);
                
                out << YAML::EndMap;
            }
            
            out << YAML::EndSeq;
            out << YAML::EndMap;
            
            // Extract section and write to stream
            std::string yamlStr = out.c_str();
            size_t startPos = yamlStr.find(Traits::SectionName);
            if (startPos != std::string::npos) {
                a_file << yamlStr.substr(startPos);
            } else {
                a_file << yamlStr;
            }
            
            return true;
            
        } catch (const std::exception& e) {
            log::error("{}: Error exporting YAML: {}", __FUNCTION__, e.what());
            return false;
        }
    }
    
    // ===== TranslationPath implementations =====

    TranslationPoint TranslationPath::GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const {
        Transition transition(a_time, InterpolationMode::kCubicHermite, a_easeIn, a_easeOut);
        return TranslationPoint(transition, PointType::kCamera);
    }

    // ===== RotationPath implementations =====

    RotationPoint RotationPath::GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const {
        Transition transition(a_time, InterpolationMode::kCubicHermite, a_easeIn, a_easeOut);
        return RotationPoint(transition, PointType::kCamera);
    }

    // ===== TranslationPath YAML implementations =====
    
    bool TranslationPath::AddPathFromFile(const std::string& a_filePath, float a_timeOffset, float a_conversionFactor) {
        return ImportPathFromYAML<TranslationPoint>(this, a_filePath, a_timeOffset, a_conversionFactor);
    }

    bool TranslationPath::ExportPath(std::ofstream& a_file, float a_conversionFactor) const {
        return ExportPathToYAML<TranslationPoint>(this, a_file, a_conversionFactor);
    }

    // ===== RotationPath YAML implementations =====
    
    bool RotationPath::AddPathFromFile(const std::string& a_filePath, float a_timeOffset, float a_conversionFactor) {
        return ImportPathFromYAML<RotationPoint>(this, a_filePath, a_timeOffset, a_conversionFactor);
    }

    bool RotationPath::ExportPath(std::ofstream& a_file, float a_conversionFactor) const {
        return ExportPathToYAML<RotationPoint>(this, a_file, a_conversionFactor);
    }

} // namespace FCFW
