#include "Hooks.h"
#include "TimelineManager.h"

#include <DbgHelp.h>

namespace Hooks
{
	void Install()
	{
		log::info("Hooking...");

		MainUpdateHook::Hook();
		LookHook::Hook();
		MovementHook::Hook();

 //       GridCellArrayHook::Hook();

		log::info("...success");
	}

	void MainUpdateHook::Nullsub()
	{
		_Nullsub();

		FCFW::TimelineManager::GetSingleton().Update();

//		LogPlayerCellInfo();
	}

	void MainUpdateHook::LogPlayerCellInfo()
	{
		auto playerPos = RE::PlayerCharacter::GetSingleton()->GetPosition();
		auto tes = RE::TES::GetSingleton();
		if (!tes || !tes->gridCells) {
			return;
		}

		auto* targetCell = tes->GetCell(playerPos);
		if (!targetCell) {
			return;
		}

		auto coords = targetCell->GetCoordinates();
		if (!coords) {
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
	
	bool GridCellArrayHook::SetCenter(RE::GridCellArray* a_this, std::int32_t a_x, std::int32_t a_y)
	{
		log::info("=== GridCellArray::SetCenter called: ({}, {}) ===", a_x, a_y);
return _SetCenter(a_this, a_x, a_y);		
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
		return _SetCenter(a_this, a_x, a_y);
	}
} // namespace Hooks
