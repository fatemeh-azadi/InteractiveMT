#include "searchnbest.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <algorithm>
#include <boost/range/iterator_range_core.hpp>
//#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <cmath>
#include <stdlib.h>
#include <vector>

using namespace std;
using namespace CATServer;
using namespace Moses;

namespace CATServer{


void SearchNBest::setNbests(const std::string &str){

    istringstream in(str);
    string x;
    int id = 0;
    while(getline(in, x) != 0){

        vector<string> v = splitLine(x);

        wstring s = L"";
        decode_utf8(v[1], s);
//        cout << id++ << " ||| " << v[1] << " ||| " << v[3] << endl;
        Translation t (id++, s, atof(v[3].c_str()));
        nbest.push_back(t);


    }

//    cout << nbest.size() << endl;
}

vector<string> SearchNBest::splitLine(string s){

    vector<string> res;
    typedef boost::split_iterator<string::iterator> string_split_iterator;
       for(string_split_iterator It=
           boost::make_split_iterator(s, first_finder(" ||| ", boost::is_iequal()));
           It!=string_split_iterator();
           ++It)
       {
           res.push_back(boost::copy_range<std::string>(*It));
       }

    return res;
}

string SearchNBest::searchPrefix_EC_Model(string prefix){

    wstring p;
    decode_utf8(prefix, p);
    vector<int> d;
    for(int i = 0; i < p.length() + 1; i++)
        d.push_back(i);

    float bestScore = 0.;
    int bestAlignment = 0;
    int bestNBest = -1;


    for(int i = 0; i < nbest.size(); i++){

        vector<int> prefixConsumed, stateConsumed;

        getEditDistance_CharLevel(d, nbest[i].str, p, prefixConsumed, stateConsumed);

        int alignment = getBestAlignment(prefixConsumed);
        int error = prefixConsumed[alignment];

        float score = nbest[i].fscore + errorWeight * getBinomialErrorCost(error, p.length());
//        float score = (1 - errorWeight) * nbest[i].fscore + errorWeight * getLogErrorCost(error);

        if(bestNBest == -1 || score > bestScore){
            bestScore = score;
            bestNBest = i;
            bestAlignment = alignment;
        }



    }

    cout << "selected NBest : " << bestNBest << " " << bestAlignment << endl;
    wstring tmp = nbest[bestNBest].str.substr(bestAlignment);
    string res;
    encode_utf8(tmp, res);

    return res;
}




string SearchNBest::searchPrefix_WordLevel(string prefix){


    wstring p;
    decode_utf8(prefix, p);

    vector< wstring > pr_words = tokenize(p);


    float bestScore = 0.;
    int bestAlignment = 0;
    int bestNBest = -1;

    bool hasPartial = false;

    if(prefix.length() > 0 && (prefix[prefix.length() - 1] != ' '))
        hasPartial = true;

    for(int i = 0; i < nbest.size(); i++){

        vector<int> prefixConsumed;

        vector<wstring> nbest_words = tokenize(nbest[i].str);

        getEditDistance_wordLevel(nbest_words, pr_words, prefixConsumed, hasPartial);

        int alignment = getBestAlignment(prefixConsumed);
        int error = prefixConsumed[alignment];

        float score = nbest[i].fscore - errorWeight * error;
//        float score = nbest[i].fscore + errorWeight * getBinomialErrorCost(error, pr_words.size());
//        float score = (1 - errorWeight) * nbest[i].fscore + errorWeight * getLogErrorCost(error);

        if(bestNBest == -1 || score > bestScore){
            bestScore = score;
            bestNBest = i;
            bestAlignment = alignment;
        }



    }

    cout << "selected NBest : " << bestNBest << " " << bestAlignment << endl;

    wstring tmp = L"";
    vector<wstring> nbest_words = tokenize(nbest[bestNBest].str);

    if(!hasPartial){
        for(int i = bestAlignment; i < nbest_words.size(); i++){
            if(tmp.length() > 0)
                tmp += L" ";
            tmp += nbest_words[i];
        }
    }else{

        if(bestAlignment > 0 && nbest_words[bestAlignment - 1].substr(0,pr_words[pr_words.size() - 1].length()) == pr_words[pr_words.size() - 1])

            tmp += nbest_words[bestAlignment - 1].substr(pr_words[pr_words.size() - 1].length());

        for(int i = bestAlignment; i < nbest_words.size(); i++){
            tmp += L" ";
            tmp += nbest_words[i];
        }

    }


    string res;
    encode_utf8(tmp, res);

    return res;
}


string SearchNBest::searchPrefix_TER(string prefix){

    wstring p;
    decode_utf8(prefix, p);

    vector< wstring > pr_words = tokenize(p);

    float bestScore = 0.;
    int bestAlignment = 0;
    int bestNBest = -1;

    bool hasPartial = false;
    TERCalc terCalc;

    if(prefix.length() > 0 && (prefix[prefix.length() - 1] != ' '))
        hasPartial = true;

    vector<wstring> bestShiftedNBest;
    for(int i = 0; i < nbest.size(); i++){

        vector<int> prefixConsumed;

        vector<wstring> nbest_words = tokenize(nbest[i].str);


//        cout << "NBest :::: ";
//        for(int i = 0; i < nbest_words.size(); i++){
//            string x;
//            encode_utf8(nbest_words[i], x);
//            cout << x << " ";
//        }
//        cout << endl;
        vector<wstring> shiftedNBest = terCalc.calcMinEdits(nbest_words, pr_words);


//        cout << "Shifted :::: ";
//        for(int i = 0; i < shiftedNBest.size(); i++){
//            string x;
//            encode_utf8(shiftedNBest[i], x);
//            cout << x << " ";
//        }
//        cout << endl;

        getEditDistance_wordLevel(shiftedNBest, pr_words, prefixConsumed, hasPartial);

        int alignment = getBestAlignment(prefixConsumed);
        int error = prefixConsumed[alignment];

        float score = nbest[i].fscore - errorWeight * error;
//        float score = nbest[i].fscore + errorWeight * getBinomialErrorCost(error, pr_words.size());
//        float score = (1 - errorWeight) * nbest[i].fscore + errorWeight * getLogErrorCost(error);

        if(bestNBest == -1 || score > bestScore){
            bestScore = score;
            bestNBest = i;
            bestAlignment = alignment;
            bestShiftedNBest = shiftedNBest;
        }



    }

    cout << "selected NBest : " << bestNBest << " " << bestAlignment << endl;

    wstring tmp = L"";
    vector<wstring> nbest_words = bestShiftedNBest;

    if(!hasPartial){
        for(int i = bestAlignment; i < nbest_words.size(); i++){
            if(tmp.length() > 0)
                tmp += L" ";
            tmp += nbest_words[i];
        }
    }else{

        if(bestAlignment > 0 && nbest_words[bestAlignment - 1].substr(0,pr_words[pr_words.size() - 1].length()) == pr_words[pr_words.size() - 1])

            tmp += nbest_words[bestAlignment - 1].substr(pr_words[pr_words.size() - 1].length());

        for(int i = bestAlignment; i < nbest_words.size(); i++){
            tmp += L" ";
            tmp += nbest_words[i];
        }

    }


    string res;
    encode_utf8(tmp, res);

    return res;

}

int SearchNBest::getBestAlignment(const vector<int> &v){

    int best = 0, bestDistance = v[0];
    for(int i = 0; i < v.size(); i++){
        if(v[i] < bestDistance){
            best = i;
            bestDistance = v[i];
        }

    }

    return best;
}

}
