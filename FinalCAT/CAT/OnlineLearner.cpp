#include "OnlineLearner.h"

OnlineLearner::OnlineLearner()
{
    nBestSize = 100;
    wlr = 0.1;
    optimiser = new MiraOptimiser();

}

OnlineLearner::OnlineLearner(float wl, int nbsize, string method, string metric)
{
    m_method = method;
    m_metric = metric;
    nBestSize = nbsize;
    wlr = wl;
    optimiser = new MiraOptimiser();
}

int OnlineLearner::split_marker_perl(string str, string marker, vector<string> &array) {
    int found = str.find(marker), prev = 0;
    while (found != string::npos) // warning!
    {
        if(found > prev)
            array.push_back(str.substr(prev, found - prev));
        prev = found + marker.length();
        found = str.find(marker, found + marker.length());
    }
    if(prev < str.length())
    array.push_back(str.substr(prev));
     /************bug*************/
    return ((int)array.size());
}

int OnlineLearner::getNGrams(std::string str, map<string, int>& ngrams) {
    vector<string> words;
    int numWords = split_marker_perl(str, " ", words);
    for (int i = 1; i <= 4; i++) {
        for (int start = 0; start < numWords - i + 1; start++) {
            string str = "";
            for (int pos = 0; pos < i; pos++) {
                str += words[start + pos] + " ";
            }
            ngrams[str]++;
        }
    }
    //return str.size();
    /************bug*************/
    return numWords;
}

void OnlineLearner::compareNGrams(map<string, int>& hyp, map<string, int>& ref, map<int, float>& countNgrams, map<int, float>& TotalNgrams) {
    for (map<string, int>::const_iterator itr = hyp.begin(); itr != hyp.end(); itr++) {
        vector<string> temp;
        int ngrams = split_marker_perl((*itr).first, " ", temp);
        TotalNgrams[ngrams] += hyp[(*itr).first];
        if (ref.find((*itr).first) != ref.end()) {
            /************bug*************/
            if (hyp[(*itr).first] <= ref[(*itr).first]) {
                countNgrams[ngrams] += hyp[(*itr).first];
            } else {
                countNgrams[ngrams] += ref[(*itr).first];
            }
        }
    }
    for (int i = 1; i <= 4; i++) {
        TotalNgrams[i] += 0.1;
        countNgrams[i] += 0.1;
    }
    return;
}

float OnlineLearner::GetBleu(std::string hypothesis, std::string reference) {
    double bp = 1;
    map<string, int> hypNgrams, refNgrams;
    map<int, float> countNgrams, TotalNgrams;
    map<int, double> BLEU;
    int length_translation = getNGrams(hypothesis, hypNgrams);
    int length_reference = getNGrams(reference, refNgrams);
    compareNGrams(hypNgrams, refNgrams, countNgrams, TotalNgrams);

    for (int i = 1; i <= 4; i++) {
        BLEU[i] = (countNgrams[i]*1.0) / (TotalNgrams[i]*1.0);
    }
    double ratio = ((length_reference * 1.0 + 1.0) / (length_translation * 1.0 + 1.0));
    if (length_translation < length_reference)
        bp = exp(1 - ratio);
    return ((bp * exp((log(BLEU[1]) + log(BLEU[2]) + log(BLEU[3]) + log(BLEU[4])) / 4))*100.);
}

double OnlineLearner::GetTER(string hyp, string ref){
    vector<string> h, r;
    boost::split(h, hyp, boost::is_any_of(" "));
    boost::split(r, ref, boost::is_any_of(" "));
    vector<wstring> h2, r2;
    for(int i = 0; i < h.size(); i++){
        if(h[i] == "")
            continue;
        wstring x;
        decode_utf8(h[i], x);
        h2.push_back(x);

    }

    for(int i = 0; i < r.size(); i++){
        if(r[i] == "")
            continue;
        wstring x;
        decode_utf8(r[i], x);
        r2.push_back(x);
    }

    CATServer::TERCalc ter;
    return ter.getTER(h2, r2)*100.;

}

