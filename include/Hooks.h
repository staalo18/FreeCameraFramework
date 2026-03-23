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


    class FreeCameraRollHook
    {
    public:
        static void Hook()
        {
            // Hook FromEulerAnglesZXY called during FreeCamera::GetRotation
            REL::Relocation<std::uintptr_t> getRot{ RELOCATION_ID(49814, 50744), 0x1B };
            auto& trampoline = SKSE::GetTrampoline();
            _FromEulerAnglesZXY = trampoline.write_call<5>(getRot.address(), FromEulerAnglesZXY);
        }

        // Set custom roll value
        static void SetFreeCameraRoll(float a_roll) { m_cameraRoll = a_roll; }
        static float GetFreeCameraRoll() { return m_cameraRoll; }

    private:
        static void FromEulerAnglesZXY(RE::NiMatrix3* a_matrix, float a_z, float a_x, float a_y);
        static inline REL::Relocation<decltype(FromEulerAnglesZXY)> _FromEulerAnglesZXY;
        static inline float m_cameraRoll{ 0.0f };
    };
	
	
	class UpdateLODHook
	{
	public:
		static void Hook();
		static inline std::uintptr_t _UpdateLOD{ 0 };

	private:
		static void UpdateLOD(RE::BGSTerrainManager* a_this, RE::NiPoint3* a_lodOrigin, std::uint32_t* a_flags);
	};

	
	class PlayerGhostHook
	{
	public:
		static void Hook();
		static void SetPlayerGhost(bool a_ghost) { m_playerIsGhost = a_ghost; }
		static inline std::uintptr_t _IsGhost{ 0 };
	private:
		static bool IsGhost(RE::Actor* a_this);
		static inline bool m_playerIsGhost{ false };
	};


	class CalculateDetectionHook
	{	public:
		static void Hook();
		static void DisablePlayerDetection(bool a_disable) { m_disablePlayerDetection = a_disable; }
		static inline std::uintptr_t _CalculateDetection{ 0 };
	private:
		static void CalculateDetection(RE::Actor* a_this, 
										RE::Actor* a_other,
										std::int32_t* a_detectionValue, // [OUT] result written to DetectionState
										std::uint8_t* a_detectedFlag,   // [OUT] bool: was detected?
										std::uint8_t* a_stealthFlag,    // [OUT] stealth-point related
										std::int32_t* a_stealthMult,    // [OUT] stealth multiplier
										RE::NiPoint3* a_targetPos,      // [OUT] target's position
										std::int32_t* a_unk1,
										std::int32_t* a_unk2,
										std::int32_t* a_unk3);
		static inline bool m_disablePlayerDetection{ false };
	};


