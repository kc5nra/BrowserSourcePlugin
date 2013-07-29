/**
* John Bradley (jrb@turrettech.com)
*/

#include "BrowserSource.h"
#include "BrowserSourcePlugin.h"
#include "MimeTypes.h"

//#include <Awesomium\WebCore.h>
//#include <Awesomium\WebView.h>
//#include <Awesomium\WebURL.h>
//#include <Awesomium\STLHelpers.h>
//#include <Awesomium\BitmapSurface.h>
//#include <Awesomium\DataPak.h>
//#include <Awesomium\JSValue.h>
//#include <Awesomium\WebViewListener.h>

//using namespace Awesomium;

#include <Coherent/UI/ViewInfo.h>
#include <d3d10.h>

#define NO_VIEW -2
#define PENDING_VIEW -1

//class BrowserSource::BrowserSourceListener : public WebViewListener::Load, public JSMethodHandler
//{
//
//public:
//    BrowserSourceListener(BrowserSource *browserSource) 
//    {
//        this->browserSource = browserSource;
//    }
//
//    ~BrowserSourceListener() 
//    {
//    }
//
//private:
//    BrowserSource *browserSource;
//
//public: //WebViewListener::Load
//    void OnBeginLoadingFrame(Awesomium::WebView* caller, int64 frameId, bool isMainFrame, const Awesomium::WebURL& url, bool isErrorPage) {}
//    void OnFailLoadingFrame(Awesomium::WebView* caller, int64 frameId, bool isMainFrame, const Awesomium::WebURL& url, int errorCode, const Awesomium::WebString& errorDesc) {}
//    void OnFinishLoadingFrame(Awesomium::WebView* caller, int64 frameId, bool isMainFrame, const Awesomium::WebURL& url) {};
//
//    void 
//    OnMethodCall(
//        Awesomium::WebView* caller, 
//        unsigned int remoteObjectId, 
//        const Awesomium::WebString& methodName,	
//        const Awesomium::JSArray& args)
//    {
//        auto &javascriptExtensions = browserSource->javascriptExtensions;
//        auto functionName = ToString(methodName);
//
//        for(UINT i = 0; i < javascriptExtensions.size(); i++) {
//            if (javascriptExtensions[i]->Handles(NO_RETURN_ARGUMENT, remoteObjectId, functionName)) {
//                javascriptExtensions[i]->Handle(functionName, args);
//                return;
//            }
//        }
//    }
//
//    JSValue 
//    OnMethodCallWithReturnValue(
//        Awesomium::WebView* caller, 
//        unsigned int remoteObjectId, 
//        const Awesomium::WebString& methodName, 
//        const Awesomium::JSArray& args)
//    {
//        auto &javascriptExtensions = browserSource->javascriptExtensions;
//        auto functionName = ToString(methodName);
//
//        for(UINT i = 0; i < javascriptExtensions.size(); i++) {
//            if (javascriptExtensions[i]->Handles(RETURN_ARGUMENT, remoteObjectId, functionName)) {
//                return javascriptExtensions[i]->Handle(functionName, args);
//            }
//        }
//        return JSValue::Undefined();
//    }
//
//    void OnDocumentReady(Awesomium::WebView* caller, const Awesomium::WebURL& url)
//    {
//
//        if (!browserSource->hasRegisteredJavascriptExtensions) {
//            auto &javascriptExtensions = browserSource->javascriptExtensions;
//
//            for (UINT i = 0; i < javascriptExtensions.size(); i++) {
//                javascriptExtensions[i]->Register(caller);
//            }
//            browserSource->hasRegisteredJavascriptExtensions = true;
//        }
//
//        caller->ExecuteJavascript(WSLit("OBSApiReady()"), WSLit(""));
//    }
//
//};

BrowserSource::BrowserSource(XElement *data)
{
	Log(TEXT("Using Browser Source"));

    globalSourceRefCount = -1;
    hasRegisteredJavascriptExtensions = false;

    //browserSourceListener = new BrowserSourceListener(this);
    config = new BrowserSourceConfig(data);

    InitializeCriticalSection(&textureLock);

    UpdateSettings();	
}

BrowserSource::~BrowserSource()
{

    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();

    while(isWaitingForView) {
        browserManager->Update();
        Sleep(20);
    }
    browserManager->ShutdownAndWait(this);

    //delete browserSourceListener;

    if (texture) {
        texture = nullptr;
    }

    delete config;
   /* for (UINT i = 0; i < dataSources.size(); i++) {
        delete dataSources[i];
    }
    dataSources.clear();
    
    for (UINT i = 0; i < javascriptExtensions.size(); i++) {
        delete javascriptExtensions[i];
    }
    javascriptExtensions.clear();*/
    
    DeleteCriticalSection(&textureLock);

    config = nullptr;
}

