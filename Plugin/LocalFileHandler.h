/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

//#include <Awesomium\WebString.h>
//#include <Awesomium\DataSource.h>
//#include <Awesomium\WebString.h>
//#include <Awesomium\STLHelpers.h>

#include <Coherent/UI/FileHandler.h>
#include <Coherent/UI/URLParse.h>

#include "STLUtilities.h"

#include <cctype>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

class LocalFileHandler : public Coherent::UI::FileHandler
{

public:
	/// Requests to read a resource
	/// @param url the coui url of the resource.
	/// @note The url has been decoded - any \%xx have been replaced by their original characters.
	/// @param response the response to be filled with the resource data or notified for failure
	void ReadFile(const wchar_t* url, Coherent::UI::ResourceResponse* response)
    {
        Coherent::UI::URLComponent scheme;
        Coherent::UI::URLComponent path;
        Coherent::UI::URLComponent query;
        Coherent::UI::URLComponent fragment;
        
        std::string filePath;
        
        if ((CoherentGetURLParser().Parse(url, &scheme, &path, &query, &fragment)) && path.Start)
		{
			filePath.append(std::string(path.Start, path.Start + path.Length));
            std::vector<std::string> elements;
            BSP::Split(filePath, '/', elements);

            if (elements.size() > 0) {
                if (elements[0] == "local") {
                    filePath = filePath.substr(6);
                }
            }
		} else {
            filePath.append(std::string(url, url + wcslen(url)));
        }

        std::ifstream ifs;
		ifs.open(filePath, std::ifstream::binary);
		
        void *buffer = nullptr;
        bool fileExistsAndReadable = false;

        if (fileExistsAndReadable = ifs.good()) {
			ifs.seekg(0, std::ifstream::end);
			size_t fileSize = (size_t)ifs.tellg();
            
            buffer = response->GetBuffer(fileSize);
            if (buffer != nullptr) {
                ifs.seekg(0, std::ifstream::beg);
			    ifs.read(reinterpret_cast<char *>(buffer), fileSize);
            }
        }
        
        ifs.close();

        if (!fileExistsAndReadable || buffer == nullptr) {
            response->SignalFailure();
            return;
        }
        		
        response->SignalSuccess();
    }

