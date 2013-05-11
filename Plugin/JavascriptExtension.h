/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include <Awesomium\WebView.h>
#include <Awesomium\JSArray.h>
#include <Awesomium\WebString.h>
#include <Awesomium\STLHelpers.h>
#include <Awesomium\WebStringArray.h>

using namespace Awesomium;

enum JavascriptFunctionType {
    NO_RETURN_ARGUMENT,
    RETURN_ARGUMENT
};

class JavascriptExtension {

protected:
    WebString globalObjectName;
    unsigned int remoteObjectId;
   
    WebStringArray noReturnArgumentFunctions;
    WebStringArray returnArgumentFunctions;

public:
    JavascriptExtension(WebString &globalObjectName) 
    {
        this->globalObjectName = globalObjectName;
        remoteObjectId = 0;
    }

    virtual ~JavascriptExtension() {};

public:
    virtual bool Register(WebView *webView) 
    {
        JSValue &value = webView->CreateGlobalJavascriptObject(globalObjectName);
        if (value.IsObject()) {
            JSObject object = value.ToObject();
            
            remoteObjectId = object.remote_id();

            for(unsigned int i = 0; i < noReturnArgumentFunctions.size(); i++) {
                object.SetCustomMethod(noReturnArgumentFunctions[i], false);
            }
            for(unsigned int i = 0; i < returnArgumentFunctions.size(); i++) {
                object.SetCustomMethod(returnArgumentFunctions[i], true);
            }

            return true;
        }
        return false;
    }

    virtual bool Handles(const enum JavascriptFunctionType functionType, const unsigned int remoteObjectId, const WebString &functionName) {
        if (this->remoteObjectId == remoteObjectId) {
            WebStringArray *functionArray = NULL;
            switch (functionType) {
                case NO_RETURN_ARGUMENT:
                {
                    functionArray = &noReturnArgumentFunctions;
                    break;
                }
                case RETURN_ARGUMENT:
                {
                    functionArray = &returnArgumentFunctions;
                    break;
                }
            }

            if (functionArray) {
                for(unsigned int i = 0; i < functionArray->size(); i++) {
                    if (functionArray->At(i) == functionName) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    virtual JSValue Handle(const WebString &functionName, const JSArray &args)=0;
};

class JavascriptExtensionFactory {
public:
    virtual JavascriptExtension *Create()=0;
};

