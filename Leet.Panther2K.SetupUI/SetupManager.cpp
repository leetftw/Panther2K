#include "SetupManager.h"

#include <string>
#include <sstream>
#include <algorithm>

#include "../PugiXML/pugixml.hpp"
#include "Page.h"

#define STEP(a) std::tuple<const wchar_t*, StepResult(SetupManager::*)()>(L#a, &SetupManager::##a)

#define IOCTL_MOUNTMGR_QUERY_POINTS CTL_CODE('m', 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define MOUNTMGR_IS_VOLUME_NAME(s, len) (                                          \
     (len == 48 || (len == 49 && s[48] == L'\\')) &&           \
     s[0] == L'\\' &&                                                          \
     s[1] == L'?' &&                                                           \
     s[2] == L'?' &&                                                           \
     s[3] == L'\\' &&                                                          \
     s[4] == L'V' &&                                                           \
     s[5] == L'o' &&                                                           \
     s[6] == L'l' &&                                                           \
     s[7] == L'u' &&                                                           \
     s[8] == L'm' &&                                                           \
     s[9] == L'e' &&                                                           \
     s[10] == L'{' &&                                                          \
     s[19] == L'-' &&                                                          \
     s[24] == L'-' &&                                                          \
     s[29] == L'-' &&                                                          \
     s[34] == L'-' &&                                                          \
     s[47] == L'}'                                                             \
    )


#include "WelcomePage.h"
#include "ImageSelectionPage.h"
#include "BootMethodSelectionPage.h"
#include "DiskSelectionPage.h"
#include "MessageBoxPage.h"
#include "WimApplyPage.h"
#include "PartitionSelectionPage.h"

typedef struct _MOUNTMGR_MOUNT_POINT {
    ULONG  SymbolicLinkNameOffset;
    USHORT SymbolicLinkNameLength;
    USHORT Reserved1;
    ULONG  UniqueIdOffset;
    USHORT UniqueIdLength;
    USHORT Reserved2;
    ULONG  DeviceNameOffset;
    USHORT DeviceNameLength;
    USHORT Reserved3;
} MOUNTMGR_MOUNT_POINT, * PMOUNTMGR_MOUNT_POINT;

typedef struct _MOUNTMGR_MOUNT_POINTS {
    ULONG                Size;
    ULONG                NumberOfMountPoints;
    MOUNTMGR_MOUNT_POINT MountPoints[1];
} MOUNTMGR_MOUNT_POINTS, * PMOUNTMGR_MOUNT_POINTS;



PageResult ShowPageModal(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger, Page* page)
{
    PageResult result;
    KEY_EVENT_RECORD* record = console->Read();
    while (!record->bKeyDown || (result = page->HandleKey(record->wVirtualKeyCode)) == PageSuccess)
    {
        safeFree(logger, record);
        record = console->Read();
    }
    safeFree(logger, record);

    return result;
}


void Leet::Panther2K::SetupManager::PreviousStep()
{
    currentStep -= 2;
}

Leet::Panther2K::SetupManager::SetupManager(Leet::Panther2K::Util::Console* console, Leet::Panther2K::Util::Logger* logger)
{
    this->console = console;
    this->logger = logger;
}

