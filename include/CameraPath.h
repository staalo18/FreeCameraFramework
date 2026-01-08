#pragma once

#include "CameraTypes.h"
#include "_ts_SKSEFunctions.h"
#include "FCFW_Utils.h"
#include <stdexcept>


namespace FCFW {
    class TranslationPoint {
    public:
        TranslationPoint(
            const Transition& a_transition = Transition(0.0f, InterpolationMode::kCubicHermite, false, false),
            PointType a_pointType = PointType::kWorld,
            const RE::NiPoint3& a_point = {0.f, 0.f, 0.f},
            const RE::NiPoint3& a_offset = {0.f, 0.f, 0.f},
            RE::TESObjectREFR* a_reference = nullptr,
            bool a_isOffsetRelative = false)
            : m_transition(a_transition)
            , m_point(a_point)           // world coordinates (kWorld and kCamera(baked))
            , m_pointType(a_pointType)   // kWorld, kReference, kCamera
            , m_reference(a_reference)   // used for kReference type
            , m_offset(a_offset)         // offset for kReference and kCamera types
            , m_isOffsetRelative(a_isOffsetRelative) {} // unused for kWorld type

        RE::NiPoint3 GetPointAtCamera() const {
            RE::NiPoint3 cameraPos = _ts_SKSEFunctions::GetCameraPos();
            return cameraPos + m_offset;
        }

        RE::NiPoint3 GetPoint() const {
            if (m_pointType == PointType::kReference && m_reference) {
                RE::NiPoint3 offset = m_offset;
                
                // If offset is relative to reference heading, rotate it
                if (m_isOffsetRelative) {
                    float pitch = 0.0f;
                    float yaw = 0.0f;
                    RE::Actor* actor = m_reference->As<RE::Actor>();
                    if (actor) {
                        yaw = actor->GetHeading(false);
                    } else {
                        pitch = m_reference->GetAngleX();
                        yaw = m_reference->GetAngleZ();
                    }
                    
                    // Rotate offset by reference's orientation
                    // First rotate around Z axis (yaw), then around X axis (pitch)
                    // Original offset is in reference's local space (forward=Y, right=X, up=Z)
                    
                    // Yaw rotation (around Z axis)
                    float cosYaw = std::cos(yaw);
                    float sinYaw = std::sin(yaw);
                    RE::NiPoint3 yawRotated;
                    yawRotated.x = m_offset.y * sinYaw + m_offset.x * cosYaw;
                    yawRotated.y = m_offset.y * cosYaw - m_offset.x * sinYaw;
                    yawRotated.z = m_offset.z;
                    
                    // Pitch rotation (around X axis)
                    // Pitch rotates in the Y-Z plane
                    float cosPitch = std::cos(pitch);
                    float sinPitch = std::sin(pitch);
                    RE::NiPoint3 rotatedOffset;
                    rotatedOffset.x = yawRotated.x;
                    rotatedOffset.y = yawRotated.z * sinPitch + yawRotated.y * cosPitch;
                    rotatedOffset.z = yawRotated.z * cosPitch - yawRotated.y * sinPitch;
                    
                    offset = rotatedOffset;
                }
                
                return m_reference->GetPosition() + offset;
            }
            return m_point;
        }
       
        bool IsNearlyEqual(const TranslationPoint& other) const {
            RE::NiPoint3 pos = GetPoint();
            RE::NiPoint3 otherPos = other.GetPoint();
            return std::abs(pos.x - otherPos.x) < EPSILON_COMPARISON &&
                   std::abs(pos.y - otherPos.y) < EPSILON_COMPARISON &&
                   std::abs(pos.z - otherPos.z) < EPSILON_COMPARISON;
        }
       
        TranslationPoint CubicHermite(const TranslationPoint& p0, const TranslationPoint& p1,
                                   const TranslationPoint& p2, const TranslationPoint& p3, float t) {
            TranslationPoint result;
            result.m_transition = m_transition;

            auto pt0 = p0.GetPoint();
            auto pt1 = p1.GetPoint();
            auto pt2 = p2.GetPoint();
            auto pt3 = p3.GetPoint();

            result.m_point.x = CubicHermiteInterpolate(pt0.x, pt1.x, pt2.x, pt3.x, t);
            result.m_point.y = CubicHermiteInterpolate(pt0.y, pt1.y, pt2.y, pt3.y, t);
            result.m_point.z = CubicHermiteInterpolate(pt0.z, pt1.z, pt2.z, pt3.z, t);

            return result;
        }
       
