/**
 * John Bradley (jrb@turrettech.com)
 */


#include "BrowserSourcePluginWrapper.h"

static HINSTANCE hinstDLL = 0;
static HMODULE hmodBspPlugin;
static HMODULE hmodAvCodec;
static HMODULE hmodAvFormat;
static HMODULE hmodAvUtil;
static HMODULE hmodLibEGL;
static HMODULE hmodLibGLESv2;
static HMODULE hmodIcudt;
static HMODULE hmodAwesomium;

static LOADPLUGIN_PROC InternalLoadPlugin = 0;
static UNLOADPLUGIN_PROC InternalUnloadPlugin = 0;
static ONSTARTSTREAM_PROC InternalOnStartStream = 0;
static ONSTOPSTREAM_PROC InternalOnStopStream = 0;
static GETPLUGINNAME_PROC InternalGetPluginName = 0;
static GETPLUGINDESCRIPTION_PROC InternalGetPluginDescription = 0;

bool LoadPlugin()
{
	if (InternalLoadPlugin &&
		InternalUnloadPlugin &&
		InternalOnStartStream && 
		InternalOnStopStream &&
		InternalGetPluginName &&
		InternalGetPluginName) 
	{
			return InternalLoadPlugin();
	}

	return false;
}

void UnloadPlugin()
{
	InternalUnloadPlugin();
}

void OnStartStream()
{
	InternalOnStartStream();
}

void OnStopStream()
{
	InternalOnStopStream();
}

CTSTR GetPluginName()
{
	return InternalGetPluginName();
}

CTSTR GetPluginDescription()
{
	return InternalGetPluginDescription();
}

BOOL CALLBACK DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			
			// order is important!

			hmodAvUtil = LoadLibrary(L".\\plugins\\BrowserSourcePlugin\\lib\\avutil-51.dll");
			hmodAvCodec = LoadLibrary(L".\\plugins\\BrowserSourcePlugin\\lib\\avcodec-53.dll");
			hmodAvFormat = LoadLibrary(L".\\plugins\\BrowserSourcePlugin\\lib\\avformat-53.dll");
			
			hmodLibGLESv2 = LoadLibrary(L".\\plugins\\BrowserSourcePlugin\\lib\\libGLESv2.dll");
			hmodLibEGL = LoadLibrary(L".\\plugins\\BrowserSourcePlugin\\lib\\libEGL.dll");
			
			hmodIcudt = LoadLibrary(L".\\plugins\\BrowserSourcePlugin\\lib\\icudt.dll");
			
			hmodAwesomium = LoadLibrary(L".\\plugins\\BrowserSourcePlugin\\lib\\awesomium.dll");

			hmodBspPlugin = LoadLibrary(L".\\plugins\\BrowserSourcePlugin\\BrowserSourcePlugin.dll");
			
			if (hmodBspPlugin != NULL) {
				InternalLoadPlugin = (LOADPLUGIN_PROC)GetProcAddress(hmodBspPlugin, "LoadPlugin");
				InternalUnloadPlugin = (UNLOADPLUGIN_PROC)GetProcAddress(hmodBspPlugin, "UnloadPlugin");
				InternalOnStartStream = (ONSTARTSTREAM_PROC)GetProcAddress(hmodBspPlugin, "OnStartStream");
				InternalOnStopStream = (ONSTOPSTREAM_PROC)GetProcAddress(hmodBspPlugin, "OnStopStream");
				InternalGetPluginName = (GETPLUGINNAME_PROC)GetProcAddress(hmodBspPlugin, "GetPluginName");
				InternalGetPluginDescription = (GETPLUGINDESCRIPTION_PROC)GetProcAddress(hmodBspPlugin, "GetPluginDescription");

			}
			break;
 
		case DLL_PROCESS_DETACH:

			if (hmodBspPlugin) FreeLibrary(hmodBspPlugin);
			if (hmodAwesomium) FreeLibrary(hmodAwesomium);
			if (hmodIcudt) FreeLibrary(hmodIcudt);
			if (hmodLibEGL) FreeLibrary(hmodLibEGL);
			if (hmodLibGLESv2) FreeLibrary(hmodLibGLESv2);
			if (hmodAvFormat) FreeLibrary(hmodAvFormat);
			if (hmodAvCodec) FreeLibrary(hmodAvCodec);
			if (hmodAvUtil) FreeLibrary(hmodAvUtil);
			break;
 
	}
	return TRUE;
}