double OnlineLearner::GetWER(string hyp, string ref){
    vector<string> h, r;
    boost::split(h, hyp, boost::is_any_of(" "));
    boost::split(r, ref, boost::is_any_of(" "));


    vector<wstring> h2, r2;
    for(int i = 0; i < h.size(); i++){
        if(h[i] == "")
            continue;
        wstring x;
        decode_utf8(h[i], x);
        h2.push_back(x);

    }

    for(int i = 0; i < r.size(); i++){
        if(r[i] == "")
            continue;
        wstring x;
        decode_utf8(r[i], x);
        r2.push_back(x);
    }
    vector<int> rConsumed;
    getEditDistance_wordLevel(h2, r2, rConsumed, 0);

    return rConsumed[h2.size()]/(double)r2.size()*100.;

}

wstring OnlineLearner::getBestSuffix(wstring prefix, wstring hyp){
    bool hasPartial = 1;
    if(prefix.length() == 0)
        return hyp;
    if(prefix[prefix.size() -1] == L' ')
        hasPartial = 0;

    vector<wstring> h, p;
    boost::split(h, hyp, boost::is_any_of(L" "));
    boost::split(p, prefix, boost::is_any_of(L" "));
    if(p[p.size() - 1] == L"")
        p.pop_back();
    vector<int> prefixConsumed;
    getEditDistance_wordLevel(h, p, prefixConsumed, hasPartial);

    int minEdits = h.size()+p.size(), idx = -1;
    for(int i = 0; i < prefixConsumed.size(); i++){
        if(hasPartial && p.size() > 0){
            int k = p[p.size() - 1].length();
            if(i > 0 && prefixConsumed[i] < minEdits && h[i - 1].substr(0, k) == p[p.size() - 1]){
                idx = i;
                minEdits = prefixConsumed[i];
            }
        }
        else if(prefixConsumed[i] < minEdits){
            minEdits = prefixConsumed[i];
            idx = i;
        }
    }

    wstring res = L"";

    if(idx == -1)
        return res;
    if(hasPartial && p.size()){
        int k = p[p.size() - 1].length();

        res += h[idx - 1].substr(k);

    }
    for(int i = idx; i < h.size(); i++){
        if(res.length() > 0)
            res += L" ";
        res += h[i];
    }
    return res;

}

double OnlineLearner::GetKSMR(string hyp, string ref){


    wstring p = L"";
    wstring h, r, s = L"";
    decode_utf8(hyp, h);
    decode_utf8(ref, r);

    int ks = 0, ma = 0, idx = 0;
    while(idx < ref.length()){
        p = r.substr(0, idx);
        s = getBestSuffix(p, h);
        int i = 0;
        while (idx < r.length() && i < s.length()
                && r[idx] == s[i]){
            idx++;
            i++;
        }
        if(i > 0)
            ma++;
        ks++;
        idx++;

    }

    double ksmr = (ks + ma) / (double)r.length() * 100;
    return ksmr;

}


// print the best translation in HypothesisStringStream
// and mark all the phrase-pairs in the best path at PP_BEST
void OnlineLearner::PrintHypo(const Hypothesis* hypo, ostream& HypothesisStringStream) {
    if (hypo->GetPrevHypo() != NULL) {
        PrintHypo(hypo->GetPrevHypo(), HypothesisStringStream);
        Phrase p = hypo->GetCurrTargetPhrase();
        for (size_t pos = 0; pos < p.GetSize(); pos++) {
            const Factor *factor = p.GetFactor(pos, 0);
            HypothesisStringStream << *factor << " ";
        }
        std::string sourceP = hypo->GetSourcePhraseStringRep();
        std::string targetP = hypo->GetTargetPhraseStringRep();

        PP_BEST[sourceP][targetP] = 1;

    }
}

bool OnlineLearner::has_only_spaces(const std::string& str) {
    return (str.find_first_not_of(' ') == str.npos);
}

void OnlineLearner::RunOnlineLearning(Manager *manager) {

    if(m_method == "Mira"){
        RunMIRA(manager);

    }else if(m_method == "OldMira")
        RunOldMira(manager);
    else if(m_method == "Perceptron")
        RunPerceptron(manager);
    else if(m_method == "PA2")
        RunPA2(manager);

}


