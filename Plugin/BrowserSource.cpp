/**
* John Bradley (jrb@turrettech.com)
*/

#include "BrowserSource.h"
#include "BrowserSourcePlugin.h"
#include "DataSources.h"
#include "MimeTypes.h"

#include <Awesomium\WebCore.h>
#include <Awesomium\WebView.h>
#include <Awesomium\WebURL.h>
#include <Awesomium\STLHelpers.h>
#include <Awesomium\BitmapSurface.h>
#include <Awesomium\DataPak.h>
#include <Awesomium\JSValue.h>
#include <Awesomium\WebViewListener.h>

using namespace Awesomium;

#define NO_VIEW -2
#define PENDING_VIEW -1

class BrowserSource::BrowserSourceListener : public WebViewListener::Load, public JSMethodHandler
{

public:
    BrowserSourceListener(BrowserSource *browserSource) 
    {
        this->browserSource = browserSource;
    }

    ~BrowserSourceListener() 
    {
    }

private:
    BrowserSource *browserSource;

public: //WebViewListener::Load
    void OnBeginLoadingFrame(Awesomium::WebView* caller, int64 frameId, bool isMainFrame, const Awesomium::WebURL& url, bool isErrorPage) {}
    void OnFailLoadingFrame(Awesomium::WebView* caller, int64 frameId, bool isMainFrame, const Awesomium::WebURL& url, int errorCode, const Awesomium::WebString& errorDesc) {}
    void OnFinishLoadingFrame(Awesomium::WebView* caller, int64 frameId, bool isMainFrame, const Awesomium::WebURL& url) {};

    void 
    OnMethodCall(
        Awesomium::WebView* caller, 
        unsigned int remoteObjectId, 
        const Awesomium::WebString& methodName,	
        const Awesomium::JSArray& args)
    {
        auto &javascriptExtensions = browserSource->javascriptExtensions;
        auto functionName = ToString(methodName);

        for(UINT i = 0; i < javascriptExtensions.size(); i++) {
            if (javascriptExtensions[i]->Handles(NO_RETURN_ARGUMENT, remoteObjectId, functionName)) {
                javascriptExtensions[i]->Handle(functionName, args);
                return;
            }
        }
    }

    JSValue 
    OnMethodCallWithReturnValue(
        Awesomium::WebView* caller, 
        unsigned int remoteObjectId, 
        const Awesomium::WebString& methodName, 
        const Awesomium::JSArray& args)
    {
        auto &javascriptExtensions = browserSource->javascriptExtensions;
        auto functionName = ToString(methodName);

        for(UINT i = 0; i < javascriptExtensions.size(); i++) {
            if (javascriptExtensions[i]->Handles(RETURN_ARGUMENT, remoteObjectId, functionName)) {
                return javascriptExtensions[i]->Handle(functionName, args);
            }
        }
        return JSValue::Undefined();
    }

    void OnDocumentReady(Awesomium::WebView* caller, const Awesomium::WebURL& url)
    {

        if (!browserSource->hasRegisteredJavascriptExtensions) {
            auto &javascriptExtensions = browserSource->javascriptExtensions;

            for (UINT i = 0; i < javascriptExtensions.size(); i++) {
                javascriptExtensions[i]->Register(caller);
            }
            browserSource->hasRegisteredJavascriptExtensions = true;
        }

        caller->ExecuteJavascript(WSLit("OBSApiReady()"), WSLit(""));
    }

};

BrowserSource::BrowserSource(XElement *data)
{
	Log(TEXT("Using Browser Source"));

    hWebView = -2;
    globalSourceRefCount = 1;

    hasRegisteredJavascriptExtensions = false;

    browserSourceListener = new BrowserSourceListener(this);
    config = new BrowserSourceConfig(data);

    InitializeCriticalSection(&textureLock);

    UpdateSettings();	
}

BrowserSource::~BrowserSource()
{

    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();
    browserManager->ShutdownAndWait(this);

    delete browserSourceListener;

    if (texture) {
        delete texture;
        texture = nullptr;
    }

    delete config;
    for (UINT i = 0; i < dataSources.size(); i++) {
        delete dataSources[i];
    }
    dataSources.clear();
    
    for (UINT i = 0; i < javascriptExtensions.size(); i++) {
        delete javascriptExtensions[i];
    }
    javascriptExtensions.clear();
    
    DeleteCriticalSection(&textureLock);

    config = nullptr;
}

void BrowserSource::Tick(float fSeconds)
{
    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();

    if (globalSourceRefCount > 0) {
        if (hWebView == PENDING_VIEW) {
            browserManager->Update();
        } else if (hWebView >= 0) {
            browserManager->AddEvent(new Browser::Event(Browser::UPDATE, this, hWebView));
        }
    }
}

void BrowserSource::GlobalSourceEnterScene()
{
    InterlockedIncrement(&globalSourceRefCount);
}

void BrowserSource::GlobalSourceLeaveScene()
{
    InterlockedDecrement(&globalSourceRefCount);
}

