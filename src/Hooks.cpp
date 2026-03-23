#include "Hooks.h"
#include "TimelineManager.h"
#include <_ts_SKSEFunctions.h>
#include <DbgHelp.h>

namespace Hooks
{
	void Install()
	{
		log::info("Hooking...");

		MainUpdateHook::Hook();
		LookHook::Hook();
		MovementHook::Hook();
		ToggleFreeCameraHook::Hook();
		FreeCameraRollHook::Hook();
		PlayerGhostHook::Hook();
		CalculateDetectionHook::Hook();
		UpdateLODHook::Hook();

//		PlayerSetParentCellHook::Hook();
//		StreamingQueueHook::Hook();
//		RequestDetectionLevelHook::Hook();
//		IsHostileToPlayerHook::Hook();
//		UpdateCellGridHook::Hook();

//		UpdateGridLandObjectsHook::Hook();
//		UpdateCellLandstateHook::Hook();

//		TerrainManagerIncrementalUpdateHook::Hook();
//		PostTeleportUpdateHook::Hook();
//		ActivateLoadedGridCellsHook::Hook();
//		ActivateCellRefs3DHook::Hook();
		
//      GridCellArrayHook::Hook();
		
		log::info("...success");
	}

	void MainUpdateHook::Nullsub()
	{
		_Nullsub();

		ToggleFreeCameraHook::HandleDeferredFreeCameraToggle();

		FCFW::TimelineManager::GetSingleton().Update();

//		LogPlayerCellInfo();
	}

	void MainUpdateHook::LogPlayerCellInfo()
	{
		auto playerPos = RE::PlayerCharacter::GetSingleton()->GetPosition();
log::info("{}: player pos: ({}, {}, {})", __FUNCTION__, playerPos.x, playerPos.y, playerPos.z);
		auto tes = RE::TES::GetSingleton();
		if (!tes || !tes->gridCells) {
log::info("{}: no gridcells", __FUNCTION__);
			return;
		}

		auto* targetCell = tes->GetCell(playerPos);
		if (!targetCell) {
log::info("{}: no targetCell", __FUNCTION__);
			return;
		}

		auto coords = targetCell->GetCoordinates();
		if (!coords) {
log::info("{}: no coords", __FUNCTION__);
			return;
		}
		log::info("{}: player coords: ({}, {}), currentGrid: ({}, {})", __FUNCTION__, 
		coords->cellX, coords->cellY, tes->currentGridX, tes->currentGridY);           
	}

	void LookHook::ProcessThumbstick(RE::LookHandler* a_this, RE::ThumbstickEvent* a_event, RE::PlayerControlsData* a_data)
	{
		auto& timelineManager = FCFW::TimelineManager::GetSingleton();

		if (!RE::UI::GetSingleton()->GameIsPaused()) {
			timelineManager.SetUserTurning(true);
			size_t activeID = timelineManager.GetActiveTimelineID();
			if (activeID != 0 && timelineManager.IsPlaybackRunning(activeID) && !timelineManager.IsUserRotationAllowed(activeID)) {
				return; // ignore look input during playback
			}
		}

		_ProcessThumbstick(a_this, a_event, a_data);
	}

	void LookHook::ProcessMouseMove(RE::LookHandler* a_this, RE::MouseMoveEvent* a_event, RE::PlayerControlsData* a_data)
	{
		auto& timelineManager = FCFW::TimelineManager::GetSingleton();

		if (!RE::UI::GetSingleton()->GameIsPaused()) {
			timelineManager.SetUserTurning(true);
			size_t activeID = timelineManager.GetActiveTimelineID();
			if (activeID != 0 && timelineManager.IsPlaybackRunning(activeID) && !timelineManager.IsUserRotationAllowed(activeID)) {
				return; // ignore look input during playback
			}
		}

		_ProcessMouseMove(a_this, a_event, a_data);
	}

	void MovementHook::ProcessThumbstick(RE::MovementHandler* a_this, RE::ThumbstickEvent* a_event, RE::PlayerControlsData* a_data)
	{
		auto& timelineManager = FCFW::TimelineManager::GetSingleton();
		size_t activeID = timelineManager.GetActiveTimelineID();
		if (a_event && a_event->IsLeft() && activeID != 0 && timelineManager.IsPlaybackRunning(activeID) && !RE::UI::GetSingleton()->GameIsPaused()) {
			return; // ignore movement input during playback
		}

		_ProcessThumbstick(a_this, a_event, a_data);
	}

