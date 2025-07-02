#pragma once

#include <windows.h>

#include <vector>
#include <tuple>

#include <PantherConsole.h>
#include <PantherLogger.h>

#include <SetupEngineC.h>

typedef enum {
    Success,
    Fail,
    SkipNext,
    GoBack,
    Exit,
} StepResult;

namespace Leet
{
	namespace Panther2K
	{
		class SetupManager
		{
        public:
            void PreviousStep();

            SetupManager(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger);
            void RunSetup();

            HRESULT GetResult();

        private:
            int currentStep = 0;
            Leet::Panther2K::Util::Console* console = nullptr;
            Leet::Panther2K::Util::Logger* logger = nullptr;
            std::vector<std::tuple<const wchar_t*, StepResult(SetupManager::*)()>> setupSteps = { };
            HRESULT exitCode = S_OK;

            HSetupEngine engine = nullptr;
            bool useLegacy = false;

            StepResult Initialize();
            StepResult LoadConfiguration();
            StepResult InitializeEngine();
            StepResult WelcomeUser();
            
            StepResult SelectWIMImage();
            
            StepResult SelectBootMethod();
            StepResult SelectPartMethod();
            StepResult SelectPartitions();
            
            StepResult StartInstallation();
            StepResult HandleInstallMessages();
            StepResult FinalizeSetup();
		};
	}
}