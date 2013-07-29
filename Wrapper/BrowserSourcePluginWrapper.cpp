/**
 * John Bradley (jrb@turrettech.com)
 */


#include "BrowserSourcePluginWrapper.h"

static HINSTANCE hinstDLL = 0;
static HMODULE hmodBspPlugin;
static HMODULE hmodCoherentUI;
static HMODULE hmodZlib;
static HMODULE hmodSciLexer;

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
		InternalGetPluginDescription) 
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
			hmodCoherentUI	= LoadLibrary(L".\\plugins\\BrowserSourcePlugin\\CoherentUI.dll");

			// SwfReader reqs
			hmodZlib		= LoadLibrary(L".\\plugins\\BrowserSourcePlugin\\lib\\zlib1.dll");

			// Settings GUI reqs	
			hmodSciLexer	= LoadLibrary(L".\\plugins\\BrowserSourcePlugin\\lib\\SciLexer.dll");
			
			// main plugin dll
			hmodBspPlugin	= LoadLibrary(L".\\plugins\\BrowserSourcePlugin\\BrowserSourcePlugin.dll");

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
			if (hmodSciLexer) FreeLibrary(hmodSciLexer);
			if (hmodZlib) FreeLibrary(hmodZlib);
			if (hmodCoherentUI) FreeLibrary(hmodCoherentUI);

			break;
 
	}
	return TRUE;
}