	void MovementHook::ProcessButton(RE::MovementHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
	{
		bool bRelevant = false;
		if (a_event)
		{
			auto& userEvent = a_event->QUserEvent();
			auto userEvents = RE::UserEvents::GetSingleton();

			if (userEvent == userEvents->forward || 
				userEvent == userEvents->back ||
				userEvent == userEvents->strafeLeft ||
				userEvent == userEvents->strafeRight) {
				bRelevant = a_event->IsPressed();
			}
		}
		auto& timelineManager = FCFW::TimelineManager::GetSingleton();
		size_t activeID = timelineManager.GetActiveTimelineID();
		if (bRelevant && activeID != 0 && timelineManager.IsPlaybackRunning(activeID) && !RE::UI::GetSingleton()->GameIsPaused()) {
			return; // ignore movement input during playback
		}

		_ProcessButton(a_this, a_event, a_data);
	}

	void ToggleFreeCameraHook::Hook()
	{
		_ToggleFreeCamera = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(49876, 50809, 0),
			6,
			reinterpret_cast<std::uintptr_t>(ToggleFreeCamera)
		);
		
		// Initialize FCFW_Utils with trampoline address
		FCFW::InitializeFreeCameraTrampoline(_ToggleFreeCamera);
	}

	void ToggleFreeCameraHook::ToggleFreeCamera(RE::PlayerCamera* a_this, bool a_freezeTime)
	{
		// Always allow the toggle to complete to avoid state corruption:
		// Just blocking the original toggleFreeCamera call in case tfc is triggered via console
		// while a timeline is active would have unintended side effect that
		// camera zoom is locked (mouse wheel no longer works) after timeline playback ends.
		// Reason most likely is that console tfc has some post-toggle logic that runs
		// even if we block the original function.

		using FuncType = void(*)(RE::PlayerCamera*, bool);
		auto func = reinterpret_cast<FuncType>(_ToggleFreeCamera);
		func(a_this, a_freezeTime); // call the vanilla ToggleFreeCameraMode function
		
		// If we have an active timeline and just exited free camera via above func() call, 
		// schedule re-entry for next frame (see comment above).
		auto& timelineManager = FCFW::TimelineManager::GetSingleton();
		size_t activeID = timelineManager.GetActiveTimelineID();
		if (activeID > 0 && !a_this->IsInFreeCameraMode()) {
			m_reEnterFreeCamera = true;
		}
	}

	void ToggleFreeCameraHook::HandleDeferredFreeCameraToggle() {
		if (m_reEnterFreeCamera) {
			m_reEnterFreeCamera = false;
			auto* playerCamera = RE::PlayerCamera::GetSingleton();
			auto activeID = FCFW::TimelineManager::GetSingleton().GetActiveTimelineID();
			if (playerCamera && !playerCamera->IsInFreeCameraMode() && activeID > 0) {
				FCFW::ToggleFreeCameraNotHooked(false);
			}
		}
	}

	void FreeCameraRollHook::FromEulerAnglesZXY(RE::NiMatrix3* a_matrix, float a_z /*yaw*/, float a_x /*pitch*/, float /*roll*/)
	{
		// Replace roll parameter with custom value
		return _FromEulerAnglesZXY(a_matrix, a_z, a_x, m_cameraRoll);
	}