void Leet::Panther2K::SetupManager::RunSetup()
{
    setupSteps = {
        STEP(Initialize),
        STEP(LoadConfiguration),
        STEP(InitializeEngine),
        STEP(WelcomeUser),
        STEP(SelectWIMImage),
        STEP(SelectBootMethod),
        STEP(SelectPartMethod),
        STEP(SelectPartitions),
        STEP(StartInstallation),
        STEP(HandleInstallMessages),
        STEP(FinalizeSetup)
    };

    for (currentStep = 0; currentStep < setupSteps.size(); currentStep++)
    {
        auto step = setupSteps[currentStep];
        wlogf(logger, PANTHER_LL_NORMAL, MAX_PATH, L"[Client] Running step '%s'...", std::get<0>(step));
        switch ((*this.*(std::get<1>(step)))()) 
        {
        case StepResult::Exit:
        {
            wlogc(logger, PANTHER_LL_BASIC, L"[Client] Exit requested, stopping installation...");
            if (SUCCEEDED(exitCode)) 
            {
                exitCode = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                return;
            }
        }
        case StepResult::Fail:
        {
            if (HRESULT_FACILITY(exitCode) == FACILITY_WIN32)
                wlogerr(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Setup failed: %s (0x%08X)", HRESULT_CODE(exitCode), exitCode);
            else wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Setup failed: HRESULT 0x%08X", exitCode);

            Page page;
            page.statusText = L"";
            page.Initialize(console);
            page.Draw();

            MessageBoxPage messagePage = MessageBoxPage(L"Panther2K encountered an irrecoverable error. The application cannot continue running. See debug.log for more information.", true, &page);
            messagePage.Initialize(console, &page);
            messagePage.ShowDialog();

            return;
        }
        case StepResult::SkipNext:
            currentStep++;
            break;
        case StepResult::GoBack:
            currentStep -= 2;
            break;
        }
    }

    return;
}

HRESULT Leet::Panther2K::SetupManager::GetResult()
{
    return exitCode;
}

StepResult Leet::Panther2K::SetupManager::Initialize()
{
    if (!logger || !console)
    {
        exitCode = HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
        return StepResult::Fail;
    }
    return StepResult::Success;
}

bool parseColor(const pugi::xml_node& parentNode, const std::wstring& nodeName, Leet::Panther2K::Util::CONSOLE_COLOR& color, Leet::Panther2K::Util::Logger* logger) 
{
    pugi::xml_node colorNode = parentNode.child(nodeName.c_str());
    if (!colorNode)
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to load config! <%s> misses child node <%s>.", parentNode.name(), nodeName.c_str());
        return false;
    }

    pugi::xpath_node_set childNodes = colorNode.select_nodes(REL NODE(L"*"));
    if (childNodes.empty()) 
    {
        return true;
    } 
    else if (childNodes.size() > 1)
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to load config! <%s>/<%s> has more than one value.", parentNode.name(), nodeName.c_str());
        return false;
    }

    std::wstring colorType = childNodes.first().node().name();
    std::wstring colorValue = childNodes.first().node().text().as_string();
    if (colorType == L"RGB")
    {
        int r = 0, g = 0, b = 0;
        std::replace(colorValue.begin(), colorValue.end(), ',', ' ');
        std::wistringstream(colorValue) >> r >> g >> b;
        color.R = r; color.G = g; color.B = b;
        return true;
    }
    else if (colorType == L"HEX")
    {
        if (!colorValue[0] == L'#' || colorValue.size() != 7)
        {
            wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to load config! <%s>/<%s>/<HEX> is not a valid hex color.", parentNode.name(), nodeName.c_str());
            return false;
        }

        unsigned int hexColor = 0;
        std::wstringstream ss;
        ss << std::hex << colorValue.substr(1);
        ss >> hexColor;
        return true;
    }

    wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to load config! <%s>/<%s> has an unrecognized color type <%s>.", parentNode.name(), nodeName.c_str(), colorType.c_str());
    return false;
}

