#ifndef EDITDISTANCE_H
#define EDITDISTANCE_H

#include <string>
#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include "utf8.h"

namespace CATServer{

static double p_Error = 0.005;

static void getEditDistance_CharLevel(const std::vector<int> & dis, const std::wstring &s,
                                      const std::wstring& p, std::vector<int> &prefixConsumed,
                                      std::vector<int> &stateConsumed ){


   int **editCost = (int **)calloc(sizeof (int*), s.length() + 1);
    for(int i = 0; i < s.length() + 1; i++)
        editCost[i] = (int *)calloc(sizeof(int), p.length() + 1);

    for(int i = 0; i < p.length() + 1; i++)
        editCost[0][i] = dis[i];


    for(int i = 1; i < s.length() + 1; i++){
        editCost[i][0] = editCost[i - 1][0] + 1;

        for(int j = 1; j < p.length() + 1; j++){

            if(s[i - 1] == p[j - 1])
                editCost[i][j] = editCost[i - 1][j - 1];
            else
                editCost[i][j] = editCost[i - 1][j - 1] + 1;

            editCost[i][j] = std::min(editCost[i][j], std::min(editCost[i - 1][j] + 1, editCost[i][j - 1] + 1));
        }
    }

    prefixConsumed.assign(s.length() + 1, 0);
    stateConsumed.assign(p.length() + 1, 0);
    for(int i = 0; i < s.length() + 1; i++){
        prefixConsumed[i] = editCost[i][p.length()];
//        cout << prefixConsumed[i] << " ";
    }
//    cout << endl;

    for(int i = 0; i < p.length() + 1; i++)
        stateConsumed[i] = editCost[s.length()][i];

    for(int i = 0; i < s.length() + 1; i++)
        free(editCost[i]);
    free(editCost);
}

static void getEditDistance_wordLevel(const std::vector<std::wstring> &s,
                                      const std::vector<std::wstring> &p, std::vector<int> &prefixConsumed, bool has_partial){


   int **editCost = (int **)calloc(sizeof (int*), s.size() + 1);
    for(int i = 0; i < s.size() + 1; i++)
        editCost[i] = (int *)calloc(sizeof(int), p.size() + 1);

    for(int i = 0; i < p.size() + 1; i++)
        editCost[0][i] = i;


    for(int i = 1; i < s.size() + 1; i++){
        editCost[i][0] = editCost[i - 1][0] + 1;

        for(int j = 1; j < p.size() ; j++){

            if(s[i - 1] == p[j - 1])
                editCost[i][j] = editCost[i - 1][j - 1];
            else
                editCost[i][j] = editCost[i - 1][j - 1] + 1;

            editCost[i][j] = std::min(editCost[i][j], std::min(editCost[i - 1][j] + 1, editCost[i][j - 1] + 1));
        }
        int j = p.size();
        if(!has_partial){

            if(s[i - 1] == p[p.size() - 1])
                editCost[i][p.size()] = editCost[i - 1][p.size() - 1];
            else
                editCost[i][p.size()] = editCost[i - 1][p.size() - 1] + 1;
            editCost[i][j] = std::min(editCost[i][j], std::min(editCost[i - 1][j] + 1, editCost[i][j - 1] + 1));
        }else{

            if(p[j - 1].length() <= s[i - 1].length() && s[i - 1].substr(0, p[j - 1].length()) == p[j - 1] )
                editCost[i][p.size()] = editCost[i - 1][p.size() - 1];
            else
                editCost[i][p.size()] = editCost[i - 1][p.size() - 1] + 1;
            editCost[i][j] = std::min(editCost[i][j], std::min(editCost[i - 1][j] + 1, editCost[i][j - 1] + 1));
        }
    }

    prefixConsumed.assign(s.size() + 1, 0);

    for(int i = 0; i < s.size() + 1; i++){
        prefixConsumed[i] = editCost[i][p.size()];
//        cout << prefixConsumed[i] << " ";
    }
//    cout << endl;

    for(int i = 0; i < s.size() + 1; i++)
        free(editCost[i]);
    free(editCost);
}


static double getBinomialErrorCost(int error, int p_len){

    double res = 0;
    for(int i = 1; i <= error; i++)
        res += log(p_Error);
    for(int i = 1; i <= p_len - error; i++){
        res += (log(i + error) - log((double)i) + log(1. - p_Error));
    }

    return res;

}

static double getLogErrorCost(int error){

    double res = -error;



    return res;

}

static void setErrorProb(double pe){
    p_Error = pe;
}

}

#endif // EDITDISTANCE_H
