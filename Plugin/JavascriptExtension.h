/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include <Awesomium\WebView.h>
#include <Awesomium\JSArray.h>
#include <Awesomium\WebString.h>
#include <Awesomium\STLHelpers.h>
#include <Awesomium\WebStringArray.h>

#include <set>

using namespace Awesomium;

enum JavascriptFunctionType {
    NO_RETURN_ARGUMENT,
    RETURN_ARGUMENT
};

class JavascriptExtension {

protected:
    std::string globalObjectName;
    unsigned int remoteObjectId;
   
    std::set<std::string> noReturnArgumentFunctions;
    std::set<std::string> returnArgumentFunctions;

public:
    JavascriptExtension(std::string globalObjectName) 
    {
        this->globalObjectName = globalObjectName;
        remoteObjectId = 0;
    }

    virtual ~JavascriptExtension() {};

public:
    virtual bool Register(WebView *webView) 
    {
        JSValue &value = webView->CreateGlobalJavascriptObject(ToWebString(globalObjectName));
        if (value.IsObject()) {
            JSObject object = value.ToObject();
            
            remoteObjectId = object.remote_id();
            for(auto i = noReturnArgumentFunctions.begin(); i != noReturnArgumentFunctions.end(); i++) {
                object.SetCustomMethod(ToWebString(*i), false);
            }
            for(auto i = returnArgumentFunctions.begin(); i != returnArgumentFunctions.end(); i++) {
                object.SetCustomMethod(ToWebString(*i), true);
            }
           
            return true;
        }
        return false;
    }

    virtual bool Handles(const enum JavascriptFunctionType functionType, const unsigned int remoteObjectId, const std::string &functionName) {
        if (this->remoteObjectId == remoteObjectId) {
            std::set<std::string> *functionArray = NULL;
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
                if (functionArray->find(functionName) != functionArray->end()) {
                    return true;
                }
            }
        }

        return false;
    }

    virtual JSValue Handle(const std::string &functionName, const JSArray &args)=0;
};

class JavascriptExtensionFactory {
public:
    virtual JavascriptExtension *Create()=0;
};

