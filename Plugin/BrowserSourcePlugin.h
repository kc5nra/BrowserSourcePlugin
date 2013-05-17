/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "OBSApi.h"
#include "BrowserManager.h"

#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)

class ServerPingSettings;

namespace Awesomium 
{
    class WebCore;
}

extern String ToAPIString(WebString &webString);

struct BrowserSourceConfig 
{
private:
    XElement *element;
public:
    String url;
    UINT width;
    UINT height;
    String customCss;
    bool isWrappingAsset;
    String assetWrapTemplate;
    bool isExposingOBSApi;

public:  // transient data, only rely on this if you know what you are doing
    HWND hwndAssetWrapTemplateEditor;
    HWND hwndCustomCssEditor;

    BrowserSourceConfig(XElement *element)
    {
        this->element = element;
        hwndAssetWrapTemplateEditor = 0;
        hwndCustomCssEditor = 0;

        Reload();
    }

    void Populate()
    {
        url = TEXT("http://www.obsproject.com");
        width = 640;
        height = 480;
        customCss = TEXT("::-webkit-scrollbar { visibility: hidden; }\r\nbody { background-color: rgba(0, 0, 0, 0); }");
        isWrappingAsset = false;
        assetWrapTemplate =
            L"<html>\r\n"
            L"  <head></head>\r\n"
            L"  <body>\r\n"
            L"    <object width='$(WIDTH)' height='$(HEIGHT)'>\r\n"
            L"      <param name='movie' value='$(FILE)'></param>\r\n"
            L"      <param name='allowscriptaccess' value='always'></param>\r\n"
            L"      <param name='wmode' value='transparent'></param>\r\n"
            L"      <embed \r\n"
            L"        src='$(FILE)' \r\n"
            L"        type='application/x-shockwave-flash' \r\n"
            L"        allowscriptaccess='always' \r\n"
            L"        width='$(WIDTH)' \r\n"
            L"        height='$(HEIGHT)' \r\n"
            L"        wmode='transparent'>\r\n"
            L"      </embed>\r\n"
            L"    </object>\r\n"
            L"  </body>\r\n"
            L"</html>\r\n";
        isExposingOBSApi = true;

    }

    void Reload()
    {
        url = element->GetString(TEXT("url"));
        width = element->GetInt(TEXT("width"));
        height = element->GetInt(TEXT("height"));
        customCss = element->GetString(TEXT("css"));
        isWrappingAsset = (element->GetInt(TEXT("isWrappingAsset")) == 1);
        assetWrapTemplate = element->GetString(TEXT("assetWrapTemplate"));
        isExposingOBSApi = (element->GetInt(TEXT("isExposingOBSApi")) == 1);
    }

    void Save()
    {
        element->SetString(TEXT("url"), url);
        element->SetInt(TEXT("width"), width);
        element->SetInt(TEXT("height"), height);
        element->SetString(TEXT("css"), customCss);
        element->SetInt(TEXT("isWrappingAsset"), (isWrappingAsset) ? 1 : 0);
        element->SetString(TEXT("assetWrapTemplate"), assetWrapTemplate);
        element->SetInt(TEXT("isExposingOBSApi"), (isExposingOBSApi) ? 1 : 0);
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
    BrowserManager *browserManager;

public:
    BrowserManager *GetBrowserManager() { return browserManager; }

};

EXTERN_DLL_EXPORT bool LoadPlugin();
EXTERN_DLL_EXPORT void UnloadPlugin();
EXTERN_DLL_EXPORT void OnStartStream();
EXTERN_DLL_EXPORT void OnStopStream();
EXTERN_DLL_EXPORT CTSTR GetPluginName();
EXTERN_DLL_EXPORT CTSTR GetPluginDescription();
