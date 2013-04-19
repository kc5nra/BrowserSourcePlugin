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

struct BrowserSourceConfig {
private:
	XElement *element;
public:
	String url;
	UINT width;
	UINT height;
	String customCss;

	BrowserSourceConfig(XElement *element)
	{
		this->element = element;
		Reload();
	}

	void Populate()
	{
		url = TEXT("http://www.obsproject.com");
		width = 640;
		height = 480;
		customCss = TEXT("::-webkit-scrollbar { visibility: hidden; }\nbody { background-color: rgba(0, 0, 0, 0); }");
	}

	void Reload()
	{
		url = element->GetString(TEXT("url"));
		width = element->GetInt(TEXT("width"));
		height = element->GetInt(TEXT("height"));
		customCss = element->GetString(TEXT("css"));
	}

	void Save()
	{
		element->SetString(TEXT("url"), url);
		element->SetInt(TEXT("width"), width);
		element->SetInt(TEXT("height"), height);
		element->SetString(TEXT("css"), customCss);
	}
};

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
