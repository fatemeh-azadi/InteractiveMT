#include "moses/Util.h"
#include "moses/TypeDef.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/Hypothesis.h"
#include "moses/Factor.h"
#include "moses/TrellisPath.h"
#include "moses/TrellisPathList.h"
#include "moses/Manager.h"
#include "OnlineLearning/Optimiser.h"
#include <sstream>
#include "TER/TERCalc.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include "utf8.h"
#include "editdistance.h"
#ifndef ONLINELEARNER_H
#define ONLINELEARNER_H

using namespace std;
using namespace Moses;
using namespace Optimizer;
using namespace CATServer;
typedef std::map<std::string, std::map<std::string, float> > pp_feature;
typedef std::map<std::string, std::map<std::string, int> > pp_list;
typedef float learningrate;


class OnlineLearner
{
private:
    std::string implementation;
    pp_feature m_feature;
    pp_list m_featureIdx;
    pp_list PP_ORACLE, PP_BEST;
    learningrate flr, wlr;
    int nBestSize;
    float m_weight;
    int m_PPindex;
    std::string m_postedited;
    bool m_learn, m_normaliseScore;
    MiraOptimiser* optimiser;
    std::string m_method;
    std::string m_metric;
public:
    OnlineLearner();
    OnlineLearner(float wlr, int nbsize, string method, string metric);
    void RunOnlineLearning(Manager *manager);
    void PrintHypo(const Hypothesis* hypo, ostream& HypothesisStringStream);
    bool SetPostEditedSentence(std::string s);
    void RunOldMira(Manager* manager);
    bool has_only_spaces(const std::string& str);
    float GetBleu(std::string hypothesis, std::string reference);
    void compareNGrams(map<string, int>& hyp, map<string, int>& ref, map<int, float>& countNgrams, map<int, float>& TotalNgrams);
    int getNGrams(std::string str, map<string, int>& ngrams);
    int split_marker_perl(string str, string marker, vector<string> &array);
    void RemoveJunk();
    void RunMIRA(Manager *manager);
    void getNBest(Manager *manager, vector<string> &HypothesisList, vector<float> &modelScores, vector<ScoreComponentCollection> &featureValues, std::vector<float> &BleuScores);

    int findPrediction(const vector<float> &modelScores, const vector<float> &cost);
    int findOracle(const vector<float> &modelScores, const vector<float> &cost);
    double GetTER(string hyp, string ref);
    double GetWER(string hyp, string ref);
    double GetKSMR(string hyp, string ref);
    wstring getBestSuffix(wstring prefix, wstring hyp);

    void RunPerceptron(Manager *manager);
    void RunPA2(Manager *manager);
};

#endif // ONLINELEARNER_H
