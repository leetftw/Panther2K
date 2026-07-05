#include "Configuration.h"
#include <cwchar>
#include <algorithm>

namespace Leet
{
    namespace Panther2K
    {
        using Util::Logger;

        SetupConfiguration::SetupConfiguration() {}
        SetupConfiguration::~SetupConfiguration() {}

        bool SetupConfiguration::LoadConfiguration(const std::wstring& filePath)
        {
            pugi::xml_parse_result result = document_.load_file(filePath.c_str());
            if (!result)
                return false;
            rootNode_ = document_.select_node(L"/Panther2KConfig").node();
            return rootNode_;
        }

        bool SetupConfiguration::HasRoot() const
        {
            return rootNode_;
        }

        pugi::xml_node SetupConfiguration::GetRoot() const
        {
            return rootNode_;
        }

        // Console node
        bool SetupConfiguration::HasConsole() const
        {
            return rootNode_ && rootNode_.child(L"Console");
        }

        pugi::xml_node SetupConfiguration::GetConsole() const
        {
            return rootNode_.child(L"Console");
        }

        bool SetupConfiguration::ValidateConsole(Logger* logger) const
        {
            if (!rootNode_) {
                wlogc(logger, PANTHER_LL_BASIC, L"[Client] Configuration contains errors! Missing node <Panther2KConfig>.");
                return false;
            }
            int count = 0;
            for (auto node : rootNode_.children(L"Console"))
                ++count;
            if (count > 1) {
                wlogc(logger, PANTHER_LL_BASIC, L"[Client] Configuration contains errors! More than one <Console> node defined.");
                return false;
            }
            if (count == 0) {
                wlogc(logger, PANTHER_LL_BASIC, L"[Client] Configuration contains errors! Missing node <Console>.");
                return false;
            }
            return true;
        }