StepResult Leet::Panther2K::SetupManager::LoadConfiguration()
{
    wlogc(logger, PANTHER_LL_NORMAL, L"[Client] Loading configuration...");

    Page page;
    page.statusText = L"  Parsing 'config.xml'...";
    page.Initialize(console);
    page.Draw();
    
    pugi::xml_document document;
    document.load_file(L"config.xml");

    pugi::xml_node rootNode = document.select_node(NODE("Panther2KConfig")).node();
    if (!rootNode)
    {
        wlogc(logger, PANTHER_LL_BASIC, L"[Client] Failed to load config! Missing node <Panther2KConfig>.");
        exitCode = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        return StepResult::Fail;
    }

    const wchar_t* query = REL NODE(L"Console");
    pugi::xpath_node_set consoleNodes = rootNode.select_nodes(query);
    if (consoleNodes.size() > 1)
    {
        wlogc(logger, PANTHER_LL_BASIC, L"[Client] Failed to load config! More than one <Console> node defined.");
        exitCode = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        return StepResult::Fail;
    }
    else if (consoleNodes.size() == 0)
    {
        wlogc(logger, PANTHER_LL_BASIC, L"[Client] Failed to load config! Missing node <Console>.");
        exitCode = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        return StepResult::Fail;
    }
    pugi::xml_node consoleNode = consoleNodes.first().node();

    Leet::Panther2K::Util::CONSOLE_COLOR* colors = new Leet::Panther2K::Util::CONSOLE_COLOR[6];

#if PANTHER_RELEASE_TYPE == PANTHER_RT_RELEASE
    Util::CONSOLE_COLOR defColor = { 0, 0, 170 };
    if (!parseColor(consoleNode, L"BackgroundColor", defColor, logger))
        return StepResult::Fail;
    colors[0] = defColor;
    defColor = { 170, 170, 170 };
    if (!parseColor(consoleNode, L"ForegroundColor", defColor, logger))
        return StepResult::Fail;
    colors[1] = defColor;
    defColor = { 170, 0, 0 };
    if (!parseColor(consoleNode, L"ErrorColor", defColor, logger))
        return StepResult::Fail;
    colors[2] = defColor;
    defColor = { 255, 255, 0 };
    if (!parseColor(consoleNode, L"ProgressBarColor", defColor, logger))
        return StepResult::Fail;
    colors[3] = defColor;
    defColor = { 255, 255, 255 };
    if (!parseColor(consoleNode, L"LightForegroundColor", defColor, logger))
        return StepResult::Fail;
    colors[4] = defColor;
    defColor = { 0, 0, 0 };
    if (!parseColor(consoleNode, L"DarkForegroundColor", defColor, logger))
        return StepResult::Fail;
    colors[5] = defColor;
#else
    colors[0] = { 170 / 3, 0, 170 / 2 };
    colors[1] = { 170, 170, 170 };
    colors[2] = { 170, 0, 0 };
    colors[3] = { 255, 255, 0 };
    colors[4] = { 255, 255, 255 };
    colors[5] = { 0, 0, 0 };
    wlogc(logger, PANTHER_LL_BASIC, L"[Client] Not a release build, forcing console colors.");
#endif
    console->SetColorTable(colors, 6);

    pugi::xml_node columnsNode = consoleNode.child(L"Columns");
    if (!columnsNode)
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to load config! <%s> misses child node <Columns>.", consoleNode.name());
        exitCode = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        return StepResult::Fail;
    }
    pugi::xml_node rowsNode = consoleNode.child(L"Rows");
    if (!rowsNode)
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to load config! <%s> misses child node <Rows>.", consoleNode.name());
        exitCode = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        return StepResult::Fail;
    }
    int cols = columnsNode.first_child().empty() ? 80 : columnsNode.text().as_int();
    int rows = rowsNode.first_child().empty() ? 25 : rowsNode.text().as_int();
    console->SetSize(cols, rows);
    console->Clear();
    page.Draw();

    for (auto& child : rootNode.children()) 
    {
        if (wcscmp(child.name(), L"") == 0)
        {
            int logLevel = child.text().as_int();
            if (logLevel < PANTHER_LL_BASIC || logLevel > PANTHER_LL_VERBOSE)
            {
                wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to load config! Invalid log level %d.", logLevel);
                exitCode = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                return StepResult::Fail;
			}
            logger->SetLogLevel(logLevel);
            continue;
        }
    }

    return StepResult::Success;
}

StepResult Leet::Panther2K::SetupManager::InitializeEngine()
{
    if (engine) return StepResult::Success;

    HRESULT result = PantherCreateEngine(&engine, logger);
    if (FAILED(result))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to initialize setup engine! (0x%08X)", result);
        exitCode = result;
        return StepResult::Fail;
    }

    return StepResult::Success;
}

StepResult Leet::Panther2K::SetupManager::WelcomeUser()
{
    WelcomePage page;
    page.Initialize(console);
    page.Draw();

    MessageBoxPage msgBox(L"This is a beta release of Panther2K. Going back to a previous step is not supported yet. You should expect to have some issues in certain configurations. Leet is not responsible for any damage to your system and/or personal data.", false, &page);
    msgBox.Initialize(console, &page);
    msgBox.ShowDialog();
    
    page.Draw();
    PageResult result = ShowPageModal(console, logger, &page);
    if (result == PageExit) return StepResult::Exit;

    return StepResult::Success;
}

