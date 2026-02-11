#pragma once

namespace Hooks
{
	class MainUpdateHook
	{
	public:
		static void Hook()
		{
			auto& trampoline = SKSE::GetTrampoline();
			REL::Relocation<uintptr_t> hook{ RELOCATION_ID(35565, 36564) };  // 5B2FF0, 5D9F50, main update
			
			_Nullsub = trampoline.write_call<5>(hook.address() + RELOCATION_OFFSET(0x748, 0xC26), Nullsub);  // 5B3738, 5DAB76
		}

	private:
		static void Nullsub();
		static inline REL::Relocation<decltype(Nullsub)> _Nullsub;		
		static void LogPlayerCellInfo();
	};

	class LookHook
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> LookHandlerVtbl{ RE::VTABLE_LookHandler[0] };
			_ProcessThumbstick = LookHandlerVtbl.write_vfunc(0x2, ProcessThumbstick);
			_ProcessMouseMove = LookHandlerVtbl.write_vfunc(0x3, ProcessMouseMove);
		}

	private:
		static void ProcessThumbstick(RE::LookHandler* a_this, RE::ThumbstickEvent* a_event, RE::PlayerControlsData* a_data);
		static void ProcessMouseMove(RE::LookHandler* a_this, RE::MouseMoveEvent* a_event, RE::PlayerControlsData* a_data);

		static inline REL::Relocation<decltype(ProcessThumbstick)> _ProcessThumbstick;
		static inline REL::Relocation<decltype(ProcessMouseMove)> _ProcessMouseMove;
	};
	
	class MovementHook
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> MovementHandlerVtbl{ RE::VTABLE_MovementHandler[0] };
			_ProcessThumbstick = MovementHandlerVtbl.write_vfunc(0x2, ProcessThumbstick);
			_ProcessButton = MovementHandlerVtbl.write_vfunc(0x4, ProcessButton);
		}

	private:
		static void ProcessThumbstick(RE::MovementHandler* a_this, RE::ThumbstickEvent* a_event, RE::PlayerControlsData* a_data);
		static void ProcessButton(RE::MovementHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data);

		static inline REL::Relocation<decltype(ProcessThumbstick)> _ProcessThumbstick;
		static inline REL::Relocation<decltype(ProcessButton)> _ProcessButton;
	};

	
	class FreeCameraStateHook
	{
	public:
		static void Hook()
		{
			REL::Relocation<std::uintptr_t> FreeCameraStateVtbl{ RE::VTABLE_FreeCameraState[0] };
			_Update = FreeCameraStateVtbl.write_vfunc(0x3, Update);
		}

	private:
		static void Update(RE::FreeCameraState* a_this);
		
		static inline REL::Relocation<decltype(Update)> _Update;
	};

	class ToggleFreeCameraHook
	{
	public:
		static void Hook();

		static void HandleDeferredFreeCameraToggle();
		
		static inline std::uintptr_t _ToggleFreeCamera{ 0 };
		
	private:
		static void ToggleFreeCamera(RE::PlayerCamera* a_this, bool a_freezeTime);
		static inline bool m_reEnterFreeCamera;
	};

    class GridCellArrayHook
    {
    public:
        static void Hook()
        {
            REL::Relocation<std::uintptr_t> GridCellArrayVtbl{ RE::VTABLE_GridCellArray[0] };
            _SetCenter = GridCellArrayVtbl.write_vfunc(0x3, SetCenter);
        }

    private:
        static bool SetCenter(RE::GridCellArray* a_this, std::int32_t a_x, std::int32_t a_y);
        static inline REL::Relocation<decltype(SetCenter)> _SetCenter;
    };


	void Install();
} // namespace Hooks	