void OnlineLearner::RunMIRA(Manager *manager){

    const StaticData& staticData = StaticData::Instance();
    ScoreComponentCollection weightUpdate = staticData.GetAllWeights();


    cerr << "Online Learner  ...... " << endl;
    cerr << "Post Edit       : " << m_postedited << endl;


    std::string bestOracle;
    std::vector<string> HypothesisList;
    std::vector<float> losses, cost, modelScores;

    std::vector<ScoreComponentCollection> featureValues;

    getNBest(manager, HypothesisList, modelScores, featureValues, cost);
    int yPlus = findOracle(modelScores, cost);
    int yMinus = findPrediction(modelScores, cost);

    cerr << "yPlus -> modelScore : " << modelScores[yPlus] << "   ,   cost : " << cost[yPlus] << endl;
    cerr << "yMinus -> modelScore : " << modelScores[yMinus] << "   ,   cost : " << cost[yMinus] << endl;
    double margin = modelScores[yMinus] - modelScores[yPlus];
    double costt = cost[yMinus] - cost[yPlus];
    double loss = margin + costt;
    if(loss > 0){

        ScoreComponentCollection diff = featureValues[yPlus];
        diff.MinusEquals(featureValues[yMinus]);

        double delta = min(0.001, loss / diff.InnerProduct(diff));
        diff.MultiplyEquals(delta);
        diff.MultiplyEquals(wlr);
        weightUpdate.PlusEquals(diff);
    }
    StaticData::InstanceNonConst().SetAllWeights(weightUpdate);

}

void OnlineLearner::RunPerceptron(Manager *manager){

    const StaticData& staticData = StaticData::Instance();
    ScoreComponentCollection weightUpdate = staticData.GetAllWeights();


    cerr << "Online Learner  ...... " << endl;
    cerr << "Post Edit       : " << m_postedited << endl;


    std::string bestOracle;
    std::vector<string> HypothesisList;
    std::vector<float> losses, cost, modelScores;

    std::vector<ScoreComponentCollection> featureValues;

    getNBest(manager, HypothesisList, modelScores, featureValues, cost);
    int yPlus = findOracle(modelScores, cost);
    int yMinus = findPrediction(modelScores, cost);

    cerr << "yPlus -> modelScore : " << modelScores[yPlus] << "   ,   cost : " << cost[yPlus] << endl;
    cerr << "yMinus -> modelScore : " << modelScores[yMinus] << "   ,   cost : " << cost[yMinus] << endl;



        ScoreComponentCollection diff = featureValues[yPlus];
        diff.MinusEquals(featureValues[yMinus]);

        FVector v = diff.GetScoresVector();
        for(int i = 0; i < v.size(); i++){
            if(v[i] > 0)
                diff.Assign(i, 1);
            else if(v[i] < 0)
                diff.Assign(i, -1);
            else
                diff.Assign(i, 0);

        }

        diff.MultiplyEquals(wlr);
        weightUpdate.MultiplyEquals(1 - wlr);
        weightUpdate.PlusEquals(diff);

    StaticData::InstanceNonConst().SetAllWeights(weightUpdate);

}

void OnlineLearner::RunPA2(Manager *manager){

    const StaticData& staticData = StaticData::Instance();
    ScoreComponentCollection weightUpdate = staticData.GetAllWeights();


    cerr << "Online Learner  ...... " << endl;
    cerr << "Post Edit       : " << m_postedited << endl;


    std::string bestOracle;
    std::vector<string> HypothesisList;
    std::vector<float> losses, cost, modelScores;

    std::vector<ScoreComponentCollection> featureValues;

    getNBest(manager, HypothesisList, modelScores, featureValues, cost);
    int yPlus = findOracle(modelScores, cost);
    int yMinus = findPrediction(modelScores, cost);

    cerr << "yPlus -> modelScore : " << modelScores[yPlus] << "   ,   cost : " << cost[yPlus] << endl;
    cerr << "yMinus -> modelScore : " << modelScores[yMinus] << "   ,   cost : " << cost[yMinus] << endl;
    double margin = modelScores[yPlus] - modelScores[yMinus];
    double loss = sqrt(abs(cost[yMinus] - cost[yPlus]));
    if(margin < loss){

        ScoreComponentCollection diff = featureValues[yPlus];
        diff.MinusEquals(featureValues[yMinus]);

        double C = 100;
        double delta = (loss - margin) / (diff.InnerProduct(diff) + C);
        diff.MultiplyEquals(delta);
        diff.MultiplyEquals(wlr);

        weightUpdate.MultiplyEquals(1 - wlr);
        weightUpdate.PlusEquals(diff);

        StaticData::InstanceNonConst().SetAllWeights(weightUpdate);
    }

}