StepResult Leet::Panther2K::SetupManager::SelectWIMImage()
{
    std::vector<std::wstring> paths;

    HANDLE hMPM = CreateFileW(L"\\\\.\\MountPointManager", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (hMPM == INVALID_HANDLE_VALUE)
    {
        // Could not open mount point manager
        exitCode = HRESULT_FROM_WIN32(GetLastError());
        return StepResult::Fail;
    }

    MOUNTMGR_MOUNT_POINT point;
    memset(&point, 0, sizeof(MOUNTMGR_MOUNT_POINT));
    DWORD bytesReturned = 0;

    size_t size = sizeof(MOUNTMGR_MOUNT_POINTS);
    PMOUNTMGR_MOUNT_POINTS mountPoints = static_cast<PMOUNTMGR_MOUNT_POINTS>(safeMalloc(logger, size));
    BOOL bResult = DeviceIoControl(hMPM, IOCTL_MOUNTMGR_QUERY_POINTS, &point, sizeof(MOUNTMGR_MOUNT_POINT), mountPoints, size, &bytesReturned, NULL);
    
    while (!bResult && GetLastError() == ERROR_MORE_DATA) 
    {
        mountPoints = static_cast<PMOUNTMGR_MOUNT_POINTS>(realloc(mountPoints, mountPoints->Size));
        if (!mountPoints)
        {
            logger->WriteDirect(PANTHER_LL_BASIC, L"FATAL: OUT OF MEMORY.\r\n");
            ExitProcess(HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY));
            return StepResult::Fail;
        }
        bResult = DeviceIoControl(hMPM, IOCTL_MOUNTMGR_QUERY_POINTS, &point, sizeof(MOUNTMGR_MOUNT_POINT), mountPoints, mountPoints->Size, &bytesReturned, NULL);
    }

    CloseHandle(hMPM);
    if (!bResult)
    {
        safeFree(logger, mountPoints);
        exitCode = HRESULT_FROM_WIN32(GetLastError());
        return StepResult::Fail;
    }

    wlogc(logger, PANTHER_LL_DETAILED, L"[Client] Enumerating volumes...");
    for (int i = 0; i < mountPoints->NumberOfMountPoints; i++)
    {
        wlogf(logger, PANTHER_LL_VERBOSE, MAX_PATH * 3, L"[Client] - %.*s is mounted at path %.*s", 
            static_cast<unsigned int>(mountPoints->MountPoints[i].DeviceNameLength / sizeof(wchar_t)), 
            reinterpret_cast<wchar_t*>(reinterpret_cast<char*>(mountPoints) + mountPoints->MountPoints[i].DeviceNameOffset),
            static_cast<unsigned int>(mountPoints->MountPoints[i].SymbolicLinkNameLength / sizeof(wchar_t)), 
            reinterpret_cast<wchar_t*>(reinterpret_cast<char*>(mountPoints) + mountPoints->MountPoints[i].SymbolicLinkNameOffset));

        wchar_t* symlink = reinterpret_cast<wchar_t*>(reinterpret_cast<char*>(mountPoints) + mountPoints->MountPoints[i].SymbolicLinkNameOffset);
        size_t length = mountPoints->MountPoints[i].SymbolicLinkNameLength / sizeof(wchar_t);
        if (!MOUNTMGR_IS_VOLUME_NAME(symlink, length))
            continue;

        symlink[1] = L'\\'; 
        paths.push_back(std::wstring(symlink, length));
    }

    safeFree(logger, mountPoints);

    wlogc(logger, PANTHER_LL_DETAILED, L"[Client] Searching for WIM file in enumerated volumes...");

    std::wstring wimPath = L"";
    for (auto& a : paths)
    {
        wlogc(logger, PANTHER_LL_VERBOSE, a.c_str());

        std::wstring path = a + L"\\sources\\install.wim";
        DWORD dwAttrib = GetFileAttributesW(path.c_str());
        if (dwAttrib != INVALID_FILE_ATTRIBUTES &&
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
            wimPath.assign(path);

        path = a + L"\\sources\\install.esd";
        dwAttrib = GetFileAttributesW(path.c_str());
        if (dwAttrib != INVALID_FILE_ATTRIBUTES &&
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
            wimPath.assign(path);
       
        path = a + L"\\sources\\install.swm";
        dwAttrib = GetFileAttributesW(path.c_str());
        if (dwAttrib != INVALID_FILE_ATTRIBUTES &&
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
            wimPath.assign(path);
    }

    if (wimPath == L"")
    {
        exitCode = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Could not find a WIM file! The installation is aborted. (0x%08X)", exitCode);
        return StepResult::Fail;
    }

    wlogf(logger, PANTHER_LL_DETAILED, MAX_PATH * 2, L"[Client] Found WIM file at path %s", wimPath.c_str());

    //HRESULT hResult = PantherEngineSetWimFile(engine, L"\\\\?\\Volume{88d8d147-48e7-41e9-a4d2-7943f6dd64a9}\\sources\\boot.wim");
    HRESULT hResult = PantherEngineSetWimFile(engine, wimPath.c_str());
    if (FAILED(hResult))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to load WIM file. The engine reported an error. The installation is aborted. (0x%08X)", hResult);
        exitCode = hResult;
        return StepResult::Fail;
    }

    wlogc(logger, PANTHER_LL_DETAILED, L"[Client] Retrieving image list...");
    PantherWimInfo* wimInfo;
    hResult = PantherEngineGetWimInfo(engine, &wimInfo);

    if (FAILED(hResult))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to retrieve image list. The engine reported an error. The installation is aborted. (0x%08X)", hResult);
        exitCode = hResult; 
        return StepResult::Fail;
    }

    int selectedIndex;
    if (wimInfo->ImageCount == 1)
    {
        selectedIndex = 1;
    }
    else
    {
        wlogc(logger, PANTHER_LL_DETAILED, L"[Client] Multiple images exist, requesting choice from user.");
        ImageSelectionPage page;
        page.Initialize(console);
        if (!page.SetData(wimInfo))
        {
            exitCode = HRESULT_FROM_WIN32(GetLastError());
            wlogf(logger, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Client] Failed to read image list data! The installation is aborted. (0x%08X)", exitCode);
            return StepResult::Fail;
        }
        page.Draw();
        LocalFree(wimInfo);

        PageResult result = ShowPageModal(console, logger, &page);
        if (result == PageGoBack) return StepResult::GoBack;
        else if (result == PageExit) return StepResult::Exit;

        selectedIndex = page.GetResult();
    }

    hResult = PantherEngineSetWimIndex(engine, selectedIndex);
    if (FAILED(hResult))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to set image index. The engine reported an error. The installation is aborted. (0x%08X)", hResult);
        exitCode = hResult;
        return StepResult::Fail;
    }

    return StepResult::Success;
}