        // My favourite technique: macros
        #define COLOR_NODE_METHODS(NAME) \
            bool SetupConfiguration::Has##NAME() const { \
                auto node = GetConsole(); \
                return node && node.child(L#NAME); \
            } \
            pugi::xml_node SetupConfiguration::Get##NAME() const { \
                auto node = GetConsole(); \
                return node.child(L#NAME); \
            } \
            bool SetupConfiguration::Validate##NAME(Logger* logger) const { \
                auto console = GetConsole(); \
                auto colorNode = console.child(L#NAME); \
                if (!colorNode) { \
                    wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Configuration contains errors! <%s> misses child node <%s>.", console.name(), L#NAME); \
                    return false; \
                } \
                pugi::xpath_node_set childNodes = colorNode.select_nodes(L"*"); \
                if (childNodes.size() > 1) { \
                    wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Configuration contains errors! <%s>/<%s> has more than one value.", console.name(), L#NAME); \
                    return false; \
                } \
                if (childNodes.size() == 1) { \
                    std::wstring colorType = childNodes.first().node().name(); \
                    std::wstring colorValue = childNodes.first().node().text().as_string(); \
                    if (colorType == L"RGB") { \
                        /* Accept any value, parseColor will check format */ \
                        return true; \
                    } else if (colorType == L"HEX") { \
                        if (colorValue.empty() || colorValue[0] != L'#' || colorValue.size() != 7) { \
                            wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Configuration contains errors! <%s>/<%s>/<HEX> is not a valid hex color.", console.name(), L#NAME); \
                            return false; \
                        } \
                        return true; \
                    } else { \
                        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Configuration contains errors! <%s>/<%s> has an unrecognized color type <%s>.", console.name(), L#NAME, colorType.c_str()); \
                        return false; \
                    } \
                } \
                return true; \
            }

        COLOR_NODE_METHODS(BackgroundColor)
        COLOR_NODE_METHODS(ForegroundColor)
        COLOR_NODE_METHODS(ErrorColor)
        COLOR_NODE_METHODS(ProgressBarColor)
        COLOR_NODE_METHODS(LightForegroundColor)
        COLOR_NODE_METHODS(DarkForegroundColor)

        // Console size
        bool SetupConfiguration::HasColumns() const
        {
            auto node = GetConsole();
            return node && node.child(L"Columns");
        }
        bool SetupConfiguration::ValidateColumns(Logger* logger) const
        {
            auto node = GetConsole();
            auto colNode = node.child(L"Columns");
            if (!colNode) {
                wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Configuration contains errors! <%s> misses child node <Columns>.", node.name());
                return false;
            }
            return true;
        }
        int SetupConfiguration::GetColumns() const
        {
            auto node = GetConsole();
            auto colNode = node.child(L"Columns");
            if (!colNode || colNode.first_child().empty())
                return 80;
            return colNode.text().as_int();
        }

        bool SetupConfiguration::HasRows() const
        {
            auto node = GetConsole();
            return node && node.child(L"Rows");
        }
        bool SetupConfiguration::ValidateRows(Logger* logger) const
        {
            auto node = GetConsole();
            auto rowNode = node.child(L"Rows");
            if (!rowNode) {
                wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Configuration contains errors! <%s> misses child node <Rows>.", node.name());
                return false;
            }
            return true;
        }
        int SetupConfiguration::GetRows() const
        {
            auto node = GetConsole();
            auto rowNode = node.child(L"Rows");
            if (!rowNode || rowNode.first_child().empty())
                return 25;
            return rowNode.text().as_int();
        }

        // LogLevel
        bool SetupConfiguration::HasLogLevel() const
        {
            if (!rootNode_) return false;
            for (auto& child : rootNode_.children()) {
                if (wcscmp(child.name(), L"LogLevel") == 0)
                    return true;
            }
            return false;
        }
        bool SetupConfiguration::ValidateLogLevel(Logger* logger) const
        {
            if (!rootNode_) return false;
            for (auto& child : rootNode_.children()) 
            {
                if (wcscmp(child.name(), L"LogLevel") == 0) 
                {
                    std::wstring logLevelStr = child.text().as_string();
                    // Convert to lower case for case-insensitive comparison
                    std::transform(logLevelStr.begin(), logLevelStr.end(), logLevelStr.begin(), ::towlower);
                    if (logLevelStr == L"basic" || logLevelStr == L"0");
                    else if (logLevelStr == L"normal" || logLevelStr == L"1");
                    else if (logLevelStr == L"detailed" || logLevelStr == L"2");
                    else if (logLevelStr == L"verbose" || logLevelStr == L"3");
                    else 
                    {
                        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Configuration contains errors! Invalid log level '%s'.", child.text().as_string());
                        return false;
                    }
                }
            }
            return true;
        }

        int SetupConfiguration::GetLogLevel() const
        {
            if (!rootNode_) return -1;
            for (auto& child : rootNode_.children())
            {
                if (wcscmp(child.name(), L"LogLevel") == 0)
                {
                    std::wstring logLevelStr = child.text().as_string();
                    std::transform(logLevelStr.begin(), logLevelStr.end(), logLevelStr.begin(), ::towlower);
                    if (logLevelStr == L"basic" || logLevelStr == L"0") {
                        return 0;
                    } else if (logLevelStr == L"normal" || logLevelStr == L"1") {
                        return 1;
                    } else if (logLevelStr == L"detailed" || logLevelStr == L"2") {
                        return 2;
                    } else if (logLevelStr == L"verbose" || logLevelStr == L"3") {
                        return 3;
                    }
                }
            }
            return -1;
        }

        // Root / WimFile
        bool SetupConfiguration::HasWimFile() const
        {
			if (!rootNode_) return false;
            for (auto& child : rootNode_.children())
            {
                if (wcscmp(child.name(), L"WimFile") == 0)
                    return true;
            }
            return false;
		}

        bool SetupConfiguration::ValidateWimFile(Logger* logger) const
        {
            if (!rootNode_) return false;
            for (auto& child : rootNode_.children())
            {
                if (wcscmp(child.name(), L"WimFile") == 0)
                {
                    std::wstring wimFilePath = child.text().as_string();
                    if (wimFilePath.empty()) {
                        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Configuration contains errors! <WimFile> value is empty.");
                        return false;
                    }
                    return true;
                }
            }
            return false;
        }

        const wchar_t* SetupConfiguration::GetWimFile() const
        {
            if (!rootNode_) return L"";
            for (auto& child : rootNode_.children()) 
            {
                if (wcscmp(child.name(), L"WimFile") == 0) 
                {
                    return child.text().get();
                }
            }
            return L"";
        }

        bool SetupConfiguration::HasWimIndex() const
        {
            if (!rootNode_) return false;
            for (auto& child : rootNode_.children()) 
            {
                if (wcscmp(child.name(), L"WimIndex") == 0)
                    return true;
            }
            return false;
        }

        bool SetupConfiguration::ValidateWimIndex(Logger* logger) const
        {
            if (!rootNode_) return false;
            for (auto& child : rootNode_.children()) 
            {
                if (wcscmp(child.name(), L"WimIndex") == 0)
                {
                    int index = child.text().as_int(-1);
                    if (index < 1) {
                        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Configuration contains errors! <WimIndex> value must be a positive integer.");
                        return false;
                    }
                    return true;
                }
            }
			return false;
        }

        int SetupConfiguration::GetWimIndex() const
        {
            if (!rootNode_) return -1;
            for (auto& child : rootNode_.children()) {
                if (wcscmp(child.name(), L"WimIndex") == 0) {
                    return child.text().as_int(-1);
                }
            }
            return -1;
        }

        bool SetupConfiguration::HasBootMethod() const
        {
            if (!rootNode_) return false;
            for (auto& child : rootNode_.children()) 
            {
                if (wcscmp(child.name(), L"BootMethod") == 0)
                    return true;
            }
            return false;
		}

        bool SetupConfiguration::ValidateBootMethod(Logger* logger) const
        {
            if (!rootNode_) return false;
            for (auto& child : rootNode_.children()) 
            {
                if (wcscmp(child.name(), L"BootMethod") == 0) 
                {
                    std::wstring method = child.text().as_string();
                    std::transform(method.begin(), method.end(), method.begin(), ::towlower);
                    if (method == L"uefi") {
                        return true;
                    } else if (method == L"legacy") {
                        return true;
                    } else {
                        wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"[Client] Configuration contains errors! <BootMethod> has an unrecognized value <%s>.", child.text().as_string());
                        return false;
                    }
                }
            }
            return false;
        }

        bool SetupConfiguration::GetBootMethod() const
        {
            if (!rootNode_) return false;
            for (auto& child : rootNode_.children()) {
                if (wcscmp(child.name(), L"BootMethod") == 0) {
                    std::wstring method = child.text().as_string();
                    std::transform(method.begin(), method.end(), method.begin(), ::towlower);
                    if (method == L"uefi") {
                        return false;
                    }
                    else if (method == L"legacy") {
                        return true;
                    }
                }
            }

            return true;
        }
    }
}

