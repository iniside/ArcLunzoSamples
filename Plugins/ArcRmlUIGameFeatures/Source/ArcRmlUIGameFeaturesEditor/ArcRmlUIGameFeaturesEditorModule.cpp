// Copyright 2026 Lukasz Baran. All Rights Reserved.

#include "ArcRmlGameFeatureActionPaths.h"
#include "Cook/ArcRmlCookPathProviders.h"
#include "Modules/ModuleManager.h"

class FArcRmlUIGameFeaturesEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		ProviderHandle = ArcRmlCookPathProviders::Get().AddStatic(&ArcRmlGameFeatureActionPaths::Read);
	}

	virtual void ShutdownModule() override
	{
		ArcRmlCookPathProviders::Get().Remove(ProviderHandle);
		ProviderHandle.Reset();
	}

private:
	FDelegateHandle ProviderHandle;
};

IMPLEMENT_MODULE(FArcRmlUIGameFeaturesEditorModule, ArcRmlUIGameFeaturesEditor)