	void UpdateLODHook::Hook()
	{
		log::info("{}: Hooking BGSTerrainManager...", __FUNCTION__);

		_UpdateLOD = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(31017, 31802, 0),
			8,
			reinterpret_cast<std::uintptr_t>(UpdateLOD)
		);
	}


	void UpdateLODHook::UpdateLOD(RE::BGSTerrainManager* a_this, RE::NiPoint3* a_lodOrigin, std::uint32_t* a_flags)
	{
		if (_UpdateLOD == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}

		RE::NiPoint3 position; 
		if (a_lodOrigin) {
			position = *a_lodOrigin;
		}

		if (RE::PlayerCamera::GetSingleton()->IsInFreeCameraMode()) {
			// In free camera state, LOD starts flickering when the camera is moving into different cells than the player cell.
			// This is because in free camera state UpdateLOD is called twice per frame, from different tracks:
			//  - Once with flags=0 (full update)from PostTeleportUpdate, which forces a full LOD reload centered on the player cell
			//  - Once with flags=1 (incremental update) from UpdateCellGrid, which shifts LOD tiles based on camera movement. 
			// The former conflicts with the latter, causing random LOD flickering when camera and player are in different cells 
			// (whichever of the two calls comes later in the frame wins).
		
			// To fix this, set LOD origin to current camera position before passing to original UpdateLOD()

			position = _ts_SKSEFunctions::GetCameraPos();
		}
		
		// Calling original UpdateLOD
		using FuncType = void(*)(RE::BGSTerrainManager*, RE::NiPoint3*, std::uint32_t*);
		reinterpret_cast<FuncType>(_UpdateLOD)(a_this, &position, a_flags);
	}


	void PlayerGhostHook::Hook()
	{
		_IsGhost = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(36286, 37275, 0),
			5,
			reinterpret_cast<std::uintptr_t>(IsGhost)
		);
	}

	bool PlayerGhostHook::IsGhost(RE::Actor* a_this)
	{
		if (_IsGhost == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return false;
		}

		if (m_playerIsGhost && a_this == RE::PlayerCharacter::GetSingleton()) {
			return true;
		}

		using FuncType = bool(*)(RE::Actor*);
		return reinterpret_cast<FuncType>(_IsGhost)(a_this);
	}


	void CalculateDetectionHook::Hook()
	{
		_CalculateDetection = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(36758, 37774, 0),
			7,
			reinterpret_cast<std::uintptr_t>(CalculateDetection)
		);
	}


	void CalculateDetectionHook::CalculateDetection(RE::Actor* a_this, 
													RE::Actor* a_other,
													std::int32_t* a_detectionValue, // [OUT] result written to DetectionState
													std::uint8_t* a_detectedFlag,   // [OUT] bool: was detected?
													std::uint8_t* a_stealthFlag,    // [OUT] stealth-point related
													std::int32_t* a_stealthMult,    // [OUT] stealth multiplier
													RE::NiPoint3* a_targetPos,      // [OUT] target's position
													std::int32_t* a_unk1,
													std::int32_t* a_unk2,
													std::int32_t* a_unk3)
	{
		if (_CalculateDetection == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}

		if (m_disablePlayerDetection && a_other && a_other == RE::PlayerCharacter::GetSingleton()) {
            *a_detectionValue = -1000;  // kNotDetected
            return;
		}

		using FuncType = void(*)(RE::Actor*, RE::Actor*, std::int32_t*, std::uint8_t*, std::uint8_t*, std::int32_t*, RE::NiPoint3*, std::int32_t*, std::int32_t*, std::int32_t*);
		reinterpret_cast<FuncType>(_CalculateDetection)(a_this, a_other, a_detectionValue, a_detectedFlag, a_stealthFlag, a_stealthMult, a_targetPos, a_unk1, a_unk2, a_unk3);
	}

	void PlayerSetParentCellHook::Hook()
	{
		_SetParentCell = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(39391, 40466, 0),
			5,
			reinterpret_cast<std::uintptr_t>(SetParentCell)
		);
	}

	void PlayerSetParentCellHook::SetParentCell(RE::PlayerCharacter* a_this,
                      RE::TESObjectCELL*   a_newCell,
                      std::uint64_t        a_p3,
                      std::uint64_t        a_p4)
	{
		if (_SetParentCell == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
log::info("{}: PlayerSetParentCellHook called", __FUNCTION__);


		using FuncType = void(*)(RE::PlayerCharacter*, RE::TESObjectCELL*, std::uint64_t, std::uint64_t);
		reinterpret_cast<FuncType>(_SetParentCell)(a_this, a_newCell, a_p3, a_p4);
		
		if (m_playbackInProgress) {
log::info("{}: modifying SetParentCell", __FUNCTION__);
            *reinterpret_cast<std::uint8_t*>(
                reinterpret_cast<std::uintptr_t>(a_this) + 0xBDA) &= ~0x10u;
		}
	}

	void StreamingQueueHook::Hook() {
		_StreamingQueuePendingCount = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(18154, 18545, 0),
			5,
			reinterpret_cast<std::uintptr_t>(StreamingQueuePendingCount)
		);
	}

	int StreamingQueueHook::StreamingQueuePendingCount() {
		if (m_forceResetStreamingQueue) {
//log::info("{}: Force-resetting streaming queue pending count to 0", __FUNCTION__);
//			return 0;
		}

		auto* queue = GetQueue();
		if (!queue) {
			log::error("{}: streaming queue null", __FUNCTION__);
			return 0;
		}
		using FuncType = int(*)(void*);
		int result = reinterpret_cast<FuncType>(_StreamingQueuePendingCount)(queue);
log::info("{}: StreamingQueuePendingCount: {}", __FUNCTION__, result);
		return result;
	}

	void* StreamingQueueHook::GetQueue() {
		// Lazy-init: the pointer variable is not populated at plugin load time.
		// Read it on first call, during which the game has already initialized streaming.
		if (!m_streamingQueue) {
			static REL::Relocation<void**> ptrVar{ RELOCATION_ID(514741, 400899) };
			m_streamingQueue = *ptrVar;  // dereference: pointer var → queue struct address
		}
		return m_streamingQueue;
	}

