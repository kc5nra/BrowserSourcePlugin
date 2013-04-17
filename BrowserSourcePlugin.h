/**
 * John Bradley (jrb@turrettech.com)
 */
#pragma once

#include "OBSApi.h"
#include "BrowserManager.h"

#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)

class ServerPingSettings;
namespace Awesomium {
	class WebCore;
}

class BrowserSourcePlugin
{

public:
    static HINSTANCE hinstDLL;
	static BrowserSourcePlugin *instance;

public:
    BrowserSourcePlugin();
    ~BrowserSourcePlugin();

private:
	bool isDynamicLocale;
	ServerPingSettings *settings;
	BrowserManager *browserManager;


public:
	ServerPingSettings *GetSettings() { return settings; }
	BrowserManager *GetBrowserManager() { return browserManager; }

};

EXTERN_DLL_EXPORT bool LoadPlugin();
EXTERN_DLL_EXPORT void UnloadPlugin();
EXTERN_DLL_EXPORT void OnStartStream();
EXTERN_DLL_EXPORT void OnStopStream();
EXTERN_DLL_EXPORT CTSTR GetPluginName();
EXTERN_DLL_EXPORT CTSTR GetPluginDescription();
