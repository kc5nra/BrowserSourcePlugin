/**
 * John Bradley (jrb@turrettech.com)
 */


#include "BrowserSourcePlugin.h"
#include "Localization.h"
#include "BrowserSource.h"
#include "resource.h"

#include <Awesomium\WebCore.h>

HINSTANCE BrowserSourcePlugin::hinstDLL = NULL;
BrowserSourcePlugin *BrowserSourcePlugin::instance = NULL;

#define BROWSER_SOURCE_CLASS TEXT("BrowserSource")

INT_PTR CALLBACK ConfigureDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
    {
        case WM_INITDIALOG:
        {
			SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
			BrowserSourceConfig *config = (BrowserSourceConfig *)lParam;

			LocalizeWindow(hwnd);

			HWND hwndUrlOrAsset = GetDlgItem(hwnd, IDC_URL_OR_ASSET);
			HWND hwndWidth = GetDlgItem(hwnd, IDC_WIDTH);
            HWND hwndHeight = GetDlgItem(hwnd, IDC_HEIGHT);
			HWND hwndCustomCss = GetDlgItem(hwnd, IDC_CUSTOM_CSS);
			HWND hwndIsWrappingAsset = GetDlgItem(hwnd, IDC_IS_WRAPPING_ASSET);
			HWND hwndAssetWrapTemplate = GetDlgItem(hwnd, IDC_ASSET_WRAP_TEMPLATE);

			SendMessage(hwndUrlOrAsset, WM_SETTEXT, 0, (LPARAM)config->url.Array());
			SendMessage(hwndWidth, WM_SETTEXT, 0, (LPARAM)IntString(config->width).Array());
			SendMessage(hwndHeight, WM_SETTEXT, 0, (LPARAM)IntString(config->height).Array());
			SendMessage(hwndCustomCss, WM_SETTEXT, 0, (LPARAM)config->customCss.Array());
			SendMessage(hwndIsWrappingAsset, BM_SETCHECK, config->isWrappingAsset, 0);
			SendMessage(hwndAssetWrapTemplate, WM_SETTEXT, 0, (LPARAM)config->assetWrapTemplate.Array());

			SendMessage(hwndAssetWrapTemplate, WM_ENABLE, config->isWrappingAsset, 0);

			return TRUE;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wParam)) 
			{
				case IDC_IS_WRAPPING_ASSET:
				{
					HWND hwndIsWrappingAsset = GetDlgItem(hwnd, IDC_IS_WRAPPING_ASSET);
					HWND hwndAssetWrapTemplate = GetDlgItem(hwnd, IDC_ASSET_WRAP_TEMPLATE);

					bool isWrappingAsset = (SendMessage(hwndIsWrappingAsset, BM_GETCHECK, 0, 0) == 1);
					SendMessage(hwndAssetWrapTemplate, WM_ENABLE, isWrappingAsset, 0);
					break;
				}
				case IDOK:
				{
					HWND hwndUrlOrAsset = GetDlgItem(hwnd, IDC_URL_OR_ASSET);
					HWND hwndWidth = GetDlgItem(hwnd, IDC_WIDTH);
					HWND hwndHeight = GetDlgItem(hwnd, IDC_HEIGHT);
					HWND hwndCustomCss = GetDlgItem(hwnd, IDC_CUSTOM_CSS);
					HWND hwndIsWrappingAsset = GetDlgItem(hwnd, IDC_IS_WRAPPING_ASSET);
					HWND hwndAssetWrapTemplate = GetDlgItem(hwnd, IDC_ASSET_WRAP_TEMPLATE);

					BrowserSourceConfig *config = (BrowserSourceConfig *)GetWindowLongPtr(hwnd, DWLP_USER);

					String str;
					str.SetLength((UINT)SendMessage(hwndUrlOrAsset, WM_GETTEXTLENGTH, 0, 0));
                    if(!str.Length()) return TRUE;
                    SendMessage(hwndUrlOrAsset, WM_GETTEXT, str.Length()+1, (LPARAM)str.Array());
					config->url = str;

					str.SetLength((UINT)SendMessage(hwndWidth, WM_GETTEXTLENGTH, 0, 0));
                    if(!str.Length()) return TRUE;
                    SendMessage(hwndWidth, WM_GETTEXT, str.Length()+1, (LPARAM)str.Array());
					config->width = str.ToInt();

					str.SetLength((UINT)SendMessage(hwndHeight, WM_GETTEXTLENGTH, 0, 0));
                    if(!str.Length()) return TRUE;
                    SendMessage(hwndHeight, WM_GETTEXT, str.Length()+1, (LPARAM)str.Array());
					config->height = str.ToInt();

					str.SetLength((UINT)SendMessage(hwndCustomCss, WM_GETTEXTLENGTH, 0, 0));
                    if(str.Length()) {
						SendMessage(hwndCustomCss, WM_GETTEXT, str.Length()+1, (LPARAM)str.Array());
					} else {
						str.Clear();
					}

					config->isWrappingAsset = (SendMessage(hwndIsWrappingAsset, BM_GETCHECK, 0, 0) == 1);

					str.SetLength((UINT)SendMessage(hwndAssetWrapTemplate, WM_GETTEXTLENGTH, 0, 0));
                    if(!str.Length()) return TRUE;
					SendMessage(hwndAssetWrapTemplate, WM_GETTEXT, str.Length()+1, (LPARAM)str.Array());
					
					config->assetWrapTemplate = str;

					EndDialog(hwnd, IDOK);
					return TRUE;
				}
				case IDCANCEL:
				{
					EndDialog(hwnd, IDCANCEL);
					return TRUE;
				}
			}
			break;
		}
		case WM_CLOSE:
		{
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		}
	}
	
	return FALSE;
}

ImageSource* STDCALL CreateBrowserSource(XElement *data)
{
	return new BrowserSource(data); 
}

bool STDCALL ConfigureBrowserSource(XElement *element, bool bCreating)
{

	XElement *dataElement = element->GetElement(TEXT("data"));

	bool isMissingDataElement;
	if (isMissingDataElement = !dataElement) {
		dataElement = element->CreateElement(TEXT("data"));
	}

	BrowserSourceConfig *config = new BrowserSourceConfig(dataElement);
	if (isMissingDataElement) {
		config->Populate();
	}

	if(DialogBoxParam(BrowserSourcePlugin::hinstDLL, MAKEINTRESOURCE(IDD_BROWSERCONFIG), API->GetMainWindow(), ConfigureDialogProc, (LPARAM)config) == IDOK)
    {
		config->Save();
		element->SetInt(TEXT("cx"), config->width);
        element->SetInt(TEXT("cy"), config->height);

		delete config;
        return true;
    }

	delete config;
	return false;
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
