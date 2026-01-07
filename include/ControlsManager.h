#pragma once

namespace FCFW {
    struct DXScanCode{
        uint32_t key;

        explicit DXScanCode(uint32_t a_key) : key(a_key) {
            if (a_key > InputMap::kMaxMacros) { // not a valid DXScanCode
                key = InputMap::kMaxMacros;
            }
        }

        // Default constructor
        DXScanCode() : key(InputMap::kMaxMacros) {}

        // Allow explicit conversion to uint32_t
        explicit operator uint32_t() const { return key; }

        DXScanCode& operator=(uint32_t a_newKey) {
            key = a_newKey;
            return *this;
        }
    
        bool operator==(const DXScanCode& a_other) const {
            return key == a_other.key;
        }
        bool operator==(uint32_t a_otherKey) const {
            return key == a_otherKey;
        }
    };
    
    class ControlsManager : public RE::BSTEventSink<RE::InputEvent*> {
    public:
        static ControlsManager& GetSingleton() {
            static ControlsManager instance;
            return instance;
        }
        ControlsManager(const ControlsManager&) = delete;
        ControlsManager& operator=(const ControlsManager&) = delete;

        RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override;

    private:
        ControlsManager() = default;
        ~ControlsManager() = default;
    }; // class ControlsManager
} // namespace FCFW