StepResult Leet::Panther2K::SetupManager::SelectBootMethod()
{
    BootMethodSelectionPage page;
    page.Initialize(console);
    page.Draw();

    PageResult pageResult = ShowPageModal(console, logger, &page);
    if (pageResult == PageGoBack) return StepResult::GoBack;
    else if (pageResult == PageExit) return StepResult::Exit;

    useLegacy = page.GetResult();
    HRESULT result = PantherEngineSetUseLegacy(engine, useLegacy);
    if (FAILED(result))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to set boot method. The engine reported an error. The installation is aborted. (0x%08X)", result);
        exitCode = result; 
        return StepResult::Fail;
    }

    return StepResult::Success;
}

StepResult Leet::Panther2K::SetupManager::SelectPartMethod()
{
    DiskSelectionPage page;
    page.Initialize(console);
    HRESULT result = page.LoadData(console, logger);
    if (FAILED(result))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to enumerate disks. WinParted reported an error. The installation is aborted. (0x%08X)", result);
        exitCode = result;
        return StepResult::Fail;
    }

    userSelection:
    page.Draw();
    PageResult pageResult = ShowPageModal(console, logger, &page);
    if (pageResult == PageGoBack) return StepResult::GoBack;
    else if (pageResult == PageExit) return StepResult::Exit;

    int selectedDisk = page.GetResult();
    if (selectedDisk == -1)
    {
        wlogc(logger, PANTHER_LL_DETAILED, L"[Client] Custom partitioning requested, entering Easy Part Mode.");
        /*
        MessageBoxPage msgBox(L"Not implemented.", true, &page);
        msgBox.Initialize(console, &page);
        msgBox.ShowDialog();
        goto userSelection;*/
        return StepResult::Success;
    }

    wlogf(logger, PANTHER_LL_DETAILED, MAX_PATH, L"[Client] Selected disk #%d.", selectedDisk);
    
    wchar_t mountPoints[3][MAX_PATH];
    wchar_t volumes[3][MAX_PATH];

    wchar_t** volumesPtr = (wchar_t**)safeMalloc(logger, sizeof(wchar_t*) * 2);
    for (int i = 0; i < 2; i++)
        volumesPtr[i] = volumes[i];

    wlogc(logger, PANTHER_LL_DETAILED, L"[Client] Starting format operation on the disk...");

    result = (useLegacy ? WinPartedDll::ApplyP2KLayoutToDiskMBR : WinPartedDll::ApplyP2KLayoutToDiskGPT)
        (console, logger, selectedDisk, true, nullptr, &volumesPtr);

    safeFree(logger, volumesPtr);

    if (FAILED(result))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Client] Failed to prepare the disk. WinParted reported an error. The installation is not aborted. (0x%08X)", result);

        wchar_t displayMessage[MAX_PATH * 2];
        wchar_t errorMessage[MAX_PATH];
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, result, NULL, errorMessage, MAX_PATH, NULL);
        swprintf_s(displayMessage, L"An error occurred while preparing the disk. WinParted reported an error: %s", errorMessage);

        MessageBoxPage* msgBox = new MessageBoxPage(displayMessage, false, &page);
        msgBox->Initialize(console, &page);
        msgBox->ShowDialog();
        delete msgBox;

        goto userSelection;
    }

    wlogc(logger, PANTHER_LL_DETAILED, L"[Client] Passing volume information to engine...");
    
    std::wstring bootPartition = std::wstring(volumes[0] + 10, 38);
    std::wstring systemPartition = std::wstring(volumes[1] + 10, 38);
    
    result = PantherEngineSetBootVolume(engine, bootPartition.c_str());
    if (FAILED(result))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Client] Failed to select the boot volume (DiskPartitioning). The engine reported an error. The installation is not aborted. (0x%08X)", result);
        
        wchar_t displayMessage[MAX_PATH * 2];
        wchar_t errorMessage[MAX_PATH];
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, result, NULL, errorMessage, MAX_PATH, NULL);
        swprintf_s(displayMessage, L"An error occurred while selecting the prepared boot volume. The engine reported an error: %s", errorMessage);

        MessageBoxPage* msgBox = new MessageBoxPage(displayMessage, false, &page);
        msgBox->Initialize(console, &page);
        msgBox->ShowDialog();
        delete msgBox;

        goto userSelection;
    }

    result = PantherEngineSetSystemVolume(engine, systemPartition.c_str()); 
    if (FAILED(result))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Client] Failed to select the system volume (DiskPartitioning). The engine reported an error. The installation is not aborted. (0x%08X)", result);
        
        wchar_t displayMessage[MAX_PATH * 2];
        wchar_t errorMessage[MAX_PATH];
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, result, NULL, errorMessage, MAX_PATH, NULL);
        swprintf_s(displayMessage, L"An error occurred while selecting the prepared system volume. The engine reported an error: %s", errorMessage);

        MessageBoxPage* msgBox = new MessageBoxPage(displayMessage, false, &page);
        msgBox->Initialize(console, &page);
        msgBox->ShowDialog();
        delete msgBox;

        goto userSelection;
    }

    return StepResult::SkipNext;
}

