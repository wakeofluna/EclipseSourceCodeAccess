#include "EclipseSourceCodeAccessModule.h"
#include "Modules/ModuleManager.h"
#include "Features/IModularFeatures.h"

IMPLEMENT_MODULE(FEclipseSourceCodeAccessModule, EclipseSourceCodeAccess);

void FEclipseSourceCodeAccessModule::StartupModule()
{
	EclipseSourceCodeAccessor.Startup();

	IModularFeatures::Get().RegisterModularFeature(TEXT("SourceCodeAccessor"), &EclipseSourceCodeAccessor);
}

void FEclipseSourceCodeAccessModule::ShutdownModule()
{
	IModularFeatures::Get().UnregisterModularFeature(TEXT("SourceCodeAccessor"), &EclipseSourceCodeAccessor);

	EclipseSourceCodeAccessor.Shutdown();
}

FEclipseSourceCodeAccessor& FEclipseSourceCodeAccessModule::GetAccessor()
{
	return EclipseSourceCodeAccessor;
}
