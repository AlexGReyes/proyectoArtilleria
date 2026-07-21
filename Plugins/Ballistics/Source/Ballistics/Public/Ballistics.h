#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

/** Log category for the Ballistics plugin. */
DECLARE_LOG_CATEGORY_EXTERN(LogSisaBallistics, Log, All);

class FBallisticsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
