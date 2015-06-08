#ifndef WORDVECTOR_H
#define WORDVECTOR_H

#include<vector>
#include<string>
#include <map>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <iostream>
#include <stdlib.h>
#include <boost/algorithm/string.hpp>

namespace CaitraWordVector{

class WordVector
{

public:
    static const long long max_size = 2000;         // max length of strings
    static const long long N = 40;                  // number of closest words that will be shown
    static const long long max_w = 100;              // max length of vocabulary entries
    long long words, size;
    float *M;
    char *vocab;
    std::map<std::string, std::map<std::string, float> > cosineDistance;
    std::map<std::string, long long> dic;
public:
    void readWordVec(std::string vectorsFile);
    WordVector();
    ~WordVector(){ free(M); free(vocab); }
    void getVector(std::string s, std::vector<std::string> &wordVec, std::vector<float> &distVec);
    void findVector(std::string s);
    float getDistance(std::string s1, std::string s2);
    std::string lowercase(std::string mixedcase);
    long long getIndex(std::string s);
    float getDistance(long long s1, long long s2);
    std::vector<int> getVocabStartsWith(std::string str);


    inline std::string getWord(long long x){
        char *tmp = (char *)malloc(max_size * sizeof(char));
        strcpy(tmp, &vocab[x * max_w]);
        return std::string(tmp);
    }


};

}

#endif // WORDVECTOR_H