        TranslationPoint operator+(const TranslationPoint& other) const {
            return TranslationPoint(m_transition, PointType::kWorld, GetPoint() + other.GetPoint(), RE::NiPoint3{});
        }
        
        TranslationPoint operator-(const TranslationPoint& other) const {
            return TranslationPoint(m_transition, PointType::kWorld, GetPoint() - other.GetPoint(), RE::NiPoint3{});
        }
        
        TranslationPoint operator*(float scalar) const {
            return TranslationPoint(m_transition, PointType::kWorld, GetPoint() * scalar, RE::NiPoint3{});
        }

        Transition m_transition;
        mutable RE::NiPoint3 m_point;   // Direct position (used when m_pointType is kWorld), mutable for reference updates
        PointType m_pointType;          // Type of point: kWorld, kReference, or kCamera
        RE::TESObjectREFR* m_reference; // Reference object for dynamic positioning (kReference only)
        RE::NiPoint3 m_offset;          // Offset from reference position (kReference and kCamera)
        bool m_isOffsetRelative;        // If true, offset is rotated by reference's heading (kReference only)
    };

    class RotationPoint {
    public:
        RotationPoint(
            const Transition& a_transition = Transition(0.0f, InterpolationMode::kCubicHermite, false, false),
            PointType a_pointType = PointType::kWorld,
            const RE::BSTPoint2<float>& a_point = {0.f, 0.f},
            const RE::BSTPoint2<float>& a_offset = {0.f, 0.f},
            RE::TESObjectREFR* a_reference = nullptr,
            bool a_isOffsetRelative = false)
            : m_transition(a_transition)
            , m_point(a_point)           // rotation (kWorld and kCamera(baked))
            , m_pointType(a_pointType)   // kWorld, kReference, kCamera
            , m_reference(a_reference)   // used for kReference type
            , m_offset(a_offset)         // offset for kReference and kCamera types
            , m_isOffsetRelative(a_isOffsetRelative) {} // unused for kWorld type

        RE::BSTPoint2<float> GetPointAtCamera() const {
            auto rotation = _ts_SKSEFunctions::GetCameraRotation();
            return RE::BSTPoint2<float>{rotation.x + m_offset.x, rotation.z + m_offset.y};  // Pitch + offsetPitch, Yaw + offsetYaw
        }