/* Below: Unused hooks */
/***********************/


	bool GridCellArrayHook::SetCenter(RE::GridCellArray* a_this, std::int32_t a_x, std::int32_t a_y)
	{
		log::info("=== GridCellArray::SetCenter called: ({}, {}) ===", a_x, a_y);
	return _SetCenter(a_this, a_x, a_y);
/*
auto player = RE::PlayerCharacter::GetSingleton();
if (!player) {
    log::error("{}: PlayerCharacter not available", __FUNCTION__);
	return _SetCenter(a_this, a_x, a_y);
}

if (!player->GetParentCell()) {
	log::error("{}: Parent cell not available!!", __FUNCTION__);
}


if (!a_this) {
	log::error("{}: GridCellArray instance is null", __FUNCTION__);
	return _SetCenter(a_this, a_x, a_y);
}

//if (FCFW::TimelineManager::GetSingleton().IsCellGridUpdatePending()) {
//    log::info("{}: Resetting to initial pos", __FUNCTION__);  
	auto* cell = a_this->GetCell(a_x, a_y);
	if (!cell) {
		log::error("{}: Target cell not found for coords ({}, {})", __FUNCTION__, a_x, a_y);
//		return _SetCenter(a_this, a_x, a_y);
//	}
//	player->SetParentCell(cell);
//	FCFW::TimelineManager::GetSingleton().SetCellGridUpdatePending(false);
}

// Log GridCellArray state before SetCenter
log::info("{}: === GridCellArray State BEFORE SetCenter ===", __FUNCTION__);
log::info("{}: Grid length: {}", __FUNCTION__, a_this->length);
log::info("{}: World center: ({:.1f}, {:.1f}, {:.1f})", __FUNCTION__, 
         a_this->worldCenter.x, a_this->worldCenter.y, a_this->worldCenter.z);
log::info("{}: Land3D attached: {}", __FUNCTION__, a_this->land3DAttached);

// Log all cells in the grid
if (a_this->cells) {
	log::info("{}: Cells array contents:", __FUNCTION__);
	for (std::uint32_t x = 0; x < a_this->length; x++) {
		for (std::uint32_t y = 0; y < a_this->length; y++) {
			auto* gridCell = a_this->cells[(x * a_this->length) + y];
			if (gridCell) {
				auto* coords = gridCell->GetCoordinates();
				if (coords) {
					log::info("{}:   Grid[{},{}] -> Cell({},{}) [ptr=0x{:X}]", 
					         __FUNCTION__, x, y, coords->cellX, coords->cellY, 
					         reinterpret_cast<uintptr_t>(gridCell));
				} else {
					log::info("{}:   Grid[{},{}] -> Cell(no coords) [ptr=0x{:X}]", 
					         __FUNCTION__, x, y, reinterpret_cast<uintptr_t>(gridCell));
				}
			} else {
				log::info("{}:   Grid[{},{}] -> nullptr", __FUNCTION__, x, y);
			}
		}
	}
} else {
	log::error("{}: Cells array is NULL!", __FUNCTION__);
}

bool result = true;
if (FCFW::TimelineManager::GetSingleton().IsPlayerCellUpdateRequested()) {
	log::info("{}: Player cell update is requested - attempting to pre-populate grid before SetCenter", __FUNCTION__);
	_ts_SKSEFunctions::UpdateTESGridCells(a_x, a_y);
	FCFW::TimelineManager::GetSingleton().SetPlayerCellUpdateRequested(false);
}// else {
	result = _SetCenter(a_this, a_x, a_y);
//}

// Log GridCellArray state after SetCenter
log::info("{}: === GridCellArray State AFTER SetCenter ===", __FUNCTION__);
log::info("{}: World center: ({:.1f}, {:.1f}, {:.1f})", __FUNCTION__, 
         a_this->worldCenter.x, a_this->worldCenter.y, a_this->worldCenter.z);
if (a_this->cells) {
	log::info("{}: Cells array contents:", __FUNCTION__);
	for (std::uint32_t x = 0; x < a_this->length; x++) {
		for (std::uint32_t y = 0; y < a_this->length; y++) {
			auto* gridCell = a_this->cells[(x * a_this->length) + y];
			if (gridCell) {
				auto* coords = gridCell->GetCoordinates();
				if (coords) {
					log::info("{}:   Grid[{},{}] -> Cell({},{}) [ptr=0x{:X}]", 
					         __FUNCTION__, x, y, coords->cellX, coords->cellY, 
					         reinterpret_cast<uintptr_t>(gridCell));
				} else {
					log::info("{}:   Grid[{},{}] -> Cell(no coords) [ptr=0x{:X}]", 
					         __FUNCTION__, x, y, reinterpret_cast<uintptr_t>(gridCell));
				}
			} else {
				log::info("{}:   Grid[{},{}] -> nullptr", __FUNCTION__, x, y);
			}
		}
	}
}

if (!player->GetParentCell()) {
	log::error("{}: After SetCenter: Parent cell not available!!", __FUNCTION__);
}

auto* cell2 = a_this->GetCell(a_x, a_y);
if (!cell2) {
	log::error("{}: After SetCenter: Target cell not found for coords ({}, {})", __FUNCTION__, a_x, a_y);
}

return result;
/*
		// Capture call stack
		void* stack[64];
		WORD frames = CaptureStackBackTrace(1, 64, stack, nullptr); // Skip current frame
		
		// Get game base address for offset calculation
		auto base = REL::Module::get().base();
		
		// Get symbol information
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, nullptr, TRUE);
		
		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		SYMBOL_INFO* symbol = (SYMBOL_INFO*)buffer;
		symbol->MaxNameLen = MAX_SYM_NAME;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		
		log::info("Call stack (base: 0x{:X}):", base);
		for (WORD i = 0; i < frames && i < 15; i++) {  // First 15 frames
			DWORD64 address = (DWORD64)(stack[i]);
			uintptr_t offset = address - base;
			
			if (SymFromAddr(process, address, 0, symbol)) {
				log::info("  [{}] 0x{:X} (Base+0x{:X}) - {}", i, address, offset, symbol->Name);
			} else {
				log::info("  [{}] 0x{:X} (Base+0x{:X})", i, address, offset);
			}
		}
		
		SymCleanup(process);
//		return true;
		
		// Call original
		return _SetCenter(a_this, a_x, a_y); */
	}