/* Below: Unused hooks */
/***********************/

	class PlayerSetParentCellHook
	{
	public:
		static void Hook();
		static void SetPlaybackInProgress(bool a_playbackInProgress) { m_playbackInProgress = a_playbackInProgress; }
		static inline std::uintptr_t _SetParentCell{ 0 };
	private:
		static void SetParentCell(RE::PlayerCharacter* a_this,
									RE::TESObjectCELL*   a_newCell,
									std::uint64_t        a_p3,
									std::uint64_t        a_p4);
		static inline bool m_playbackInProgress{ false };		
	};


	class StreamingQueueHook
	{
	public:
		static void Hook();
		static void SetForceResetStreamingQueue(bool a_forceReset) { m_forceResetStreamingQueue = a_forceReset; }
		static inline std::uintptr_t _StreamingQueuePendingCount{ 0 };
	private:
		static int StreamingQueuePendingCount();
		static void* GetQueue();

		static inline void* m_streamingQueue = nullptr;
		static inline bool m_forceResetStreamingQueue{ false };
	};

	// DAT_141ebeb52 — streaming-idle override flag.
	// When non-zero, TES::UpdateCellGrid treats the streaming queue as always empty
	// (streamingIdle = true), so every call commits the grid immediately.
	class StreamingIdleOverride
	{
	public:
		static bool Get()
		{
			static REL::Relocation<std::uint8_t*> flag{ RELOCATION_ID(514186, 400336) };
			return *flag != 0;
		}
		static void Set(bool a_enable)
		{
			static REL::Relocation<std::uint8_t*> flag{ RELOCATION_ID(514186, 400336) };
			*flag = a_enable ? 1 : 0;
		}
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

    
	class UpdateCellGridHook
    {
    public:
        static void Hook();

        static void SetBlockUpdates(bool a_block) { m_blockUpdates = a_block; }
        static inline std::uintptr_t _UpdateCellGrid{ 0 };

    private:
        static bool UpdateCellGrid(RE::TES* a_this, RE::NiPoint3* a_position, bool a_smoothHavok);
        static inline bool m_blockUpdates{ false };
    };


	class UpdateGridLandObjectsHook
    {
    public:
        static void Hook();

        // Call from StartPlayback/StopPlayback to block/unblock all external cell grid updates during playback
        static void SetBlockUpdates(bool a_block) { m_blockUpdates = a_block; }
        // Bypasses the hook and calls the original function directly (use from PlayTimeline)
        static bool UpdateGridLandObjectsNotHooked(RE::TES* a_this);

        static inline std::uintptr_t _UpdateGridLandObjects{ 0 };

    private:
        static bool UpdateGridLandObjects(RE::TES* a_this);
        static inline bool m_blockUpdates{ false };
    };


	class ActivateLoadedGridCellsHook
    {
    public:
        static void Hook();

        static inline std::uintptr_t _ActivateLoadedGridCells{ 0 };

    private:
        static void ActivateLoadedGridCells(RE::TES* a_this);
    };


	class UpdateCellLandstateHook
	{
	public:
		static void Hook();
       // Call from StartPlayback/StopPlayback to block/unblock all external cell grid updates during playback
        static void SetBlockUpdates(bool a_block) { m_blockUpdates = a_block; }
        // Bypasses the hook and calls the original function directly (use from PlayTimeline)
        static void UpdateCellLandstateNotHooked(RE::GridCellArray* a_this);

		static inline std::uintptr_t _UpdateCellLandstate{ 0 };

	private:
		static void UpdateCellLandstate(RE::GridCellArray* a_this);
        static inline bool m_blockUpdates{ false };
	};


	class ActivateCellRefs3DHook
	{
	public:
		static void Hook();

		static void SetBlockUpdates(bool a_block) { m_blockUpdates = a_block; }

		static inline std::uintptr_t _ActivateCellRefs3D{ 0 };

	private:
		static void ActivateCellRefs3D(RE::TESObjectCELL* a_this);
        static inline bool m_blockUpdates{ false };
	};

	class TerrainManagerIncrementalUpdateHook
	{
	public:
		static void Hook();

		static void SetBlockUpdates(bool a_block) { m_blockUpdates = a_block; }

		static void IncrementalUpdate();

		static inline std::uintptr_t _IncrementalUpdate{ 0 };

	private:
        static inline bool m_blockUpdates{ false };
	};
	
	
	class PostTeleportUpdateHook
	{
	public:
		static void Hook();

		static void SetBlockUpdates(bool a_block) { m_blockUpdates = a_block; }

		static inline std::uintptr_t _PostTeleportUpdate{ 0 };

	private:
		static void PostTeleportUpdate(RE::PlayerCharacter* a_this, float a_dt, bool a_skipAI);
        static inline bool m_blockUpdates{ false };
	};

	class RequestDetectionLevelHook
	{
	public:
		static void Hook();

		// When true, any actor trying to detect the player gets -1000 (undetectable)
		static void DisablePlayerDetection(bool a_disable) { m_disablePlayerDetection = a_disable; }

		static inline std::uintptr_t _RequestDetectionLevel{ 0 };

	private:
		static std::int32_t RequestDetectionLevel(RE::Actor* a_this, RE::Actor* a_target, RE::DETECTION_PRIORITY a_priority);
		static inline bool m_disablePlayerDetection{ false };
	};

	class IsHostileToPlayerHook
	{
	public:
		static void Hook();
		static void DisableHostileToPlayer(bool a_disable) { m_disableHostileToPlayer = a_disable; }
		static inline std::uintptr_t _IsHostileToActor{ 0 };
	private:
		static bool IsHostileToActor(RE::Actor* a_this, RE::Actor* a_other);
		static inline bool m_disableHostileToPlayer{ false };
	};

	class CheckValidTargetHook
	{
	public:
		static void Hook();
		static void DisableCheckValidTarget(bool a_disable) { m_disableCheckValidTarget = a_disable; }
		static inline std::uintptr_t _CheckValidTarget{ 0 };
	private:
		static bool CheckValidTarget(RE::Actor* a_this, RE::Actor* a_other);
		static inline bool m_disableCheckValidTarget{ false };
	};

	void Install();
} // namespace Hooks	