void BrowserSource::Tick(float fSeconds)
{
    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();

    if (webView != nullptr && globalSourceRefCount != 0) {
        browserManager->AddEvent(new Browser::Event(Browser::UPDATE, this, fSeconds));
    } else if (isWaitingForView) {
        browserManager->Update();
    }
}

void BrowserSource::GlobalSourceEnterScene()
{
    if (globalSourceRefCount == -1) {
        globalSourceRefCount = 1;
    } else {
        globalSourceRefCount++;
    }
}

void BrowserSource::GlobalSourceLeaveScene()
{
    globalSourceRefCount--;
}

void BrowserSource::CreateWebViewCallback(Coherent::UI::UISystem *system)
{

    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();

    if (webView) {
        webView->Destroy();
        webView = nullptr;
    }

    Coherent::UI::ViewInfo viewInfo;
    viewInfo.Width = config->width;
    viewInfo.Height = config->height;
    viewInfo.IsTransparent = true;

    browserSize.x = float(config->width);
    browserSize.y = float(config->height);

    EnterCriticalSection(&textureLock);
    
    if (texture) {
        texture = nullptr;
    }

    LeaveCriticalSection(&textureLock);

    system->CreateView(viewInfo, config->url.Array(), static_cast<Coherent::UI::ViewListenerBase *>(this));
}
//{
//
//    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();
//
//    WebPreferences webPreferences;
//    if (config->customCss.Length()) {
//
//        WebString customCss;
//        char *utf8str = config->customCss.CreateUTF8String();
//        int len = tstr_to_utf8_datalen(config->customCss);
//
//        if (utf8str) {
//            customCss = WebString::CreateFromUTF8(utf8str, len);
//            Free(utf8str);
//        }
//   
//        webPreferences.user_stylesheet = customCss;
//    }
//    webPreferences.enable_web_gl = true;
//    webPreferences.allow_file_access_from_file_url = true;
//    webPreferences.allow_universal_access_from_file_url = true;
//    webPreferences.allow_running_insecure_content = true;
//    webPreferences.enable_web_security = false;
//
//    WebSession *webSession;
//    webSession = webCore->CreateWebSession(WSLit("plugins\\BrowserSourcePlugin\\cache"), webPreferences);
//  
//    for (UINT i = 0; i < dataSources.size(); i++) {
//        delete dataSources[i];
//    }
//    dataSources.clear();
//
//    std::string assetWrapTemplate;
//    
//    char *utf8str = config->assetWrapTemplate.CreateUTF8String();
//    if (utf8str) {
//        assetWrapTemplate = utf8str;
//        Free(utf8str);
//    }
//
//    dataSources.push_back(new BrowserDataSource(config->isWrappingAsset, assetWrapTemplate, config->width, config->height));
//    dataSources.push_back(new BlankDataSource(config->isWrappingAsset, assetWrapTemplate, config->width, config->height));
//
//    for(UINT i = 0; i < dataSources.size(); i++) {
//        int mimeTypeCount = sizeof(mimeTypes) / sizeof(CTSTR);
//        for(int j = 0; j < mimeTypeCount; j += 2) {
//            dataSources[i]->AddMimeType(mimeTypes[j], mimeTypes[j+1]);
//        }
//        webSession->AddDataSource(dataSources[i]->GetHost(), dataSources[i]);
//    }
//
//    for (UINT i = 0; i < javascriptExtensions.size(); i++) {
//        delete javascriptExtensions[i];
//    }
//
//    javascriptExtensions.clear();
//    hasRegisteredJavascriptExtensions = false;
//
//    auto &javascriptExtensionFactories = browserManager->GetJavascriptExtensionFactories();
//    
//    for(auto i = javascriptExtensionFactories.begin(); i < javascriptExtensionFactories.end(); i++) {
//        javascriptExtensions.push_back((*i)->Create());
//    }
//
//    WebView *webView;
//    webView = webCore->CreateWebView(config->width, config->height, webSession);
//    webView->SetTransparent(true);
//
//    if (config->isExposingOBSApi) {
//        webView->set_js_method_handler(browserSourceListener);
//        webView->set_load_listener(browserSourceListener);
//    }
//
//    assert(config->url.Length());
//
//    WebString webString = WebString(reinterpret_cast<const wchar16 *>(config->url.Array()));
//    WebURL url(webString);
//    webView->LoadURL(url);
//
//    this->hWebView = hWebView;
//
//    browserSize.x = float(config->width);
//    browserSize.y = float(config->height);
//
//    EnterCriticalSection(&textureLock);
//    
//    if (texture) {
//        delete texture;
//        texture = nullptr;
//    }
//
//    texture = CreateTexture(config->width, config->height, GS_BGRA, nullptr, FALSE, FALSE);
//    
//    LeaveCriticalSection(&textureLock);
//
//    return webView;
//}