/*

bool GridCellArrayHook::SetCenter(RE::GridCellArray* a_this, std::int32_t a_x, std::int32_t a_y)
{
    auto perfStart = std::chrono::high_resolution_clock::now();
    
    log::info("=== GridCellArray::SetCenter called: ({}, {}) ===", a_x, a_y);
    
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        log::error("{}: PlayerCharacter not available", __FUNCTION__);
        return _SetCenter(a_this, a_x, a_y);
    }
    
    auto* tes = RE::TES::GetSingleton();
    if (!tes) {
        log::error("{}: TES not available", __FUNCTION__);
        return _SetCenter(a_this, a_x, a_y);
    }
    
    // Log current grid center
    if (tes->gridCells) {
        log::info("{}: Current grid center: ({}, {})", __FUNCTION__, 
                 tes->currentGridX, tes->currentGridY);
    }
    
    auto* worldspace = tes->GetRuntimeData2().worldSpace;
    if (!worldspace) {
        log::error("{}: Worldspace not available", __FUNCTION__);
        return _SetCenter(a_this, a_x, a_y);
    }
    
    // Check if target cell is already loaded in grid
    auto* targetCell = a_this->GetCell(a_x, a_y);
    
    if (!targetCell) {
        log::warn("{}: Target cell ({}, {}) not in grid - attempting to pre-populate grid before SetCenter", 
                  __FUNCTION__, a_x, a_y);
        
        auto populateStart = std::chrono::high_resolution_clock::now();
        
        // Manually populate the cells array with loaded cells from worldspace->cellMap
        // This should make _SetCenter much faster since cells are already in the right places
        
        std::uint32_t gridSize = a_this->length;  // Should be 5 for 5x5 grid
        std::int32_t halfGrid = gridSize / 2;     // Should be 2
        
        log::info("{}: Grid size: {}, populating {}x{} cells around ({}, {})", 
                 __FUNCTION__, gridSize, gridSize, gridSize, a_x, a_y);
        
        int populatedCount = 0;
        int alreadyPresentCount = 0;
        
        const auto& cellMap = worldspace->cellMap;
        
        // Iterate through the 5x5 grid
        for (std::uint32_t gridX = 0; gridX < gridSize; gridX++) {
            for (std::uint32_t gridY = 0; gridY < gridSize; gridY++) {
                // Calculate world cell coordinates for this grid position
                std::int32_t worldCellX = a_x - halfGrid + gridX;
                std::int32_t worldCellY = a_y - halfGrid + gridY;
                
                // Look up cell in worldspace cellMap
                RE::CellID cellID(worldCellY, worldCellX);
                auto it = cellMap.find(cellID);
                
                if (it != cellMap.end()) {
                    RE::TESObjectCELL* cell = it->second;
                    std::uint32_t arrayIndex = (gridX * gridSize) + gridY;
                    
                    // Check if already in grid
                    if (a_this->cells[arrayIndex] != cell) {
                        // Write directly into the cells array
                        a_this->cells[arrayIndex] = cell;
                        populatedCount++;
                        
                        log::info("{}: Populated grid[{},{}] with cell ({},{}) at array index {}", 
                                 __FUNCTION__, gridX, gridY, worldCellX, worldCellY, arrayIndex);
                    } else {
                        alreadyPresentCount++;
                    }
                }
            }
        }
        
        auto populateEnd = std::chrono::high_resolution_clock::now();
        auto populateDuration = std::chrono::duration_cast<std::chrono::microseconds>(populateEnd - populateStart).count();
        
        log::info("{}: Pre-populated {} cells, {} already present, took {:.3f} ms", 
                 __FUNCTION__, populatedCount, alreadyPresentCount, populateDuration / 1000.0);
        
        // Note: GetCell() doesn't work until after SetCenter updates internal state
        // But the cells are in the array, which should make SetCenter much faster
    } else {
        log::info("{}: Target cell ({}, {}) already in grid", __FUNCTION__, a_x, a_y);
    }
    
    // Now call the original SetCenter
    auto setCenterStart = std::chrono::high_resolution_clock::now();
    bool result = _SetCenter(a_this, a_x, a_y);
    auto setCenterEnd = std::chrono::high_resolution_clock::now();
    auto setCenterDuration = std::chrono::duration_cast<std::chrono::microseconds>(setCenterEnd - setCenterStart).count();
    
    log::info("{}: _SetCenter took {:.3f} ms", __FUNCTION__, setCenterDuration / 1000.0);
    
    // Log grid center after SetCenter
    log::info("{}: After SetCenter, grid center: ({}, {})", __FUNCTION__, 
             tes->currentGridX, tes->currentGridY);
    
    // Verify the target cell is now accessible via GetCell
    auto* postSetCenterCell = a_this->GetCell(a_x, a_y);
    if (postSetCenterCell) {
        auto* coords = postSetCenterCell->GetCoordinates();
        if (coords) {
            log::info("{}: SUCCESS - After SetCenter, GetCell({},{}) returns cell ({},{})", 
                     __FUNCTION__, a_x, a_y, coords->cellX, coords->cellY);
        }
    } else {
        log::error("{}: FAILURE - After SetCenter, GetCell({},{}) still returns nullptr!", 
                 __FUNCTION__, a_x, a_y);
    }
    
    // Verify result
    if (!player->GetParentCell()) {
        log::error("{}: After SetCenter: Player parent cell is NULL!", __FUNCTION__);
    } else {
        auto* coords = player->GetParentCell()->GetCoordinates();
        if (coords) {
            log::info("{}: After SetCenter: Player cell is ({}, {})", 
                     __FUNCTION__, coords->cellX, coords->cellY);
        }
    }
    
    auto perfEnd = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::microseconds>(perfEnd - perfStart).count();
    log::info("{}: TOTAL SetCenter hook took {:.3f} ms", __FUNCTION__, totalDuration / 1000.0);
    
    return result;
}
*/

	void UpdateCellGridHook::Hook()
	{
log::info("{}: Hooking UpdateCellGrid...", __FUNCTION__);
		_UpdateCellGrid = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(13148, 13288, 0), // SE: 152030
			6,
			reinterpret_cast<std::uintptr_t>(UpdateCellGrid)
		);
	}

	bool UpdateCellGridHook::UpdateCellGrid(RE::TES* a_this, RE::NiPoint3* a_position, bool a_smoothHavok)
	{
		if (_UpdateCellGrid == 0) {
			log::error("{}: UpdateCellGrid trampoline not initialized!", __FUNCTION__);
			return false;
		}

log::info("{}: UpdateCellGrid called with position ({}, {}, {})", __FUNCTION__, 
a_position ? a_position->x : 0.0f, 
a_position ? a_position->y : 0.0f, 
a_position ? a_position->z : 0.0f);

		using FuncType = bool(*)(RE::TES*, RE::NiPoint3*, bool);
		auto func = reinterpret_cast<FuncType>(_UpdateCellGrid);

		if (m_blockUpdates) {
		}

		return func(a_this, a_position, a_smoothHavok);
	}


