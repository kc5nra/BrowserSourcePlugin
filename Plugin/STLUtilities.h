/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include <string>
#include <vector>
namespace BSP {

    extern void ReplaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace);
    extern std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace);

   

    std::vector<std::string> &Split(const std::string &s, char delim, std::vector<std::string> &elems);
    std::vector<std::string> Split(const std::string &s, char delim); 

    extern std::string IntegerToString(int integer);

    extern void ReplaceStringInPlace(std::wstring& subject, const std::wstring& search, const std::wstring& replace);
    extern std::wstring ReplaceString(std::wstring subject, const std::wstring& search, const std::wstring& replace);
    extern std::wstring IntegerToWString(int integer);

    extern std::string UTF16ToUTF8(std::wstring &utf16String);
    extern std::wstring UTF8ToUTF16(const std::string& utf8String);
}