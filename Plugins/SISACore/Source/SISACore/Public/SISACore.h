#pragma once

#include "Modules/ModuleManager.h"

class FSISACoreModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
