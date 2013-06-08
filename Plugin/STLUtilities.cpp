/**
* John Bradley (jrb@turrettech.com)
*/
#include "STLUtilities.h"

#include <sstream>
#include <codecvt>

void BSP::ReplaceStringInPlace(
    std::string& subject, 
    const std::string& search,
    const std::string& replace) 
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

std::string BSP::ReplaceString(
    std::string subject, 
    const std::string& search,
    const std::string& replace) 
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}

std::vector<std::string> &BSP::Split(
    const std::string &s, 
    char delim, 
    std::vector<std::string> &elems) 
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> BSP::Split(
    const std::string &s, 
    char delim) 
{
    std::vector<std::string> elems;
    BSP::Split(s, delim, elems);
    return elems;
}

std::string BSP::IntegerToString(int integer)
{
    std::stringstream s;
    s << integer;
    return s.str();
}