#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

/** Log category for the Artillery plugin. */
DECLARE_LOG_CATEGORY_EXTERN(LogSisaArtillery, Log, All);

class FArtilleryModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
