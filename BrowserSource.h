/**
 * John Bradley (jrb@turrettech.com)
 */
#pragma once

#include "OBSApi.h"

namespace Awesomium {
	class WebCore;
	class WebSession;
	class WebView;
}

class BrowserSource : public ImageSource
{

public:
    BrowserSource(XElement *data);
    ~BrowserSource();

private:
	Vect2 size;
	Texture *texture;
	Awesomium::WebCore *webCore;
	Awesomium::WebSession *webSession;
	Awesomium::WebView *webView;

public:
    void Tick(float fSeconds);
    void DrawBitmap(UINT texID, float alpha, const Vect2 &startPos, const Vect2 &startSize);
    void Render(const Vect2 &pos, const Vect2 &size);
    void UpdateSettings();
    Vect2 GetSize() const;
};