	/// Request to write to a resource
	/// @param url the coui url of the resource.
	/// @note The url has been decoded - any \%xx have been replaced by their original characters.
	/// @param resource the resource data that has to be written
	void WriteFile(const wchar_t* url, Coherent::UI::ResourceData* resource)
    {
        resource->SignalFailure();
    }
};
//
//class DataSourceWithMimeType : public DataSource 
//{
//
//public:
//    virtual ~DataSourceWithMimeType() {};
//
//protected:
//    std::map<std::string, std::string> mimeTypes;
//
//public:
//    void AddMimeType(const std::string &mimeType, const std::string &fileType) 
//    {
//        // keyed by file extension
//        mimeTypes[fileType] = mimeType;
//    }
//
//    virtual WebString GetHost() = 0;
//};
//
//
//
//class BlankDataSource : public DataSourceWithMimeType 
//{
//
//private:
//    bool isWrappingAsset;
//    std::string assetWrapTemplate;
//    int width;
//    int height;
//
//public:
//    BlankDataSource(bool isWrappingAsset, const std::string &assetWrapTemplate, int width, int height) 
//    { 
//        this->isWrappingAsset = isWrappingAsset;
//        this->assetWrapTemplate = assetWrapTemplate;
//        this->width = width;
//        this->height = height;
//    }
//
//public:
//    virtual void OnRequest(int request_id, const WebString& path) {
//
//        if (isWrappingAsset) {
//            isWrappingAsset = false;
//
//		    BSP::ReplaceStringInPlace(assetWrapTemplate, "$(WIDTH)", BSP::IntegerToString(width));
//		    BSP::ReplaceStringInPlace(assetWrapTemplate, "$(HEIGHT)", BSP::IntegerToString(height));
//
//            SendResponse(request_id,
//			    assetWrapTemplate.length(),
//                (unsigned char *)assetWrapTemplate.c_str(),
//                WSLit("text/html"));
//        } else {
//            
//            std::string filePath = ToString(path);
//		    filePath = filePath.substr(0, filePath.find('?'));
//
//		    std::ifstream ifs;
//		    ifs.open(filePath, std::ifstream::binary);
//		    std::vector<char> data;
//		    if (ifs.good()) {
//			    ifs.seekg(0, std::ifstream::end);
//			    size_t file_size_in_byte = (size_t)ifs.tellg();
//			    data.resize(file_size_in_byte);
//			    ifs.seekg(0, std::ifstream::beg);
//			    ifs.read(&data[0], file_size_in_byte);
//		    }
//		    ifs.close();
//
//            std::string mimeType = "text/html";
//
//		    // chop off ext and lower case it
//            std::string fileExtension = filePath.substr(filePath.find_last_of(".") + 1);
//		    std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), std::tolower);
//
//		    auto itor = mimeTypes.find(fileExtension);
//		    if (itor != mimeTypes.end()) {
//			    mimeType = itor->second;
//		    }
//
//            SendResponse(request_id,
//                data.size(),
//               (data.size()) ? (unsigned char *)&data[0] : NULL,
//                ToWebString(mimeType));
//        }
//    }
//
//public: //DataSourceWithMimeType
//
//    WebString GetHost() 
//    {
//        return WSLit("blank");
//    }
//};
//
//class BrowserDataSource : public DataSourceWithMimeType
//{
//
//private:
//    bool isWrappingAsset;
//    std::string assetWrapTemplate;
//    int width;
//    int height;
//
//public:
//    BrowserDataSource(bool isWrappingAsset, const std::string &assetWrapTemplate, int width, int height) 
//    { 
//        this->isWrappingAsset = isWrappingAsset;
//        this->assetWrapTemplate = assetWrapTemplate;
//        this->width = width;
//        this->height = height;
//    }
//
//public:
//    virtual void OnRequest(int request_id, const WebString& path) 
//    {
//        
//    	std::string filePath = ToString(path);
//		filePath = filePath.substr(0, filePath.find('?'));
//
//        if (isWrappingAsset) {
//            isWrappingAsset = false;
//
//            std::string fileName = filePath.substr(filePath.find_last_of("/") + 1);
//
//			BSP::ReplaceStringInPlace(assetWrapTemplate, "$(FILE)", fileName);
//			BSP::ReplaceStringInPlace(assetWrapTemplate, "$(WIDTH)", BSP::IntegerToString(width));
//            BSP::ReplaceStringInPlace(assetWrapTemplate, "$(HEIGHT)", BSP::IntegerToString(height));
//
//            //TODO: Figure out what to do with this information
//            // Since a lot of flash is vector art, it ends up 
//            // making it super blurry if the actual size
//            // is pretty small.
//
//            //SwfReader swfReader((unsigned char *)lpFileDataUTF8);
//            //if (!swfReader.HasError()) {
//            //	swfWidth = swfReader.GetWidth();
//            //	swfHeight = swfReader.GetHeight();
//            //}
//
//            SendResponse(request_id,
//                strlen(assetWrapTemplate.c_str()),
//                (unsigned char *)assetWrapTemplate.c_str(),
//                WSLit("text/html"));
//
//        } else {
//
//            std::ifstream ifs;
//		    ifs.open(filePath, std::ifstream::binary);
//		    std::vector<char> data;
//		    if (ifs.good()) {
//			    ifs.seekg(0, std::ifstream::end);
//			    size_t file_size_in_byte = (size_t)ifs.tellg();
//			    data.resize(file_size_in_byte);
//			    ifs.seekg(0, std::ifstream::beg);
//			    ifs.read(&data[0], file_size_in_byte);
//		    }
//		    ifs.close();
//
//            std::string mimeType = "text/html";
//
//		    // chop off ext and lower case it
//            std::string fileExtension = filePath.substr(filePath.find_last_of(".") + 1);
//		    std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), std::tolower);
//
//		    auto itor = mimeTypes.find(fileExtension);
//		    if (itor != mimeTypes.end()) {
//			    mimeType = itor->second;
//		    }
//
//            SendResponse(request_id,
//                data.size(),
//               (data.size()) ? (unsigned char *)&data[0] : NULL,
//                ToWebString(mimeType));
//        }
//    }
//
//
//public: //DataSourceWithMimeType
//
//    WebString GetHost() 
//    {
//        return WSLit("local");
//    }
//};