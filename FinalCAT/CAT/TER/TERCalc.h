#ifndef TERCALC_H
#define TERCALC_H
#include <vector>
#include <string>
#include <map>
#include <stdio.h>
#include <cstdlib>
#include "TERShift.h"
#include <memory.h>
#include <iostream>
#include <algorithm>

namespace CATServer {
#define Sequence std::vector<std::wstring>
const static double INF = 99999999;
const static double BEAM_WIDTH = 20;
const static int MAX_SHIFT_SIZE = 10;
const int MAX_SHIFT_DIST = 50;

class TERCalc
{

        // used for minimum-Edit-Distance
//    int ** dp;
//    int ** path;

    std::vector<std::vector<int> > dp;
    std::vector<std::vector<int> > path;
    int dp_len;
public:
    TERCalc();
    int minEditDistance(const std::vector<std::wstring> &hyp, const std::vector<std::wstring> &ref);

    std::vector<int> getPath(const std::vector<std::wstring> &hyp, const std::vector<std::wstring> &ref);
    Sequence calcMinEdits(const std::vector<std::wstring> &hyp, const std::vector<std::wstring> &ref);
    std::map<Sequence, std::vector<int> > buildWordMatches(const Sequence &hyp, const Sequence &ref);

    Sequence performShift(const Sequence &hyp, TERShift shift);

    void findAlignErr(const std::vector<int> &path, std::vector<bool> &herr,
                      std::vector<bool> &rerr, std::vector<int> &ralign);
    void gatherPossShifts(const Sequence &hyp, const Sequence &ref,
                         const std::map<Sequence, std::vector<int> > &matchedRefSubstrs,
                          std::vector<bool> &herr, std::vector<bool> &rerr,
                          std::vector<int> &ralign, std::vector<std::vector<TERShift> > &res);

    int calcBestShift(const Sequence &hyp, const Sequence &ref,
                      const std::map<Sequence, std::vector<int> > &matchedRefSubstrs,
                      const std::vector<int> &bestPath, int curEdits,
                      Sequence &bestShift, std::vector<int> &newBestPath);

    Sequence getSubSequence(Sequence s, int start, int end);
    double getTER(const std::vector<std::wstring> &hyp, const std::vector<std::wstring> &ref);
};
}
#endif // TERCALC_H
