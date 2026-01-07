#pragma once

#include "API/TrueHUDAPI.h"

struct APIs
{
    static inline TRUEHUD_API::IVTrueHUD3* TrueHUD = nullptr;

	static void RequestAPIs();
};