int OnlineLearner::findOracle(const vector<float> &modelScores, const vector<float> &cost){
    int res = 0;

    for(int i = 0; i < modelScores.size(); i++){
        if( - cost[i] >  - cost[res])
            res = i;
    }

    return res;
}

int OnlineLearner::findPrediction(const vector<float> &modelScores, const vector<float> &cost){

    int res = 0;

    for(int i = 0; i < modelScores.size(); i++){
        if(modelScores[i]  > modelScores[res] )
            res = i;
    }

    return res;
}

void OnlineLearner::getNBest(Manager *manager, vector<string> &HypothesisList, vector<float> &modelScores, vector<ScoreComponentCollection> &featureValues, vector<float> &BleuScores){

    const StaticData& staticData = StaticData::Instance();
    const std::vector<Moses::FactorType>& outputFactorOrder = staticData.GetOutputFactorOrder();

    TrellisPathList nBestList;
    manager->CalcNBest(nBestSize, nBestList, true);


    TrellisPathList::const_iterator iter;

    float oracleScore = 0.0;
    int whichoracle = -1;

    for (iter = nBestList.begin(); iter != nBestList.end(); ++iter) {

        whichoracle++;

        const TrellisPath &path = **iter;
        const std::vector<const Hypothesis *> &edges = path.GetEdges();

        stringstream oracle;
        for (int currEdge = (int) edges.size() - 1; currEdge >= 0; currEdge--) {
            const Hypothesis &edge = *edges[currEdge];
            // CHECK(outputFactorOrder.size() > 0);
            size_t size = edge.GetCurrTargetPhrase().GetSize();
            for (size_t pos = 0; pos < size; pos++) {
                const Factor *factor = edge.GetCurrTargetPhrase().GetFactor(pos, outputFactorOrder[0]);
                oracle << *factor;
                oracle << " ";
            }
        }

        oracleScore = path.GetTotalScore();
        HypothesisList.push_back(oracle.str());

        if(m_metric == "BLEU"){
        float oraclebleu = GetBleu(oracle.str(), m_postedited);
        BleuScores.push_back(1. - oraclebleu);
        }else if(m_metric == "TER"){

            BleuScores.push_back(GetTER(oracle.str(), m_postedited));
        }else if(m_metric == "WER")
            BleuScores.push_back(GetWER(oracle.str(), m_postedited));
        else if(m_metric == "KSMR")
            BleuScores.push_back(GetKSMR(oracle.str(), m_postedited));
        else{
            cerr << "Error : invalid Metric" << endl;
            exit(1);
        }

        featureValues.push_back(path.GetScoreBreakdown());
        modelScores.push_back(oracleScore);

    }

}