//void BrowserSource::UpdateCallback(WebView *webView)
//{
//    /*BitmapSurface *surface = (BitmapSurface *)webView->surface();
//
//    EnterCriticalSection(&textureLock);
//    if (surface && texture) {
//        texture->SetImage(const_cast<unsigned char *>(surface->buffer()), GS_IMAGEFORMAT_BGRA, surface->row_span());
//    }
//
//    LeaveCriticalSection(&textureLock);*/
//
//}

//void BrowserSource::SceneChangeCallback(WebView *webView)
//{
//    /*if (config->isExposingOBSApi) {
//        webView->ExecuteJavascript(WSLit("OBSSceneChanged()"), WSLit(""));
//    }*/
//}

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

    if (!isWaitingForView) {
        if (webView == nullptr) {
            browserManager->AddEvent(new Browser::Event(Browser::CREATE_VIEW, this));
        } else {
            if (globalSourceRefCount != 0) {
                EnterCriticalSection(&textureLock);
                if (texture) {
                    DrawSprite(texture, 0xFFFFFFFF, pos.x, pos.y, pos.x + size.x, pos.y + size.y);
                }
                LeaveCriticalSection(&textureLock);
            }
        }
    }
}

void BrowserSource::OnViewCreated(Coherent::UI::View *view)
{
    this->webView = view;
    isWaitingForView = false;
}

void BrowserSource::CreateSurface(bool sharedMemory, unsigned width, unsigned height, Coherent::UI::SurfaceResponse* response)
{

    D3D10_TEXTURE2D_DESC desc = { 0 };
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D10_USAGE_DEFAULT;
        desc.MiscFlags = D3D10_RESOURCE_MISC_SHARED;
        desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;

    ID3D10Texture2D* sharedTexture = NULL;
    ID3D10Device *d3d10device = static_cast<ID3D10Device *>(GS->GetDevice());
    
    d3d10device->CreateTexture2D( &desc, NULL, &sharedTexture);
    
    IDXGIResource* pDXGIResource = NULL;
    sharedTexture->QueryInterface(__uuidof(IDXGIResource), (LPVOID*) &pDXGIResource);
    
    HANDLE sharedHandle;
    
    pDXGIResource->GetSharedHandle(&sharedHandle);
    pDXGIResource->Release();
        
    Texture *textureBuffer = GS->CreateTextureFromSharedHandle(width, height, sharedHandle);

    Coherent::UI::CoherentHandle sharedCoherentHandle(sharedHandle);
    handleToTextureMap.insert(std::make_pair(sharedCoherentHandle, textureBuffer));

    response->Signal(sharedCoherentHandle);
}

void BrowserSource::DestroySurface(Coherent::UI::CoherentHandle handle, bool usesSharedMemory)
{
    auto buffer = handleToTextureMap.find(handle);
    if (buffer != handleToTextureMap.end()) {
        delete buffer->second;
        handleToTextureMap.erase(buffer);
    }
}

void BrowserSource::OnDraw(Coherent::UI::CoherentHandle handle, bool usesSharedMemory, int width, int height)
{
    EnterCriticalSection(&textureLock);

    auto buffer = handleToTextureMap.find(handle);
    if (buffer != handleToTextureMap.end()) {
        texture = buffer->second;
    }

    LeaveCriticalSection(&textureLock);
}

void BrowserSource::OnURLRequest(const wchar_t* url, Coherent::UI::URLResponse* response)
{
    
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

//void BrowserSource::ChangeScene() 
//{
//    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();
//    // is this a global source?
//    if (hWebView >= 0) {
//        browserManager->RunAndWait(Browser::SCENE_CHANGE, this, hWebView);
//    }
//}

void BrowserSource::UpdateSettings()
{
    BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();	

    isWaitingForView = true;

    config->Reload();
    browserManager->AddEvent(new Browser::Event(Browser::CREATE_VIEW, this));
}

Vect2 BrowserSource::GetSize() const 
{
    return browserSize;
}
