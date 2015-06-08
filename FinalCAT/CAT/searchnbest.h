#ifndef SEARCHNBEST_H
#define SEARCHNBEST_H
#include "moses-cmd/TranslationAnalysis.h"
#include "moses-cmd/IOWrapper.h"
#include "moses/TrellisPath.h"
#include <string>
#include <vector>
#include <ostream>
#include <sstream>
#include "utf8.h"
#include "editdistance.h"
#include "TER/TERCalc.h"

namespace CATServer{

class Translation{

public:
    std::wstring str;
    float fscore;

    int id;

    Translation(int i, std::wstring s, float f){

        id = i;
        str = s;
        fscore = f;

    }

};

class SearchNBest
{
   const static double errorWeight = 1.5;
    std::vector<Translation> nbest;

public:

    SearchNBest(){}

    void setNbests(const std::string &str);
    std::vector<std::string> splitLine(std::string s);
    std::string searchPrefix_EC_Model(std::string prefix);
    std::string searchPrefix_WordLevel(std::string prefix);
    std::string searchPrefix_TER(std::string prefix);
    int getBestAlignment(const std::vector<int> &v);


    inline std::vector<std::wstring> tokenize(const std::wstring& str)
    {
        const std::wstring& delimiters = L" \t\n\r";
        std::vector<std::wstring> tokens;
        // Skip delimiters at beginning.
        std::wstring::size_type lastPos = str.find_first_not_of(delimiters, 0);
        // Find first "non-delimiter".
        std::wstring::size_type pos     = str.find_first_of(delimiters, lastPos);

        while (std::wstring::npos != pos || std::wstring::npos != lastPos)
        {
            // Found a token, add it to the vector.
            tokens.push_back(str.substr(lastPos, pos - lastPos));
            // Skip delimiters.  Note the "not_of"
            lastPos = str.find_first_not_of(delimiters, pos);
            // Find next "non-delimiter"
            pos = str.find_first_of(delimiters, lastPos);
        }
        return tokens;
    }
};

}
#endif // SEARCHNBEST_H
