#pragma once

#include <string>
#include "../PugiXML/pugixml.hpp"
#include "PantherLogger.h"

namespace Leet
{
namespace Panther2K
{
    class SetupConfiguration
    {
    public:
        SetupConfiguration();
        ~SetupConfiguration();

        // Loads the configuration XML file
        bool LoadConfiguration(const std::wstring& filePath);

        // Console node
        bool HasConsole() const;
        pugi::xml_node GetConsole() const;
        bool ValidateConsole(Util::Logger* logger) const;

        // Color entries
        bool HasBackgroundColor() const;
        bool ValidateBackgroundColor(Util::Logger* logger) const;
        pugi::xml_node GetBackgroundColor() const;

        bool HasForegroundColor() const;
        bool ValidateForegroundColor(Util::Logger* logger) const;
        pugi::xml_node GetForegroundColor() const;

        bool HasErrorColor() const;
        bool ValidateErrorColor(Util::Logger* logger) const;
        pugi::xml_node GetErrorColor() const;

        bool HasProgressBarColor() const;
        bool ValidateProgressBarColor(Util::Logger* logger) const;
        pugi::xml_node GetProgressBarColor() const;

        bool HasLightForegroundColor() const;
        bool ValidateLightForegroundColor(Util::Logger* logger) const;
        pugi::xml_node GetLightForegroundColor() const;

        bool HasDarkForegroundColor() const;
        bool ValidateDarkForegroundColor(Util::Logger* logger) const;
        pugi::xml_node GetDarkForegroundColor() const;

        // Console size
        bool HasColumns() const;
        bool ValidateColumns(Util::Logger* logger) const;
        int GetColumns() const;

        bool HasRows() const;
        bool ValidateRows(Util::Logger* logger) const;
        int GetRows() const;

        // LogLevel
        bool HasLogLevel() const;
        bool ValidateLogLevel(Util::Logger* logger) const;
        int GetLogLevel() const;

        // WimFile
		bool HasWimFile() const;
		bool ValidateWimFile(Util::Logger* logger) const;
        const wchar_t* GetWimFile() const;

		// WimIndex
		bool HasWimIndex() const;
		bool ValidateWimIndex(Util::Logger* logger) const;
		int GetWimIndex() const;

        // BootMethod
		bool HasBootMethod() const;
		bool ValidateBootMethod(Util::Logger* logger) const;
		bool GetBootMethod() const;

        // Root node
        bool HasRoot() const;
        pugi::xml_node GetRoot() const;

    private:
        pugi::xml_document document_;
        pugi::xml_node rootNode_;
    };
}
}