StepResult Leet::Panther2K::SetupManager::SelectPartitions()
{
    // Easy partitioning mode
    VolumeSelectionPage page(L"NTFS", 0ULL, 0ULL, 0, 0);
    page.Initialize(console);

    VolumeInformation* volumeInfos; int count;
    HRESULT result = WinPartedDll::EnumVolumes(console, logger, &volumeInfos, false, &count);
    if (FAILED(result)) DebugBreak();
    page.SetVolumeList(volumeInfos, count);

    userSelection:
    page.Draw();
    PageResult pageResult = ShowPageModal(console, logger, &page);
    if (pageResult == PageGoBack) return StepResult::GoBack;
    else if (pageResult == PageExit) return StepResult::Exit;
    
    VolumeInformation info = page.GetSelectedVolume();
    wchar_t volumes[2][128];
    result = WinPartedDll::PrepareDiskForWindows(console, logger, info.VolumeFile, useLegacy, 500000000ULL, 0ULL, volumes);
    if (FAILED(result))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Client] Failed to select the system volume (EasyPartitioning). WinParted reported an error. The installation is not aborted. (0x%08X)", result);
        
        wchar_t displayMessage[MAX_PATH * 2];
        wchar_t errorMessage[MAX_PATH];
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, result, NULL, errorMessage, MAX_PATH, NULL);
        swprintf_s(displayMessage, L"An error occurred while preparing the selected system volume. WinParted reported an error: %s", errorMessage);

        MessageBoxPage* msgBox = new MessageBoxPage(displayMessage, false, &page);
        msgBox->Initialize(console, &page);
        msgBox->ShowDialog();
        delete msgBox;

        goto userSelection;
    }

    // \\?\Volume{aabbccdd-aabb-ccdd-eeff-gghhaabbccdd}\
    // needs to be in format {aabbccdd-aabb-ccdd-eeff-gghhaabbccdd}
    volumes[0][48] = 0; volumes[1][48] = 0;
    result = PantherEngineSetSystemVolume(engine, volumes[0] + 10);
    if (FAILED(result))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Client] Failed to select the system volume (EasyPartitioning). The engine reported an error. The installation is not aborted. (0x%08X)", result);
        
        wchar_t displayMessage[MAX_PATH * 2];
        wchar_t errorMessage[MAX_PATH];
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, result, NULL, errorMessage, MAX_PATH, NULL);
        swprintf_s(displayMessage, L"An error occurred while selecting the prepared system volume. The engine reported an error: %s", errorMessage);

        MessageBoxPage* msgBox = new MessageBoxPage(displayMessage, false, &page);
        msgBox->Initialize(console, &page);
        msgBox->ShowDialog();
        delete msgBox;

        goto userSelection;
    }
    result = PantherEngineSetBootVolume(engine, volumes[1] + 10);
    if (FAILED(result))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH * 2, L"[Client] Failed to select the boot volume (EasyPartitioning). The engine reported an error. The installation is not aborted. (0x%08X)", result);

        wchar_t displayMessage[MAX_PATH * 2];
        wchar_t errorMessage[MAX_PATH];
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, result, NULL, errorMessage, MAX_PATH, NULL);
        swprintf_s(displayMessage, L"An error occurred while selecting the prepared boot volume. The engine reported an error: %s", errorMessage);

        MessageBoxPage* msgBox = new MessageBoxPage(displayMessage, false, &page);
        msgBox->Initialize(console, &page);
        msgBox->ShowDialog();
        delete msgBox;

        goto userSelection;
    }

    return StepResult::Success;
}

