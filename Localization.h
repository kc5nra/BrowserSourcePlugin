/**
 * John Bradley (jrb@turrettech.com)
 */
#pragma once

#define STR_PREFIX L"Plugins.BrowserSource."
#define KEY(k) (STR_PREFIX L ## k)
#define STR(text) locale->LookupString(KEY(text))

#ifndef BSP_VERSION
#define BSP_VERSION L" !INTERNAL VERSION!"
#endif

static CTSTR localizationStrings[] = {
	KEY("PluginName"),			L"Browser Capture Plugin",
	KEY("PluginDescription"),	L"Renders an off-screen browser as a video source"
								L"\n\n"
								L"Plugin Version: " BSP_VERSION,
	KEY("ClassName"),		L"Browser",
};