WebView *BrowserSource::CreateWebViewCallback(WebCore *webCore, const int hWebView) 
{

    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();

    WebPreferences webPreferences;
    if (config->customCss.Length()) {

        WebString customCss;
        char *utf8str = config->customCss.CreateUTF8String();
        int len = tstr_to_utf8_datalen(config->customCss);

        if (utf8str) {
            customCss = WebString::CreateFromUTF8(utf8str, len);
            Free(utf8str);
        }
   
        webPreferences.user_stylesheet = customCss;
    }
    webPreferences.enable_web_gl = true;
    WebSession *webSession;
    webSession = webCore->CreateWebSession(WSLit("plugins\\BrowserSourcePlugin\\cache"), webPreferences);
  
    for (UINT i = 0; i < dataSources.size(); i++) {
        delete dataSources[i];
    }
    dataSources.clear();

    std::string assetWrapTemplate;
    
    char *utf8str = config->assetWrapTemplate.CreateUTF8String();
    if (utf8str) {
        assetWrapTemplate = utf8str;
        Free(utf8str);
    }

    dataSources.push_back(new BrowserDataSource(config->isWrappingAsset, assetWrapTemplate, config->width, config->height));
    dataSources.push_back(new BlankDataSource(config->isWrappingAsset, assetWrapTemplate, config->width, config->height));

    for(UINT i = 0; i < dataSources.size(); i++) {
        int mimeTypeCount = sizeof(mimeTypes) / sizeof(*mimeTypes);
        mimeTypeCount /= 2;
        for(int j = 0; j < mimeTypeCount; j += 2) {
            dataSources[i]->AddMimeType(mimeTypes[j], mimeTypes[j+1]);
        }
        webSession->AddDataSource(dataSources[i]->GetHost(), dataSources[i]);
    }

    for (UINT i = 0; i < javascriptExtensions.size(); i++) {
        delete javascriptExtensions[i];
    }

    javascriptExtensions.clear();
    hasRegisteredJavascriptExtensions = false;

    auto &javascriptExtensionFactories = browserManager->GetJavascriptExtensionFactories();
    
    for(auto i = javascriptExtensionFactories.begin(); i < javascriptExtensionFactories.end(); i++) {
        javascriptExtensions.push_back((*i)->Create());
    }

    WebView *webView;
    webView = webCore->CreateWebView(config->width, config->height, webSession);
    webView->SetTransparent(true);

    if (config->isExposingOBSApi) {
        webView->set_js_method_handler(browserSourceListener);
        webView->set_load_listener(browserSourceListener);
    }

    assert(config->url.Length());

    WebString webString = WebString(reinterpret_cast<const wchar16 *>(config->url.Array()));
    WebURL url(webString);
    webView->LoadURL(url);

    this->hWebView = hWebView;

    browserSize.x = float(config->width);
    browserSize.y = float(config->height);

    EnterCriticalSection(&textureLock);
    
    if (texture) {
        delete texture;
        texture = nullptr;
    }

    texture = CreateTexture(config->width, config->height, GS_BGRA, nullptr, FALSE, FALSE);
    
    LeaveCriticalSection(&textureLock);

    return webView;
}

void BrowserSource::UpdateCallback(WebView *webView)
{
    if (globalSourceRefCount > 0) {
        BitmapSurface *surface = (BitmapSurface *)webView->surface();

        EnterCriticalSection(&textureLock);
        if (surface && texture) {
            texture->SetImage(const_cast<unsigned char *>(surface->buffer()), GS_IMAGEFORMAT_BGRA, surface->row_span());
        }

        LeaveCriticalSection(&textureLock);
    }

}

void BrowserSource::SceneChangeCallback(WebView *webView)
{
    if (config->isExposingOBSApi) {
        webView->ExecuteJavascript(WSLit("OBSSceneChanged()"), WSLit(""));
    }
}

#ifdef INTERACTION_SUPPORT // remove when implemented
void BrowserSource::InteractionCallback(WebView *webView, Interaction &interaction)
{
    webView->Focus();
    if (interaction.type & 0x80) {
        Vect2 scale = interaction.renderSize / browserSize;
        float x = interaction.data.pos.x / scale.x;
        float y = interaction.data.pos.y / scale.y;
        
        if (interaction.type == Interaction::MOUSE_MOVE) {
            webView->InjectMouseMove((int)x, (int)y);
        } else if (interaction.type == Interaction::MOUSE_LBUTTONUP) {
            webView->InjectMouseMove((int)x, (int)y);
            webView->InjectMouseDown(kMouseButton_Left);
            webView->InjectMouseUp(kMouseButton_Left);
        }
    }
}
#endif INTERACTION_SUPPORT // remove when implemented

void BrowserSource::Render(const Vect2 &pos, const Vect2 &size)
{
    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();	

    if (hWebView == NO_VIEW) {
        hWebView = PENDING_VIEW;
        browserManager->AddEvent(new Browser::Event(Browser::CREATE_VIEW, this));
    } else if (hWebView >= 0) {
        EnterCriticalSection(&textureLock);
        if (texture) {
            DrawSprite(texture, 0xFFFFFFFF, pos.x, pos.y, pos.x + size.x, pos.y + size.y);
        }
        LeaveCriticalSection(&textureLock);
    }
}

#ifdef INTERACTION_SUPPORT // remove when implemented
void BrowserSource::ProcessInteraction(Interaction &interaction)
{
    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();
    
    Browser::Event *browserEvent = new Browser::Event(Browser::INTERACTION, this, hWebView);
    browserEvent->info = new Interaction(interaction);
    browserManager->AddEvent(browserEvent);
}
#endif INTERACTION_SUPPORT // remove when implemented

void BrowserSource::ChangeScene() 
{
    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();
    // is this a global source?
    if (hWebView >= 0) {
        browserManager->RunAndWait(Browser::SCENE_CHANGE, this, hWebView);
    }
}

void BrowserSource::UpdateSettings()
{
    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();	

    if (hWebView == NO_VIEW) {
        hWebView = PENDING_VIEW;
    }

    config->Reload();
    browserManager->AddEvent(new Browser::Event(Browser::CREATE_VIEW, this, hWebView));
}

Vect2 BrowserSource::GetSize() const 
{
    return browserSize;
}