        RE::BSTPoint2<float> GetPoint() const {
            if (m_pointType == PointType::kReference && m_reference) {
                if (m_isOffsetRelative) { // If offset is relative to reference heading, use reference's facing direction
                    float pitch = 0.0f;
                    float yaw = 0.0f;
                    RE::Actor* actor = m_reference->As<RE::Actor>();
                    if (actor) {
                        yaw = actor->GetHeading(false);
                    } else {
                        pitch = m_reference->GetAngleX();
                        yaw = m_reference->GetAngleZ();
                    }
                    
                    // Apply offset to reference's base orientation
                    return RE::BSTPoint2<float>{
                        _ts_SKSEFunctions::NormalRelativeAngle(pitch + m_offset.x),
                        _ts_SKSEFunctions::NormalRelativeAngle(yaw + m_offset.y)};
                } else { // camera looks at reference with offset
                    RE::NiPoint3 refPos = m_reference->GetPosition();                
                    RE::NiPoint3 cameraPos = _ts_SKSEFunctions::GetCameraPos();
                    
                    RE::NiPoint3 toRef = refPos - cameraPos;
                    float distance = toRef.Length();
                    
                    if (distance < 0.001f) {
                        // Camera too close to reference, can't determine direction
                        return m_offset;
                    }
                    
                    toRef = toRef / distance; // Normalize to get direction vector
                    
                    // Calculate base pitch and yaw from camera-to-ref direction
                    float basePitch = -std::asin(toRef.z);
                    float baseYaw = std::atan2(toRef.x, toRef.y);
                                        
                    // If offsets are zero (or very small), just use base direction
                    if (std::abs(m_offset.x) <EPSILON_COMPARISON && std::abs(m_offset.y) < EPSILON_COMPARISON) {
                        return RE::BSTPoint2<float>{
                            _ts_SKSEFunctions::NormalRelativeAngle(basePitch),
                            _ts_SKSEFunctions::NormalRelativeAngle(baseYaw)};
                    }
                    
                    // Now apply the offsets to this base direction
                    // Convert offset to direction in local frame (where base direction is forward)
                    float cosPitchLocal = std::cos(m_offset.x);
                    float sinPitchLocal = std::sin(m_offset.x);
                    float cosYawLocal = std::cos(m_offset.y);
                    float sinYawLocal = std::sin(m_offset.y);
                    
                    // Direction in local frame (forward along base direction)
                    RE::NiPoint3 localDir;
                    localDir.x = sinYawLocal * cosPitchLocal;  // right component
                    localDir.y = cosYawLocal * cosPitchLocal;  // forward component
                    localDir.z = sinPitchLocal;                // up component
                    
                    // Build a coordinate frame where toRef is the forward (+Y) direction
                    // We need to find an orthonormal basis: right, forward, up
                    RE::NiPoint3 forward = toRef;
                    
                    // Choose an arbitrary "up" vector that's not parallel to forward
                    RE::NiPoint3 worldUp(0.0f, 0.0f, 1.0f);
                    if (std::abs(forward.z) > 0.99f) {
                        // Forward is nearly vertical, use Y axis as reference
                        worldUp = RE::NiPoint3(0.0f, 1.0f, 0.0f);
                    }
                    
                    // Right = forward × worldUp (cross product)
                    RE::NiPoint3 right;
                    right.x = forward.y * worldUp.z - forward.z * worldUp.y;
                    right.y = forward.z * worldUp.x - forward.x * worldUp.z;
                    right.z = forward.x * worldUp.y - forward.y * worldUp.x;
                    float rightLen = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
                    right = right / rightLen;
                    
                    // Up = right × forward
                    RE::NiPoint3 up;
                    up.x = right.y * forward.z - right.z * forward.y;
                    up.y = right.z * forward.x - right.x * forward.z;
                    up.z = right.x * forward.y - right.y * forward.x;
                    
                    // Transform localDir from local frame to world frame using the basis vectors
                    RE::NiPoint3 worldDir;
                    worldDir.x = localDir.x * right.x + localDir.y * forward.x + localDir.z * up.x;
                    worldDir.y = localDir.x * right.y + localDir.y * forward.y + localDir.z * up.y;
                    worldDir.z = localDir.x * right.z + localDir.y * forward.z + localDir.z * up.z;
                    
                    // Convert world direction back to pitch/yaw angles
                    float worldPitch = -std::asin(worldDir.z);
                    float worldYaw = std::atan2(worldDir.x, worldDir.y);
                    
                    return RE::BSTPoint2<float>{
                        _ts_SKSEFunctions::NormalRelativeAngle(worldPitch),
                        _ts_SKSEFunctions::NormalRelativeAngle(worldYaw)};
                }
            }
            return m_point;
        }
       
        bool IsNearlyEqual(const RotationPoint& other) const {
            RE::BSTPoint2<float> rot = GetPoint();
            RE::BSTPoint2<float> otherRot = other.GetPoint();
            return std::abs(rot.x - otherRot.x) < EPSILON_COMPARISON &&
                   std::abs(rot.y - otherRot.y) < EPSILON_COMPARISON;
        }

        RotationPoint CubicHermite(const RotationPoint& p0, const RotationPoint& p1,
                                   const RotationPoint& p2, const RotationPoint& p3, float t) {
            RotationPoint result;
            result.m_transition = m_transition;

            auto pt0 = p0.GetPoint();
            auto pt1 = p1.GetPoint();
            auto pt2 = p2.GetPoint();
            auto pt3 = p3.GetPoint();

            result.m_point.x = CubicHermiteInterpolateAngular(pt0.x, pt1.x, pt2.x, pt3.x, t);
            result.m_point.y = CubicHermiteInterpolateAngular(pt0.y, pt1.y, pt2.y, pt3.y, t);

            return result;
        }

        // Raw arithmetic operators - DO NOT wrap (needed for unwrapped space calculations)
        RotationPoint operator+(const RotationPoint& other) const {
            RE::BSTPoint2<float> result;
            result.x = GetPoint().x + other.GetPoint().x;
            result.y = GetPoint().y + other.GetPoint().y;
            return RotationPoint(m_transition, PointType::kWorld, result, RE::BSTPoint2<float>{});
        }
        
        RotationPoint operator-(const RotationPoint& other) const {
            RE::BSTPoint2<float> result;
            result.x = GetPoint().x - other.GetPoint().x;
            result.y = GetPoint().y - other.GetPoint().y;
            return RotationPoint(m_transition, PointType::kWorld, result, RE::BSTPoint2<float>{});
        }
        
