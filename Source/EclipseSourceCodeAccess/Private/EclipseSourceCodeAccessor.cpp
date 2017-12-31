#include "EclipseSourceCodeAccessor.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "DesktopPlatformModule.h"
#include "Misc/UProjectInfo.h"
#include "Misc/App.h"

DEFINE_LOG_CATEGORY_STATIC(LogEclipseAccessor, Log, All);

#define LOCTEXT_NAMESPACE "EclipseSourceCodeAccessor"

// http://help.eclipse.org/juno/index.jsp?topic=%2Forg.eclipse.platform.doc.isv%2Freference%2Fmisc%2Fruntime-options.html

bool FEclipseSourceCodeAccessor::CanAccessSourceCode() const
{
	FString Path;
	return CanRunEclipse(Path);
}

FName FEclipseSourceCodeAccessor::GetFName() const
{
	return FName("EclipseSourceCodeAccessor");
}

FText FEclipseSourceCodeAccessor::GetNameText() const
{
	return LOCTEXT("EclipseDisplayName", "Eclipse");
}

FText FEclipseSourceCodeAccessor::GetDescriptionText() const
{
	return LOCTEXT("EclipseDisplayDesc", "Open source code files with Eclipse");
}

bool FEclipseSourceCodeAccessor::OpenSolution()
{
	FString SolutionPath = GetSolutionPath();
	FString Solution = "-data \"" + SolutionPath + "\"";
	return RunEclipse(Solution, true);
}

bool FEclipseSourceCodeAccessor::OpenSolutionAtPath(const FString& InSolutionPath)
{
	FString SolutionPath = FPaths::GetPath(FPaths::GetPath(InSolutionPath));
	FString Solution = "-data \"" + SolutionPath + "\"";
	return RunEclipse(Solution, true);
}

bool FEclipseSourceCodeAccessor::DoesSolutionExist() const
{
	FString SolutionPath = GetSolutionPath();
	FString MetadataPath = FPaths::Combine(SolutionPath, TEXT(".metadata"));
	return FPaths::DirectoryExists(MetadataPath);
}

bool FEclipseSourceCodeAccessor::OpenFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber)
{
	return false;
}

bool FEclipseSourceCodeAccessor::OpenSourceFiles(const TArray<FString>& AbsoluteSourcePaths)
{
	FString Args = FString(TEXT("--launcher.timeout 60 --launcher.openFile"));
	for (const FString& SourcePath : AbsoluteSourcePaths)
	{
		const FString NewSourcePath = FString::Printf(TEXT(" \"%s\""), *SourcePath);
		Args.Append(NewSourcePath);
	}
	return RunEclipse(Args);
}

bool FEclipseSourceCodeAccessor::AddSourceFiles(const TArray<FString>& AbsoluteSourcePaths, const TArray<FString>& AvailableModules)
{
	return false;
}

bool FEclipseSourceCodeAccessor::SaveAllOpenDocuments() const
{
	return false;
}

void FEclipseSourceCodeAccessor::Tick(const float DeltaTime)
{
}

bool FEclipseSourceCodeAccessor::CanRunEclipse(FString& OutPath) const
{
	// TODO This might be not a good idea to find an executable.
	OutPath = TEXT("/usr/bin/eclipse");
	if (!FPaths::FileExists(OutPath))
	{
		TCHAR EclipseBinaryEnv[32768] = { 0 };
		FPlatformMisc::GetEnvironmentVariable(TEXT("UE4_ECLIPSE_BINARY"), EclipseBinaryEnv, ARRAY_COUNT(EclipseBinaryEnv));
		OutPath = EclipseBinaryEnv;

		if (!FPaths::FileExists(OutPath))
		{
			UE_LOG(LogEclipseAccessor, Warning, TEXT("FEclipseSourceCodeAccessor: Can not find eclipse binary - export UE4_ECLIPSE_BINARY environment variable"));
			return false;
		}
	}
	return true;
}

bool FEclipseSourceCodeAccessor::RunEclipse(const FString& InArgs, bool InReplaceInstance)
{
	FString EclipsePath;
	if (!CanRunEclipse(EclipsePath))
	{
		return false;
	}

	if (!InReplaceInstance && FPlatformProcess::IsProcRunning(RunningInstance))
	{
		FProcHandle Proc = FPlatformProcess::CreateProc(*EclipsePath, *InArgs, true, false, false, nullptr, 0, nullptr, nullptr);
		if (Proc.IsValid())
		{
			FPlatformProcess::WaitForProc(Proc);
			FPlatformProcess::CloseProc(Proc);
			return true;
		}
		return false;
	}

	FString Args;
	if (InReplaceInstance)
	{
		if (FPlatformProcess::IsProcRunning(RunningInstance))
		{
			UE_LOG(LogEclipseAccessor, Warning, TEXT("FEclipseSourceCodeAccessor: Killing previous running instance"));
			FPlatformProcess::TerminateProc(RunningInstance);
		}
		Args = InArgs;
	}
	else
	{
		FString Workspace = GetSolutionPath();
		Args = FString(TEXT("-data \"") + Workspace + TEXT("\" ") + InArgs);
	}

	if (RunningInstance.IsValid())
	{
		FPlatformProcess::WaitForProc(RunningInstance);
		FPlatformProcess::CloseProc(RunningInstance);
	}

	RunningInstance = FPlatformProcess::CreateProc(*EclipsePath, *Args, false, false, false, nullptr, 0, nullptr, nullptr);
	return RunningInstance.IsValid();
}

FString FEclipseSourceCodeAccessor::GetSolutionPath() const
{
	if (IsInGameThread())
	{
		FString RootDirPath = FPaths::RootDir();
		FString ProjectDirPath = FPaths::ProjectDir();

		if (!FUProjectDictionary(RootDirPath).IsForeignProject(ProjectDirPath))
		{
			CachedSolutionPath = FPaths::ConvertRelativePathToFull(RootDirPath);
		}
		else
		{
			CachedSolutionPath = FPaths::ConvertRelativePathToFull(FPaths::GetPath(FPaths::GetPath(ProjectDirPath)));
		}
	}
	return CachedSolutionPath;
}

void FEclipseSourceCodeAccessor::Startup()
{
	// Cache this so we don't have to do it on a background thread
	GetSolutionPath();
}

void FEclipseSourceCodeAccessor::Shutdown()
{
}