/*
	bool UpdateCellGridHook::UpdateCellGridNotHooked(RE::TES* a_this, RE::NiPoint3* a_position, bool a_smoothHavok)
	{
		if (_UpdateCellGrid == 0) {
			log::error("{}: UpdateCellGrid trampoline not initialized!", __FUNCTION__);
			return false;
		}

		using FuncType = bool(*)(RE::TES*, RE::NiPoint3*, bool);
		auto func = reinterpret_cast<FuncType>(_UpdateCellGrid);
		return func(a_this, a_position, a_smoothHavok);
	}

*/
	void UpdateGridLandObjectsHook::Hook()
	{
log::info("{}: Hooking UpdateGridLandObjects...", __FUNCTION__);
		_UpdateGridLandObjects = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(13263, 13414, 0), // SE: 15b0e0
//			REL::VariantID(15724, 15959, 0), // SE: 1d5250
			6,
			reinterpret_cast<std::uintptr_t>(UpdateGridLandObjects)
		);
	}

	bool UpdateGridLandObjectsHook::UpdateGridLandObjects(RE::TES* a_this)
	{
		if (_UpdateGridLandObjects == 0) {
			log::error("{}: UpdateGridLandObjects trampoline not initialized!", __FUNCTION__);
			return false;
		}

		if (m_blockUpdates) {
			return true;
		}
log::info("{}: Calling original UpdateGridLandObjects", __FUNCTION__);
		using FuncType = bool(*)(RE::TES*);
		auto func = reinterpret_cast<FuncType>(_UpdateGridLandObjects);
		return func(a_this);
	}

	bool UpdateGridLandObjectsHook::UpdateGridLandObjectsNotHooked(RE::TES* a_this)
	{
		if (_UpdateGridLandObjects == 0) {
			log::error("{}: UpdateGridLandObjects trampoline not initialized!", __FUNCTION__);
			return false;
		}

		using FuncType = bool(*)(RE::TES*);
		auto func = reinterpret_cast<FuncType>(_UpdateGridLandObjects);
		return func(a_this);
	}

	void ActivateLoadedGridCellsHook::Hook()
	{
log::info("{}: Hooking ActivateLoadedGridCells...", __FUNCTION__);
		_ActivateLoadedGridCells = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(13278, 13430, 0),
			6,
			reinterpret_cast<std::uintptr_t>(ActivateLoadedGridCells)
		);
	}

	void ActivateLoadedGridCellsHook::ActivateLoadedGridCells(RE::TES* a_this)
	{
		if (_ActivateLoadedGridCells == 0) {
			log::error("{}: ActivateLoadedGridCells trampoline not initialized!", __FUNCTION__);
			return;
		}

log::info("{}: Calling original ActivateLoadedGridCells", __FUNCTION__);
		using FuncType = void(*)(RE::TES*);
		auto func = reinterpret_cast<FuncType>(_ActivateLoadedGridCells);
		func(a_this);
	}


	void UpdateCellLandstateHook::Hook()
	{
		log::info("{}: Hooking UpdateCellLandstate...", __FUNCTION__);
		_UpdateCellLandstate = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(15724, 15959, 0),
			5,
			reinterpret_cast<std::uintptr_t>(UpdateCellLandstate)
		);
	}


	void UpdateCellLandstateHook::UpdateCellLandstate(RE::GridCellArray* a_this)
	{
		if (_UpdateCellLandstate == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
		if (m_blockUpdates) {
			return;
		}

		log::info("{}: Calling original UpdateCellLandstate", __FUNCTION__);
		log::info("{}: land3DAttached={}", __FUNCTION__, a_this->land3DAttached);

		using FuncType = void(*)(RE::GridCellArray*);
		auto func = reinterpret_cast<FuncType>(_UpdateCellLandstate);
		func(a_this);
	}


	void UpdateCellLandstateHook::UpdateCellLandstateNotHooked(RE::GridCellArray* a_this)
	{
		if (_UpdateCellLandstate == 0) {
			log::error("{}: UpdateCellLandstate trampoline not initialized!", __FUNCTION__);
			return;
		}

		using FuncType = void(*)(RE::GridCellArray*);
		auto func = reinterpret_cast<FuncType>(_UpdateCellLandstate);
		func(a_this);
	}


	void ActivateCellRefs3DHook::Hook()
	{
		log::info("{}: Hooking ActivateCellRefs3DHook...", __FUNCTION__);

		_ActivateCellRefs3D = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(18592, 19061, 0),
			6,
			reinterpret_cast<std::uintptr_t>(ActivateCellRefs3D)
		);
	}


	void ActivateCellRefs3DHook::ActivateCellRefs3D(RE::TESObjectCELL* a_this)
	{
		if (_ActivateCellRefs3D == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
//		if (m_blockUpdates) {
//			return;
//		}

		log::info("{}: Calling original ActivateCellRefs3D", __FUNCTION__);

		using FuncType = void(*)(RE::TESObjectCELL*);
		auto func = reinterpret_cast<FuncType>(_ActivateCellRefs3D);
		func(a_this);
	}

	void TerrainManagerIncrementalUpdateHook::Hook()
	{
		log::info("{}: Hooking TerrainManagerIncrementalUpdate...", __FUNCTION__);

		_IncrementalUpdate = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(38145, 39102, 0),
			6,
			reinterpret_cast<std::uintptr_t>(IncrementalUpdate)
		);
	}


	void TerrainManagerIncrementalUpdateHook::IncrementalUpdate()
	{
		if (_IncrementalUpdate == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}

		using FuncType = void(*)();
		auto func = reinterpret_cast<FuncType>(_IncrementalUpdate);

		if (m_blockUpdates) {
			// Redirect per-frame incremental LOD tile streaming to the camera cell.
			// UpdateLOD(flags=0) internally reads gPlayerCharacter->parentCell to determine
			// which LOD tiles to stream, so we temporarily swap parentCell to the camera's
			// current cell for the duration of this call.
			auto* player = RE::PlayerCharacter::GetSingleton();
			RE::TESObjectCELL* savedCell = nullptr;
			RE::TESObjectCELL* cameraCell = nullptr;
			if (player) {
				savedCell = player->parentCell;
				auto* playerCamera = RE::PlayerCamera::GetSingleton();
				if (playerCamera && playerCamera->currentState &&
					playerCamera->currentState->id == RE::CameraState::kFree) {
					auto* cameraState = static_cast<RE::FreeCameraState*>(playerCamera->currentState.get());

					auto* tes = RE::TES::GetSingleton();
					if (!tes) {
						log::warn("{}: Cannot access TES", __FUNCTION__);
						return;
					}

					auto* worldspace = tes->GetRuntimeData2().worldSpace;
					if (!worldspace) {
						log::warn("{}: Cannot access worldspace", __FUNCTION__);
						return;
					}

					bool loadedFromDisk;
					cameraCell = _ts_SKSEFunctions::GetCell(cameraState->translation, worldspace, loadedFromDisk);
				}
				if (cameraCell) {
					player->parentCell = cameraCell;
				}
			}

			log::info("{}: Calling original IncrementalUpdate", __FUNCTION__);
			func();

			if (player && cameraCell) {
				player->parentCell = savedCell;
			}
			return;
		}

		log::info("{}: Calling original IncrementalUpdate", __FUNCTION__);
		func();
	}


	void PostTeleportUpdateHook::Hook()
	{
		log::info("{}: Hooking ...", __FUNCTION__);

		_PostTeleportUpdate = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(39378, 40450, 0),
			6,
			reinterpret_cast<std::uintptr_t>(PostTeleportUpdate)
		);
	}


	void PostTeleportUpdateHook::PostTeleportUpdate(RE::PlayerCharacter* a_this, float a_dt, bool a_skipAI)
	{
		if (_PostTeleportUpdate == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return;
		}
//		if (m_blockUpdates) {
//			return;
//		}

		log::info("{}: Calling original PostTeleportUpdate with a_dt={}, a_skipAI={}", __FUNCTION__, a_dt, a_skipAI);

		using FuncType = void(*)(RE::PlayerCharacter*, float, bool);
		auto func = reinterpret_cast<FuncType>(_PostTeleportUpdate);
		func(a_this, a_dt, a_skipAI);
	}

	void RequestDetectionLevelHook::Hook()
	{
		_RequestDetectionLevel = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(36748, 37764, 0),
			6,
			reinterpret_cast<std::uintptr_t>(RequestDetectionLevel)
		);

	}

	std::int32_t RequestDetectionLevelHook::RequestDetectionLevel(RE::Actor* a_this, RE::Actor* a_target, RE::DETECTION_PRIORITY a_priority)
	{
		if (_RequestDetectionLevel == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return -1000;
		}

		if (m_disablePlayerDetection && a_target && a_target == RE::PlayerCharacter::GetSingleton()) {
			return -1000;
		}

		using FuncType = std::int32_t(*)(RE::Actor*, RE::Actor*, RE::DETECTION_PRIORITY);
		auto func = reinterpret_cast<FuncType>(_RequestDetectionLevel);
		std::int32_t result = func(a_this, a_target, a_priority);
		return result;
	}


	void IsHostileToPlayerHook::Hook()
	{
		_IsHostileToActor = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(36537, 37537, 0),
			5,
			reinterpret_cast<std::uintptr_t>(IsHostileToActor)
		);
	}

	bool IsHostileToPlayerHook::IsHostileToActor(RE::Actor* a_this, RE::Actor* a_other)
	{
		if (_IsHostileToActor == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return false;
		}
		
		if (m_disableHostileToPlayer && a_other && a_other == RE::PlayerCharacter::GetSingleton()) {
			return false;
		}
		
		using FuncType = bool(*)(RE::Actor*, RE::Actor*);
		return reinterpret_cast<FuncType>(_IsHostileToActor)(a_this, a_other);
	}	

	void CheckValidTargetHook::Hook()
	{
		_CheckValidTarget = _ts_SKSEFunctions::WriteFunctionHook(
			REL::VariantID(37607, 38560, 0),
			5,
			reinterpret_cast<std::uintptr_t>(CheckValidTarget)
		);
	}


	bool CheckValidTargetHook::CheckValidTarget(RE::Actor* a_this, RE::Actor* a_other)
	{
		if (_CheckValidTarget == 0) {
			log::error("{}: trampoline not initialized!", __FUNCTION__);
			return false;
		}
		
		if (m_disableCheckValidTarget && a_other && a_other == RE::PlayerCharacter::GetSingleton()) {
			return false;
		}
		using FuncType = bool(*)(RE::Actor*, RE::Actor*);
		return reinterpret_cast<FuncType>(_CheckValidTarget)(a_this, a_other);
	}

} // namespace Hooks