StepResult Leet::Panther2K::SetupManager::StartInstallation()
{
    // Create the message queue by calling PeekMessage
    MSG msg;
    PeekMessageW(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    HRESULT result = PantherEngineSetCallbackThread(engine, GetCurrentThreadId());
    if (FAILED(result))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to set callback thread. The engine reported an error. The installation is aborted. (0x%08X)", result);
        exitCode = result;
        return StepResult::Fail;
    }

    // Begin the installation
    result = PantherEngineStartInstallation(engine);
    if (FAILED(result))
    {
        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Failed to start the installation. The engine reported an error. The installation is aborted. (0x%08X)", result);
        exitCode = result;
        return StepResult::Fail;
    }

    return StepResult::Success;
}

#include "BootPreparationPage.h"

StepResult Leet::Panther2K::SetupManager::HandleInstallMessages()
{
    MSG msg;

    WimApplyPage applyPage;
    applyPage.Initialize(console);

    BootPreparationPage bootPage;
    bootPage.Initialize(console);

    applyPage.Draw();

    // Implement message loop which handles callbacks
    // Required to determine when installation is finished and for progress info.
    bool loop = true;
    while (loop)
    {
        HANDLE fileEvent = 0;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == TM_PANTHER_PROGRESS)
            {
                applyPage.Update(msg.wParam);
            }
            else if (msg.message == TM_PANTHER_FILENAME)
            {
                applyPage.Update((wchar_t*)msg.wParam);
                fileEvent = (HANDLE)msg.lParam;
            }
            else if (msg.message == TM_PANTHER_BOOTSTEP)
            {
                switch (msg.wParam)
                {
                case PANTHER_BOOTSTEP_BOOT:
                    bootPage.statusText = L"Creating boot files...";
                    bootPage.Draw();
                    break;
                case PANTHER_BOOTSTEP_RECOVERY:
                    bootPage.statusText = L"Setting up Windows Recovery Environment (RE)...";
                    bootPage.Redraw();
                    break;
                }
            }
            else if (msg.message == TM_PANTHER_FINISH)
            {
                bootPage.statusText = L"Finishing up...";
                bootPage.Redraw();
                loop = false;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Clear keyboard events (F3?)

        Sleep(50);
        if (fileEvent) SetEvent(fileEvent);
    }

    return StepResult::Success;
}

#include "FinalPage.h"

StepResult Leet::Panther2K::SetupManager::FinalizeSetup()
{
    FinalPage page;
    page.Initialize(console);
    page.Draw();

    ShowPageModal(console, logger, &page);
    return StepResult::Success;
}
