#include "args.hxx"
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

int main(int argc, char** argv) {
    std::string version = "0.2.0";

    args::ArgumentParser parser("QueryDisplayScaling Version " + version + " - GPLv3\nGitHub - https://github.com/amitxv\n");
    args::HelpFlag help(parser, "help", "display this help menu", { "help" });

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what();
        std::cerr << parser;
        return 1;
    } catch (args::ValidationError e) {
        std::cerr << e.what();
        std::cerr << parser;
        return 1;
    }

    std::vector<DISPLAYCONFIG_PATH_INFO> paths;
    std::vector<DISPLAYCONFIG_MODE_INFO> modes;
    UINT32 flags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;
    LONG result = ERROR_SUCCESS;

    do {
        // determine how many path and mode structures to allocate
        UINT32 pathCount, modeCount;
        result = GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount);

        if (result != ERROR_SUCCESS) {
            return HRESULT_FROM_WIN32(result);
        }

        // allocate the path and mode arrays
        paths.resize(pathCount);
        modes.resize(modeCount);

        // get all active paths and their modes
        result = QueryDisplayConfig(flags, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);

        // the function may have returned fewer paths/modes than estimated
        paths.resize(pathCount);
        modes.resize(modeCount);

        // it's possible that between the call to GetDisplayConfigBufferSizes and QueryDisplayConfig
        // that the display state changed, so loop on the case of ERROR_INSUFFICIENT_BUFFER.
    } while (result == ERROR_INSUFFICIENT_BUFFER);

    if (result != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(result);
    }

    for (auto& path : paths) {
        // find the target (monitor) friendly name
        DISPLAYCONFIG_TARGET_DEVICE_NAME targetName = {};
        targetName.header.adapterId = path.targetInfo.adapterId;
        targetName.header.id = path.targetInfo.id;
        targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        targetName.header.size = sizeof(targetName);
        result = DisplayConfigGetDeviceInfo(&targetName.header);

        if (result != ERROR_SUCCESS) {
            return HRESULT_FROM_WIN32(result);
        }

        // find the adapter device name
        DISPLAYCONFIG_ADAPTER_NAME adapterName = {};
        adapterName.header.adapterId = path.targetInfo.adapterId;
        adapterName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
        adapterName.header.size = sizeof(adapterName);

        result = DisplayConfigGetDeviceInfo(&adapterName.header);

        if (result != ERROR_SUCCESS) {
            return HRESULT_FROM_WIN32(result);
        }

        std::wcout
            << L"Monitor: "
            << (targetName.flags.friendlyNameFromEdid ? targetName.monitorFriendlyDeviceName : L"Unknown")
            << L"\nScaling Mode: "
            << path.targetInfo.scaling
            << L"\n";
    }
}