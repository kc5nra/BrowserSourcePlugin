/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "OBSApi.h"

//#include "Awesomium\JSObject.h"
#include <vector>
#include <map>
#include <Coherent/UI/ViewListener.h>
struct BrowserSourceConfig;

namespace Coherent 
{
    namespace UI 
    {
        class UISystem;
    }
}

class BrowserSource : public ImageSource, public Coherent::UI::ViewListener
{
    //class BrowserSourceListener;

public:
    BrowserSource(XElement *data);
    ~BrowserSource();

private:
    Vect2 browserSize;
    Texture *texture;
    bool isWaitingForView;
    Coherent::UI::View *webView;

    bool hasRegisteredJavascriptExtensions;

    unsigned int hJSGlobal;

    //BrowserSourceListener *browserSourceListener;

    //std::vector<JavascriptExtension *> javascriptExtensions;
    //std::vector<DataSourceWithMimeType *> dataSources;

    BrowserSourceConfig *config;

    int globalSourceRefCount;
    
    std::map<Coherent::UI::CoherentHandle, Texture*> handleToTextureMap;
    
    CRITICAL_SECTION textureLock;
protected:
    CRITICAL_SECTION jsGlobalLock;

public:
    // ImageSource
    void Tick(float fSeconds);
    void Render(const Vect2 &pos, const Vect2 &size);
    void GlobalSourceEnterScene();
    void GlobalSourceLeaveScene();

#ifdef INTERACTION_SUPPORT // remove when implemented
    void ProcessInteraction(Interaction &interaction);
#endif INTERACTION_SUPPORT // remove when implemented

    void ChangeScene();
    void UpdateSettings();
    Vect2 GetSize() const;
    Coherent::UI::View *GetWebView() { return webView; }
    
public:
    // callbacks
    void CreateWebViewCallback(Coherent::UI::UISystem *system);

    void OnViewCreated(Coherent::UI::View *view);
    void CreateSurface(bool sharedMemory, unsigned width, unsigned height, Coherent::UI::SurfaceResponse *response);
    void DestroySurface(Coherent::UI::CoherentHandle surface, bool usesSharedMemory);
    void OnDraw(Coherent::UI::CoherentHandle handle, bool usesSharedMemory, int width, int height);
    void OnURLRequest(const wchar_t* url, Coherent::UI::URLResponse* response);

#ifdef INTERACTION_SUPPORT // remove when implemented
    void BrowserSource::InteractionCallback(Awesomium::WebView *webView, Interaction &interaction);
#endif INTERACTION_SUPPORT // remove when implemented

};