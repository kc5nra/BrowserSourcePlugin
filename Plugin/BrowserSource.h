/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "OBSApi.h"

#include "Awesomium\JSObject.h"
#include <vector>

namespace Awesomium {
    class WebCore;
    class WebSession;
    class WebView;
    class JSArray;
    class JSValue;
    class JSMethodHandler;
}

class DataSourceWithMimeType;
class JavascriptExtension;

struct BrowserSourceConfig;

class BrowserSource : public ImageSource
{
    class BrowserSourceListener;

public:
    BrowserSource(XElement *data);
    ~BrowserSource();

private:
    Vect2 browserSize;
    Texture *texture;
    int id;
    int hWebView;
    bool hasRegisteredJavascriptExtensions;
    volatile bool isInScene;
    unsigned int hJSGlobal;

    BrowserSourceListener *browserSourceListener;

    std::vector<JavascriptExtension *> javascriptExtensions;
    std::vector<DataSourceWithMimeType *> dataSources;

    BrowserSourceConfig *config;

    CRITICAL_SECTION textureLock;
protected:
    CRITICAL_SECTION jsGlobalLock;

public:
    // ImageSource
    void Tick(float fSeconds);
    void Render(const Vect2 &pos, const Vect2 &size);

#ifdef INTERACTION_SUPPORT // remove when implemented
    void ProcessInteraction(Interaction &interaction);
#endif INTERACTION_SUPPORT // remove when implemented

    void GlobalSourceEnterScene();
    void GlobalSourceLeaveScene();
    void ChangeScene();

    void UpdateSettings();
    Vect2 GetSize() const;



public:
    int GetWebView() { return hWebView; }


public:
    // callbacks
    Awesomium::WebView *CreateWebViewCallback(Awesomium::WebCore *webCore, const int hWebView);
    void UpdateCallback(Awesomium::WebView *webView);
    void SceneChangeCallback(Awesomium::WebView *webView);

#ifdef INTERACTION_SUPPORT // remove when implemented
    void BrowserSource::InteractionCallback(Awesomium::WebView *webView, Interaction &interaction);
#endif INTERACTION_SUPPORT // remove when implemented

};