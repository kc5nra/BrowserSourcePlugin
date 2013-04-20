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
	KEY("ClassName"),			L"Browser",
	KEY("Settings"),			L"Browser Source Settings",
	KEY("UrlOrAsset"),			L"Enter a URL with http/https scheme or using the asset scheme",
	KEY("UrlOrAssetDesc"),		L"To access the current working directory you can use"
								L" asset://local/...\n"
								L"For example: asset://local/plugins/BrowserSourcePlugin/page.html\n"
								L"This will load an html file in your plugins directory",
	KEY("BrowserWidth"),		L"Browser Width:",
	KEY("BrowserHeight"),		L"Browser Height:",
	KEY("CustomCSS"),			L"Custom CSS",
	KEY("CustomCSSDesc"),		L"Custom CSS allows you to define a base CSS stylesheet that the"
								L" loaded website will automatically inherit",
	KEY("IsWrappingAsset"),		L"Wrap the asset in an html body"
};
