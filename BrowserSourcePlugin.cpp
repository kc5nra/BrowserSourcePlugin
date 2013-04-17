/**
 * John Bradley (jrb@turrettech.com)
 */


#include "BrowserSourcePlugin.h"
#include "Localization.h"
#include "BrowserSource.h"

#include <Awesomium\WebCore.h>

HINSTANCE BrowserSourcePlugin::hinstDLL = NULL;
BrowserSourcePlugin *BrowserSourcePlugin::instance = NULL;

#define BROWSER_SOURCE_CLASS TEXT("BrowserSource")


ImageSource* STDCALL CreateBrowserSource(XElement *data)
{
	return new BrowserSource(data); 
}

bool STDCALL ConfigureBrowserSource(XElement *element, bool bCreating)
{
	return true;
}

BrowserSourcePlugin::BrowserSourcePlugin()
{
	isDynamicLocale = false;
	settings = NULL;

	browserManager = new BrowserManager();

	if (!locale->HasLookup(KEY("PluginName"))) {
		isDynamicLocale = true;
		int localizationStringCount = sizeof(localizationStrings) / sizeof(CTSTR);
		Log(TEXT("Browser Source plugin strings not found, dynamically loading %d strings"), sizeof(localizationStrings) / sizeof(CTSTR));
		for(int i = 0; i < localizationStringCount; i += 2) {
			locale->AddLookupString(localizationStrings[i], localizationStrings[i+1]);
		}
		if (!locale->HasLookup(KEY("PluginName"))) {
			AppWarning(TEXT("Uh oh..., unable to dynamically add our localization keys"));
		}
	}

	API->RegisterImageSourceClass(BROWSER_SOURCE_CLASS, STR("ClassName"), (OBSCREATEPROC)CreateBrowserSource, (OBSCONFIGPROC)ConfigureBrowserSource);
}

BrowserSourcePlugin::~BrowserSourcePlugin() 
{
	
	delete browserManager;

	if (isDynamicLocale) {
		int localizationStringCount = sizeof(localizationStrings) / sizeof(CTSTR);
		Log(TEXT("Server Ping plugin instance deleted; removing dynamically loaded localization strings"));
		for(int i = 0; i < localizationStringCount; i += 2) {
			locale->RemoveLookupString(localizationStrings[i]);
		}
	}

	isDynamicLocale = false;
}

bool LoadPlugin()
{
    if(BrowserSourcePlugin::instance != NULL) {
        return false;
	}
    BrowserSourcePlugin::instance = new BrowserSourcePlugin();
    return true;
}

void UnloadPlugin()
{
    if(BrowserSourcePlugin::instance == NULL) {
        return;
	}
    delete BrowserSourcePlugin::instance;
    BrowserSourcePlugin::instance = NULL;
}

void OnStartStream()
{
	if (BrowserSourcePlugin::instance == NULL) {
		return;
	}

	BrowserSourcePlugin *plugin = BrowserSourcePlugin::instance;
	plugin->GetBrowserManager()->Startup();
}

void OnStopStream()
{
	if (BrowserSourcePlugin::instance == NULL) {
		return;
	}

	BrowserSourcePlugin *plugin = BrowserSourcePlugin::instance;
	
	plugin->GetBrowserManager()->AddEvent(new Browser::Event(Browser::SHUTDOWN, NULL));
}

CTSTR GetPluginName()
{
    return STR("PluginName");
}

CTSTR GetPluginDescription()
{
    return STR("PluginDescription");
}

BOOL CALLBACK DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if(fdwReason == DLL_PROCESS_ATTACH)
        BrowserSourcePlugin::hinstDLL = hinstDLL;
    return TRUE;
}
