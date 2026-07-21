#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

/** Log category for the Ammunition plugin. */
DECLARE_LOG_CATEGORY_EXTERN(LogSisaAmmunition, Log, All);

class FAmmunitionModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
