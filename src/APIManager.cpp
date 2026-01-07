#include "APIManager.h"

void APIs::RequestAPIs()
{
    if (!TrueHUD) {
		TrueHUD = reinterpret_cast<TRUEHUD_API::IVTrueHUD3*>(TRUEHUD_API::RequestPluginAPI(TRUEHUD_API::InterfaceVersion::V3));
		if (TrueHUD) {
			log::info("{}: Obtained TrueHUD API - {:x}", __FUNCTION__, reinterpret_cast<uintptr_t>(TrueHUD));
		} else {
			log::info("{}: Failed to obtain TrueHUD API", __FUNCTION__);
		}
	}	
}

