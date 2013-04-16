/**
 * John Bradley (jrb@turrettech.com)
 */
#pragma once

#include "OBSApi.h"

#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)

class ServerPingSettings;

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

public:
	ServerPingSettings *GetSettings() { return settings; }
};

EXTERN_DLL_EXPORT bool LoadPlugin();
EXTERN_DLL_EXPORT void UnloadPlugin();
EXTERN_DLL_EXPORT void OnStartStream();
EXTERN_DLL_EXPORT void OnStopStream();
EXTERN_DLL_EXPORT CTSTR GetPluginName();
EXTERN_DLL_EXPORT CTSTR GetPluginDescription();
