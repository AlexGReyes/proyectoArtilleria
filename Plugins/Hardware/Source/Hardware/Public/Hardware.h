#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

/** Log category for the Hardware plugin. */
DECLARE_LOG_CATEGORY_EXTERN(LogSisaHardware, Log, All);

class FHardwareModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