        RotationPoint operator*(float scalar) const {
            RE::BSTPoint2<float> result;
            result.x = GetPoint().x * scalar;
            result.y = GetPoint().y * scalar;
            return RotationPoint(m_transition, PointType::kWorld, result, RE::BSTPoint2<float>{});
        }
       
        Transition m_transition;
        mutable RE::BSTPoint2<float> m_point;  // Direct rotation (used when m_pointType is kWorld), mutable for reference updates
        PointType m_pointType;                 // Type of point: kWorld, kReference, or kCamera
        RE::TESObjectREFR* m_reference;        // Reference object for dynamic rotation (kReference only)
        RE::BSTPoint2<float> m_offset;         // Offset from camera-to-reference direction (kReference and kCamera)
        bool m_isOffsetRelative;               // If true, offset is relative to reference's facing direction (kReference only)
    };

    template<typename TransitionPoint>
    class CameraPath {
    public:
        // Friend declarations for YAML template helpers
        template<typename PointType, typename PathType>
        friend bool ImportPathFromYAML(PathType* path, const std::string& a_filePath, float a_timeOffset, float a_conversionFactor);
        
        template<typename PointType, typename PathType>
        friend bool ExportPathToYAML(const PathType* path, std::ofstream& a_file, float a_conversionFactor);
        
        virtual ~CameraPath() = default;
                
        size_t AddPoint(const TransitionPoint& a_point) {
            TransitionPoint modifiedPoint = a_point;
            if (modifiedPoint.m_transition.m_time < 0.0f) {
                modifiedPoint.m_transition.m_time = 0.0f;
            } 

            // Insert point in sorted order by time
            auto insertPos = std::lower_bound(m_points.begin(), m_points.end(), modifiedPoint.m_transition.m_time,
                [](const TransitionPoint& point, float time) {
                    return point.m_transition.m_time < time;
                });
            
            auto it = m_points.insert(insertPos, modifiedPoint);
            return std::distance(m_points.begin(), it);
        }
        
        const TransitionPoint& GetPoint(size_t a_index) const {
            if (a_index >= m_points.size()) {
                log::error("{}: index out of range", __FUNCTION__);
                throw std::out_of_range("CameraPath::GetPoint: index out of range");
            }
            
            // Update cached m_point from reference if this is a reference-based point
            if (m_points[a_index].m_pointType == PointType::kReference && m_points[a_index].m_reference) {
                m_points[a_index].m_point = m_points[a_index].GetPoint();
            }
            
            return m_points[a_index];
        }
        
        void RemovePoint(size_t a_index) {
            if (a_index < m_points.size()) {
                m_points.erase(m_points.begin() + a_index);
            }
        }
        
        void ClearPath() {
            m_points.clear();
        }
        
        size_t GetPointCount() const { return m_points.size(); }

        // Update all camera points - store current camera value into m_point
        void UpdateCameraPoints() {
            for (auto& point : m_points) {
                if (point.m_pointType == PointType::kCamera) {
                    point.m_point = point.GetPointAtCamera();
                }
            }
        }

        virtual TransitionPoint GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const = 0;
        
    protected:
        std::vector<TransitionPoint> m_points;
    };

    class TranslationPath : public CameraPath<TranslationPoint> {
    public:
        using TransitionPoint = TranslationPoint;
        using ValueType = RE::NiPoint3;  // Type returned by GetPoint()
        
        TranslationPoint GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const override;
        
        bool AddPathFromFile(const std::string& a_filePath, float a_timeOffset = 0.0f, float a_conversionFactor = 1.0f);
        bool ExportPath(std::ofstream& a_file, float a_conversionFactor = 1.0f) const;
    };

    class RotationPath : public CameraPath<RotationPoint> {
    public:
        using TransitionPoint = RotationPoint;
        using ValueType = RE::BSTPoint2<float>;  // Type returned by GetPoint()
        
        RotationPoint GetPointAtCamera(float a_time, bool a_easeIn, bool a_easeOut) const override;
        
        bool AddPathFromFile(const std::string& a_filePath, float a_timeOffset = 0.0f, float a_conversionFactor = 1.0f);
        bool ExportPath(std::ofstream& a_file, float a_conversionFactor = 1.0f) const;
    };

} // namespace FCFW