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
} StepResult;

namespace Leet
{
	namespace Panther2K
	{
		class SetupManager
		{
        public:
            void PreviousStep();

            SetupManager(Console* console, LibPanther::Logger* logger);
            void RunSetup();

            HRESULT GetResult();

        private:
            int currentStep = 0;
            Console* console = nullptr;
            LibPanther::Logger* logger = nullptr;
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