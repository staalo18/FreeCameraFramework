#pragma once

namespace FCFW {
    enum class InterpolationMode {
        kNone,
        kLinear,
        kCubicHermite
    };
    
    enum class TimelineType {
        kTranslation,
        kRotation
    };

    enum struct PointType {
        kWorld = 0,      // Static world point
        kReference = 1,  // Dynamic reference-based point
        kCamera = 2      // Static camera-based point (initialized at StartPlayback)
    };

    enum class PlaybackMode : int {
        kEnd = 0,   // Stop at end of timeline (default)
        kLoop = 1,  // Restart from beginning when timeline completes
        kWait = 2   // Stay at final position indefinitely (requires manual StopPlayback)
    };

    enum class BodyPart : int {
        kNone = 0,   // Use actor root position (GetPosition())
        kHead = 1,   // Head position
        kTorso = 2   // Torso position
    };

    
    struct Transition {
        float m_time;                           // Absolute time at which this point is reached (0 = start of playback)
        InterpolationMode m_mode;               // Interpolation mode for this segment
        bool m_easeIn;                          // Ease in (accelerate from 0 at initial point)
        bool m_easeOut;                         // Ease out (decelerate to 0 at target point)
        
        Transition() 
            : m_time(0.0f), m_mode(InterpolationMode::kCubicHermite), m_easeIn(false), m_easeOut(false) {}
        
        Transition(float a_time, InterpolationMode a_mode, bool a_easeIn, bool a_easeOut) 
            : m_time(a_time), m_mode(a_mode), m_easeIn(a_easeIn), m_easeOut(a_easeOut) {}
    };

    inline InterpolationMode ToInterpolationMode(int a_mode) {
        if (a_mode < 0 || a_mode > static_cast<int>(InterpolationMode::kCubicHermite)) {
            log::warn("Invalid interpolation mode {} passed, defaulting to kNone", a_mode);
            return InterpolationMode::kNone;
        }
        return static_cast<InterpolationMode>(a_mode);
    }

    inline PointType ToPointType(int a_mode) {
        if (a_mode < 0 || a_mode > static_cast<int>(PointType::kCamera)) {
            log::warn("Invalid point type {} passed, defaulting to kWorld", a_mode);
            return PointType::kWorld;
        }
        return static_cast<PointType>(a_mode);
    }

    inline BodyPart ToBodyPart(int a_bodyPart) {
        if (a_bodyPart < 0 || a_bodyPart > static_cast<int>(BodyPart::kTorso)) {
            log::warn("Invalid body part {} passed, defaulting to kNone", a_bodyPart);
            return BodyPart::kNone;
        }
        return static_cast<BodyPart>(a_bodyPart);
    }

    inline RE::BGSBodyPartDefs::LIMB_ENUM BodyPartToLimbEnum(BodyPart a_bodyPart) {
        switch (a_bodyPart) {
            case BodyPart::kHead: return RE::BGSBodyPartDefs::LIMB_ENUM::kHead;
            case BodyPart::kTorso: return RE::BGSBodyPartDefs::LIMB_ENUM::kTorso;
            default: return RE::BGSBodyPartDefs::LIMB_ENUM::kTorso;
        }
    }

} // namespace FCFW
