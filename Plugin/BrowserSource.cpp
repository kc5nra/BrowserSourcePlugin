/**
* John Bradley (jrb@turrettech.com)
*/

#include "BrowserSource.h"
#include "BrowserSourcePlugin.h"
#include "SwfReader.h"
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
    friend class SceneItem;
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
        List<JavascriptExtension *> &javascriptExtensions = browserSource->javascriptExtensions;

        for(UINT i = 0; i < javascriptExtensions.Num(); i++) {
            if (javascriptExtensions[i]->Handles(NO_RETURN_ARGUMENT, remoteObjectId, methodName)) {
                javascriptExtensions[i]->Handle(methodName, args);
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
        List<JavascriptExtension *> &javascriptExtensions = browserSource->javascriptExtensions;

        for(UINT i = 0; i < javascriptExtensions.Num(); i++) {
            if (javascriptExtensions[i]->Handles(RETURN_ARGUMENT, remoteObjectId, methodName)) {
                return javascriptExtensions[i]->Handle(methodName, args);
            }
        }
        return JSValue::Undefined();
    }

    void OnDocumentReady(Awesomium::WebView* caller, const Awesomium::WebURL& url)
    {

        if (!browserSource->hasRegisteredJavascriptExtensions) {
            List<JavascriptExtension *> &javascriptExtensions = browserSource->javascriptExtensions;

            for (UINT i = 0; i < javascriptExtensions.Num(); i++) {
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

    delete texture;
    texture = NULL;

    delete config;
    for (UINT i = 0; i < dataSources.Num(); i++) {
        delete dataSources[i];
    }
    dataSources.Clear();
    
    for (UINT i = 0; i < javascriptExtensions.Num(); i++) {
        delete javascriptExtensions[i];
    }
    javascriptExtensions.Clear();
    
    DeleteCriticalSection(&textureLock);

    config = NULL;
}

void BrowserSource::Tick(float fSeconds)
{
    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();

    if (hWebView == PENDING_VIEW) {
        browserManager->Update();
    } else if (hWebView >= 0) {
        browserManager->AddEvent(new Browser::Event(Browser::UPDATE, this, hWebView));
    }
}

WebView *BrowserSource::CreateWebViewCallback(WebCore *webCore, const int hWebView) 
{

    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();

    WebPreferences webPreferences;
    if (config->customCss.Length()) {
        WebString webString = WebString(reinterpret_cast<wchar16 *>(config->customCss.Array()));
        webPreferences.user_stylesheet = webString;
    }
    webPreferences.enable_web_gl = true;
    WebSession *webSession;
    webSession = webCore->CreateWebSession(WSLit("plugins\\BrowserSourcePlugin\\cache"), webPreferences);
  
    for (UINT i = 0; i < dataSources.Num(); i++) {
        delete dataSources[i];
    }
    dataSources.Clear();

    dataSources.Add(new BrowserDataSource(config->isWrappingAsset, config->assetWrapTemplate, config->width, config->height));
    dataSources.Add(new BlankDataSource(config->isWrappingAsset, config->assetWrapTemplate, config->width, config->height));

    for(UINT i = 0; i < dataSources.Num(); i++) {
        int mimeTypeCount = sizeof(mimeTypes) / sizeof(CTSTR);
        for(int j = 0; j < mimeTypeCount; j += 2) {
            dataSources[i]->AddMimeType(mimeTypes[j], mimeTypes[j+1]);
        }
        webSession->AddDataSource(dataSources[i]->GetHost(), dataSources[i]);
    }

    for (UINT i = 0; i < javascriptExtensions.Num(); i++) {
        delete javascriptExtensions[i];
    }

    javascriptExtensions.Clear();
    hasRegisteredJavascriptExtensions = false;

    List<JavascriptExtensionFactory *> &javascriptExtensionFactories = browserManager->GetJavascriptExtensionFactories();

    for(UINT i = 0; i < javascriptExtensionFactories.Num(); i++) {
        javascriptExtensions.Add(javascriptExtensionFactories[i]->Create());
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
        texture = 0;
    }

    texture = CreateTexture(config->width, config->height, GS_BGRA, NULL, FALSE, FALSE);
    LeaveCriticalSection(&textureLock);

    return webView;
}

void BrowserSource::UpdateCallback(WebView *webView)
{
    BitmapSurface *surface = (BitmapSurface *)webView->surface();

    EnterCriticalSection(&textureLock);
    if (surface && texture) {
        texture->SetImage(const_cast<unsigned char *>(surface->buffer()), GS_IMAGEFORMAT_BGRA, surface->row_span());
    }

    LeaveCriticalSection(&textureLock);

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