void OnlineLearner::RunOldMira(Manager *manager) {

    const StaticData& staticData = StaticData::Instance();
    const std::vector<Moses::FactorType>& outputFactorOrder = staticData.GetOutputFactorOrder();
    /*
    const TranslationSystem &trans_sys = StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT);


        std::vector<const ScoreProducer*> sps = trans_sys.GetFeatureFunctions();
        ScoreProducer* sp = const_cast<ScoreProducer*> (sps[0]);
        for (int i = 0; i < sps.size(); i++) {
            if (sps[i]->GetScoreProducerDescription().compare("OnlineLearner") == 0) {
                sp = const_cast<ScoreProducer*> (sps[i]);
                break;
            }
        }
        m_weight = weightUpdate.GetScoreForProducer(sp); // permanent weight stored in decoder
      */
    ScoreComponentCollection weightUpdate = staticData.GetAllWeights();

    const Hypothesis* hypo = manager->GetBestHypothesis();

    stringstream bestHypothesis;
    PP_BEST.clear();
    PrintHypo(hypo, bestHypothesis);
    float bestbleu = GetBleu(bestHypothesis.str(), m_postedited);
    float bestScore = hypo->GetScore();
    cerr << "Online Learner  ...... " << endl;
    cerr << "Best Hypothesis : " << bestHypothesis.str() << endl;
    cerr << "Post Edit       : " << m_postedited << endl;
    TrellisPathList nBestList;
    manager->CalcNBest(nBestSize, nBestList, true);

    std::string bestOracle;
    std::vector<string> HypothesisList, HypothesisHope, HypothesisFear;
    std::vector<float> loss, BleuScore, BleuScoreHope, BleuScoreFear, oracleBleuScores, lossHope, lossFear, modelScore, oracleModelScores;
    std::vector<std::vector<float> > losses, BleuScores, BleuScoresHope, BleuScoresFear, lossesHope, lossesFear, modelScores;
    std::vector<ScoreComponentCollection> featureValue, featureValueHope, featureValueFear, oraclefeatureScore;
    std::vector<std::vector<ScoreComponentCollection> > featureValues, featureValuesHope, featureValuesFear;
    std::map<int, map<string, map<string, int> > > OracleList;
    TrellisPathList::const_iterator iter;
    pp_list BestOracle, ShootemUp, ShootemDown, Visited;
    float maxBleu = 0.0, maxScore = 0.0, oracleScore = 0.0;
    int whichoracle = -1;

    for (iter = nBestList.begin(); iter != nBestList.end(); ++iter) {

        whichoracle++;
        const TrellisPath &path = **iter;
        PP_ORACLE.clear();
        const std::vector<const Hypothesis *> &edges = path.GetEdges();
        stringstream oracle;
        for (int currEdge = (int) edges.size() - 1; currEdge >= 0; currEdge--) {
            const Hypothesis &edge = *edges[currEdge];
            // CHECK(outputFactorOrder.size() > 0);
            size_t size = edge.GetCurrTargetPhrase().GetSize();
            for (size_t pos = 0; pos < size; pos++) {
                const Factor *factor = edge.GetCurrTargetPhrase().GetFactor(pos, outputFactorOrder[0]);
                oracle << *factor;
                oracle << " ";
            }
            std::string sourceP = edge.GetSourcePhraseStringRep(); // Source Phrase
            std::string targetP = edge.GetTargetPhraseStringRep(); // Target Phrase
            if (!has_only_spaces(sourceP) && !has_only_spaces(targetP)) {
                PP_ORACLE[sourceP][targetP] = 1; // phrase pairs in the current nbest_i
                OracleList[whichoracle][sourceP][targetP] = 1; // list of all phrase pairs given the nbest_i
            }
        }

        oracleScore = path.GetTotalScore();
        float oraclebleu = GetBleu(oracle.str(), m_postedited);

        // if (implementation != FOnlyPerceptron) {
        HypothesisList.push_back(oracle.str());
        BleuScore.push_back(oraclebleu);
        featureValue.push_back(path.GetScoreBreakdown());
        modelScore.push_back(oracleScore);
        // }

        if (oraclebleu > maxBleu) {
            cerr << "NBEST : " << oracle.str() << "\t|||\tBLEU : " << oraclebleu << endl;
            maxBleu = oraclebleu;
            maxScore = oracleScore;
            bestOracle = oracle.str();
            /*
            pp_list::const_iterator it1;
            ShootemUp.clear();
            ShootemDown.clear();
            for (it1 = PP_ORACLE.begin(); it1 != PP_ORACLE.end(); it1++) {
                std::map<std::string, int>::const_iterator itr1;
                for (itr1 = (it1->second).begin(); itr1 != (it1->second).end(); itr1++) {
                    if (PP_BEST[it1->first][itr1->first] != 1) {
                        ShootemUp[it1->first][itr1->first] = 1;
                    }
                }
            }
            for (it1 = PP_BEST.begin(); it1 != PP_BEST.end(); it1++) {
                std::map<std::string, int>::const_iterator itr1;
                for (itr1 = (it1->second).begin(); itr1 != (it1->second).end(); itr1++) {
                    if (PP_ORACLE[it1->first][itr1->first] != 1) {
                        ShootemDown[it1->first][itr1->first] = 1;
                    }
                }
            }*/
            oracleBleuScores.clear();
            oraclefeatureScore.clear();
            BestOracle = PP_ORACLE;
            oracleBleuScores.push_back(oraclebleu);
            oraclefeatureScore.push_back(path.GetScoreBreakdown());
        }

        /*
        // ------------------------trial--------------------------------//
        if (implementation == FPercepWMira) {
            if (oraclebleu > bestbleu) {
                pp_list::const_iterator it1;
                for (it1 = PP_ORACLE.begin(); it1 != PP_ORACLE.end(); it1++) {
                    std::map<std::string, int>::const_iterator itr1;
                    for (itr1 = (it1->second).begin(); itr1 != (it1->second).end(); itr1++) {
                        if (PP_BEST[it1->first][itr1->first] != 1 && Visited[it1->first][itr1->first] != 1) {
                            ShootUp(it1->first, itr1->first, abs(oracleScore - bestScore));
                            Visited[it1->first][itr1->first] = 1;
                        }
                    }
                }
                for (it1 = PP_BEST.begin(); it1 != PP_BEST.end(); it1++) {
                    std::map<std::string, int>::const_iterator itr1;
                    for (itr1 = (it1->second).begin(); itr1 != (it1->second).end(); itr1++) {
                        if (PP_ORACLE[it1->first][itr1->first] != 1 && Visited[it1->first][itr1->first] != 1) {
                            ShootDown(it1->first, itr1->first, abs(oracleScore - bestScore));
                            Visited[it1->first][itr1->first] = 1;
                        }
                    }
                }
            }
            if (oraclebleu < bestbleu) {
                pp_list::const_iterator it1;
                for (it1 = PP_ORACLE.begin(); it1 != PP_ORACLE.end(); it1++) {
                    std::map<std::string, int>::const_iterator itr1;
                    for (itr1 = (it1->second).begin(); itr1 != (it1->second).end(); itr1++) {
                        if (PP_BEST[it1->first][itr1->first] != 1 && Visited[it1->first][itr1->first] != 1) {
                            ShootDown(it1->first, itr1->first, abs(oracleScore - bestScore));
                            Visited[it1->first][itr1->first] = 1;
                        }
                    }
                }
                for (it1 = PP_BEST.begin(); it1 != PP_BEST.end(); it1++) {
                    std::map<std::string, int>::const_iterator itr1;
                    for (itr1 = (it1->second).begin(); itr1 != (it1->second).end(); itr1++) {
                        if (PP_ORACLE[it1->first][itr1->first] != 1 && Visited[it1->first][itr1->first] != 1) {
                            ShootUp(it1->first, itr1->first, abs(oracleScore - bestScore));
                            Visited[it1->first][itr1->first] = 1;
                        }
                    }
                }
            }
        }
        // ------------------------trial--------------------------------//
        */
    }
    cerr << "Read all the oracles in the list!\n";
    //	Update the weights
    //   if (implementation == FPercepWMira || implementation == Mira) {
    for (int i = 0; i < HypothesisList.size(); i++) // same loop used for feature values, modelscores
    {
        float bleuscore = BleuScore[i];
        loss.push_back(maxBleu - bleuscore);
    }
    modelScores.push_back(modelScore);
    featureValues.push_back(featureValue);
    BleuScores.push_back(BleuScore);
    losses.push_back(loss);
    oracleModelScores.push_back(maxScore);
    cerr << "Updating the Weights\n";
    size_t update_status = optimiser->updateWeights(weightUpdate, featureValues, losses,
                                                    BleuScores, modelScores, oraclefeatureScore, oracleBleuScores, oracleModelScores, wlr, 0, 0);
    StaticData::InstanceNonConst().SetAllWeights(weightUpdate);
    //cerr << "\nWeight : " << weightUpdate.GetScoreForProducer(sp) << "\n";
    //    }
    return;

}

bool OnlineLearner::SetPostEditedSentence(std::string s) {
    if (m_postedited.empty()) {
        m_postedited = s;
        return true;
    } else {
        cerr << "post edited already exists.. " << m_postedited << endl;
        return false;
    }
}

void OnlineLearner::RemoveJunk() {
    m_postedited.clear();
    PP_ORACLE.clear();
    PP_BEST.clear();
}

