/**
* John Bradley (jrb@turrettech.com)
*/

#include "BrowserSource.h"
#include "BrowserSourcePlugin.h"
#include "SwfReader.h"
#include "DataSources.h"
#include "MimeTypes.h"
#include "KeyboardManager.h"

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

class BrowserSource::BrowserSourceListener : public WebViewListener::Load, public JSMethodHandler, public KeyboardListener
{
    friend class SceneItem;
public:
    BrowserSourceListener(BrowserSource *browserSource) 
    {
        this->browserSource = browserSource;
        InitializeCriticalSection(&keyLock);
    }

    ~BrowserSourceListener() 
    {
        DeleteCriticalSection(&keyLock);
    }

private:
    BrowserSource *browserSource;
    List<Key::Key> keyboardEvents;
    CRITICAL_SECTION keyLock;
    bool isAcceptingKeyboardEvents;

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
        if (methodName == WSLit("setSceneItem")) {
            if (args.size() == 1 && args[0].IsObject()) {
                JSObject object = args[0].ToObject();
                if (!object.HasProperty(WSLit("name"))) {
                    Log(TEXT("invalid set request for scene item, object must have name property"));
                    return;
                }

                String name = ToAPIString(object.GetProperty(WSLit("name")).ToString());

                //API->EnterSceneMutex();

                SceneItem *sceneItem = API->GetScene()->GetSceneItem(name);
                if (sceneItem) {
                    bool validObject = true;
                    Vect2 position;
                    Vect2 size;

                    if (validObject &= object.HasProperty(WSLit("x"))) {
                        position.x = (float)object.GetProperty(WSLit("x")).ToDouble(); 
                    }
                    if (validObject &= object.HasProperty(WSLit("y"))) {
                        position.y = (float)object.GetProperty(WSLit("y")).ToDouble(); 
                    }
                    if (validObject &= object.HasProperty(WSLit("width"))) {
                        size.x = (float)object.GetProperty(WSLit("width")).ToDouble(); 
                    }
                    if (validObject &= object.HasProperty(WSLit("height"))) {
                        size.y = (float)object.GetProperty(WSLit("height")).ToDouble(); 
                    }

                    if (validObject) {
                        sceneItem->SetPos(position);
                        sceneItem->SetSize(size);
                    }

                } else {
                    Log(TEXT("invalid set request for scene item, scene with name %s not found"), name);
                }

                //API->LeaveSceneMutex();
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
        if (remoteObjectId == browserSource->hJSGlobal) {
            if (methodName == WSLit("getScene")) {
            } else if (methodName == WSLit("getSceneItem")) {
                if (args.size() == 1) {
                    if (args[0].IsString()) {
                        API->EnterSceneMutex();

                        Scene *scene = API->GetScene();
                        String &string = ToAPIString(args[0].ToString());
                        SceneItem *sceneItem = scene->GetSceneItem(string);
                        JSObject jsObject;
                        jsObject.SetProperty(WSLit("name"), args[0].ToString());
                        jsObject.SetProperty(WSLit("index"), JSValue((int)sceneItem->GetID()));
                        jsObject.SetProperty(WSLit("x"), JSValue(sceneItem->GetPos().x));
                        jsObject.SetProperty(WSLit("y"), JSValue(sceneItem->GetPos().y));
                        jsObject.SetProperty(WSLit("width"), JSValue(sceneItem->GetSize().x));
                        jsObject.SetProperty(WSLit("height"), JSValue(sceneItem->GetSize().y));

                        API->LeaveSceneMutex();

                        return jsObject;
                    }
                }
            }
        }

        return JSValue::Undefined();
    }

    void OnDocumentReady(Awesomium::WebView* caller, const Awesomium::WebURL& url)
    {
        EnterCriticalSection(&browserSource->jsGlobalLock);
        JSValue value = caller->CreateGlobalJavascriptObject(WSLit("OBSApi"));
        if (value.IsObject()) {
            JSObject object = value.ToObject();
            object.SetCustomMethod(WSLit("getSceneItem"), true);
            object.SetCustomMethod(WSLit("setSceneItem"), false);
            browserSource->hJSGlobal = object.remote_id();
        } else {
            Log(TEXT("error creating javascript api object (%d)"), caller->last_error());
        }
        caller->ExecuteJavascript(WSLit("OBSApiReady()"), WSLit(""));
        LeaveCriticalSection(&browserSource->jsGlobalLock);
    }

    void KeyboardEvent(Key::Key &key)
    {
        EnterCriticalSection(&keyLock);
        keyboardEvents.Add(key);
        LeaveCriticalSection(&keyLock);
    }

    JSArray GetKeyEvents() 
    {
        EnterCriticalSection(&keyLock);
        JSArray jsArray;
        while(keyboardEvents.Num())
        {
            Key::Key &key = keyboardEvents[0];
            JSArray args;
            args.Push(JSValue(key.alt));
            args.Push(JSValue(key.shift));
            args.Push(JSValue(key.control));
            args.Push(JSValue(key.capslock));
            args.Push(JSValue(key.type));
            args.Push(JSValue((int)key.vkCode));
            jsArray.Push(args);
            keyboardEvents.Remove(0);
        }
        LeaveCriticalSection(&keyLock);
        return jsArray;
    }

    void ClearKeyEvents()
    {
        EnterCriticalSection(&keyLock);
        keyboardEvents.Clear();
        LeaveCriticalSection(&keyLock);
    }
};

BrowserSource::BrowserSource(XElement *data)
{
    Log(TEXT("Using Browser Source"));

    hWebView = -2;

    config = new BrowserSourceConfig(data);
    browserSourceListener = new BrowserSourceListener(this);

    InitializeCriticalSection(&textureLock);
    InitializeCriticalSection(&jsGlobalLock);

    UpdateSettings();	
}

BrowserSource::~BrowserSource()
{

    KeyboardManager *keyboardManager = BrowserSourcePlugin::instance->GetBrowserManager()->GetKeyboardManager();
    keyboardManager->RemoveListener(browserSourceListener);

    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();
    browserManager->ShutdownAndWait(this);

    delete browserSourceListener;

    EnterCriticalSection(&textureLock);
    delete texture;
    texture = NULL;
    LeaveCriticalSection(&textureLock);

    delete config;
    for (UINT i = 0; i < dataSources.Num(); i++) {
        delete dataSources[i];
    }

    dataSources.Clear();

    DeleteCriticalSection(&textureLock);
    DeleteCriticalSection(&jsGlobalLock);

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
    WebPreferences webPreferences;
    WebString webString = WebString(reinterpret_cast<wchar16 *>(config->customCss.Array()));
    webPreferences.user_stylesheet = webString;
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

    WebView *webView;
    webView = webCore->CreateWebView(config->width, config->height, webSession);
    webView->SetTransparent(true);

    KeyboardManager *keyboardManager = BrowserSourcePlugin::instance->GetBrowserManager()->GetKeyboardManager();

    keyboardManager->RemoveListener(browserSourceListener);
    browserSourceListener->ClearKeyEvents();

    if (config->isExposingOBSApi && config->hasKeyboardEventListener)
    {    
        keyboardManager->AddListener(browserSourceListener);
    }

    if (config->isExposingOBSApi) {
        webView->set_js_method_handler(browserSourceListener);
        webView->set_load_listener(browserSourceListener);
    }

    webString = WebString(reinterpret_cast<const wchar16 *>(config->url.Array()));
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
