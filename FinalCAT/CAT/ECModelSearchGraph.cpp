/*
 * ECModelSearchGraph.cpp
 *
 *  Created on: Jun 15, 2014
 *      Author: fatemeh
 */

#include <vector>
#include <map>
#include <algorithm>
#include <queue>
#include <boost/algorithm/string.hpp>
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "Distance.h"
#include "utf8.h"

#include "ECModelSearchGraph.h"

using namespace std;

namespace CATServer{



void  ECModelSearchGraph::createGraph(vector<SearchGraphNode> v) {

    map<int, int> recombination;
    for (int i = 0; i < v.size(); i++) {
        int id = v[i].hypo->GetId();

        int toState;
        int forward = v[i].forward;
        float fscore = v[i].fscore;

        if(id == 0){
            State newState = State(-1, forward, fscore, fscore);
            states.push_back(newState);
            stateId2hypId.push_back(id);
            continue;
        }

        const Hypothesis *prevHypo = v[i].hypo->GetPrevHypo();
        int fromState = prevHypo->GetId();
        float backwardScore = v[i].hypo->GetScore();
        float transitionScore = (v[i].hypo->GetScore() - prevHypo->GetScore());

        int recombined = -1;
        if (v[i].recombinationHypo != NULL)
            recombined = v[i].recombinationHypo->GetId();

        string out = v[i].hypo->GetCurrTargetPhrase().GetStringRep(StaticData::Instance().GetOutputFactorOrder());


        if (recombined >= 0) {
            recombination[ id ] = recombined;
            toState = recombined;
        }
        else {
            toState = id;
        }
        float pathScore = backwardScore + fscore;
        vector<Word> o = tokenize( out, pathScore);
        Transition newTransition( toState, transitionScore, o, out);
        int thisKey = hypId2stateId[ fromState ];
        states[ thisKey ].transitions.push_back( newTransition );


        if (recombined == -1) {

            State newState( thisKey, forward, fscore, backwardScore+fscore );
            states.push_back( newState );
            hypId2stateId[ id ] = stateId2hypId.size();
            stateId2hypId.push_back( id );
        }

    }

    hypId2stateId[-1]=-1;

    for(int state=0; state<states.size(); state++) {
        int forward = states[state].forward;
        if (recombination.count(forward)) {
            forward = recombination[ forward ];
        }
        states[state].forward = hypId2stateId[ forward ];
        for ( transIter transition = states[state].transitions.begin(); transition != states[state].transitions.end(); transition++ ) {
            transition->to_state = hypId2stateId[ transition->to_state ];
            int nxt = transition->to_state;
            if(states[nxt].back == state)
                states[nxt].best_back_transition = &(*transition);
        }

    }

    TRACE_ERR("graph has " << states.size() << " states, pruned down from " << v.size() << "\n");


}

void ECModelSearchGraph::print(){

    cout << states.size() << endl;
    for(int i = 0 ; i < states.size(); i++){
        cout << states[i].forward << " " << states[i].forward_score << " " << states[i].best_score << endl;

    }
}


string ECModelSearchGraph::getPrefix(string prefix){


    wstring p;
    decode_utf8(prefix, p);

//    cout << "computing edit distances..." << endl;
    // compute the edit distances of the best path for each state
    computeEditDistances(p);

//    cout << "editDistance Computed." << endl;

    int bestAlignment;
    int bestState = selectBestState(p, bestAlignment);


//    cout << "best state selected" << endl;
    string suffix = getBestSuffix(bestState, bestAlignment);

    return suffix;



}

//string ECModelSearchGraph::searchPrefix_EC_Model(string prefix){
//    wstring p;
//    decode_utf8(prefix, p);
//    double best_score = states[0].best_score + errorWeigth * getBinomialErrorCost(p.length(), p.length());
//    int best = 0, best_alignment = 0;
//    wstring bestString = L"";
//    for(int i = 0; i < states.size(); i++){
//        string s = getBestPrefix(i, 0) + " " + getBestSuffix(i, 0);
//        wstring x;
//        decode_utf8(s, x);

//        vector<int> dis(p.length() + 1, 0);
//        for(int i = 0; i < dis.size(); i++)
//            dis[i] = i;
//        vector<int> prefixConsumed, stateConsumed;
////        cout << x.length() << endl;
//        getEditDistance_CharLevel(dis, x, p, prefixConsumed, stateConsumed);

////        cout << prefixConsumed.size() << endl;
//        int a = getBestAlignment(prefixConsumed);

//        int error = prefixConsumed[a];
//        double score = states[i].best_score + errorWeigth * getBinomialErrorCost(error, p.length());

//        if(best_score < score){
//            best_score = score;
//            best = i;
//            best_alignment = a;
//            bestString = x;
//        }

////        cout << "state " << i << " : best-alignment=" << a << " error=" << error << " translation-score="
////             << states[i].best_score << "errorCost=" << getBinomialErrorCost(error, p.length()) << " total-score=" << score << " best-translation='" << s << "'" << endl;


//    }

//    string res;
//    bestString = bestString.substr(best_alignment);
//    encode_utf8(bestString, res);
////    cout << best << " " << best_alignment << endl;
//    return res;

//}

// get the suffix in the best path from states[stateId]
string ECModelSearchGraph::getBestSuffix(int stateId, int alignment){

//    cout << "best suffix" << endl;
    wstring res = L"";
    if(stateId != 0){
        res = states[stateId].best_back_transition->outputString;
//    cout << res.length() << endl;
    res = res.substr(alignment, res.length() - alignment);
    }
//    cout << "ddd" << endl;
    stateId = states[stateId].forward;
    while(stateId != -1){

//        cout << stateId << endl;
        res += (L" " + states[stateId].best_back_transition->outputString);
        stateId = states[stateId].forward;
    }
//    cout << stateId << endl;

    string ret;
    encode_utf8(res, ret);

//    cout << ret << endl;
    return ret;
}

// get the suffix in the best path from states[stateId]
string ECModelSearchGraph::getBestPrefix(int stateId, int alignment){

//    cout << "best prefix" << endl;
    wstring res = L"";
    if(stateId != 0){
        res = states[stateId].best_back_transition->outputString;
        stateId = states[stateId].back;
    }
    res = res.substr(0, alignment);


    while(stateId != 0){

        res = (states[stateId].best_back_transition->outputString + L" " + res);
        stateId = states[stateId].back;
    }

    string ret;
    encode_utf8(res, ret);

    return ret;
}

int ECModelSearchGraph::selectBestState(wstring p, int& bestAlignment){

    int best = 0;
    bestAlignment = getBestAlignment(0, p.length());
    int error = states[best].editCost_prefixConsumed[bestAlignment];
    double bestScore = states[best].best_score + errorWeigth * getBinomialErrorCost(error, p.length());

    for(int i = 1; i < states.size(); i++){

        int alignment = getBestAlignment(i, p.length());
        error = states[i].editCost_prefixConsumed[alignment];
        double score = states[i].best_score + errorWeigth * getBinomialErrorCost(error, p.length());
        string suffix= getBestSuffix(i, alignment);
        string preffix = getBestPrefix(i, alignment);
        cout << "state " << i << " : best-alignment=" << alignment << " error=" << error << " translation-score="
             << states[i].best_score << " total-score=" << score << " best-translation='" << preffix << "+" << suffix << "'" << endl;
        if(score > bestScore){
            bestScore = score;
            best = i;
            bestAlignment = alignment;
        }
    }

    error = states[best].editCost_prefixConsumed[bestAlignment];
    cout << best << " " << bestScore << " " << states[best].best_score<< " " << getBinomialErrorCost(error, p.length()) << endl;
    return best;

}

// returns the alignment with best edit cost for states[stateId]
int ECModelSearchGraph::getBestAlignment(int stateId, int p_len){

    int sz = states[stateId].editCostSize;
    double bestScore = states[stateId].editCost_prefixConsumed[0];
    int bestAlignment = 0;
    for(int i = 1; i <= sz; i++){
        double score = states[stateId].editCost_prefixConsumed[i];
        if(bestScore > score){
            bestAlignment = i;
            bestScore = score;
        }

    }

    return bestAlignment;
}

// returns the alignment with best edit cost for states[stateId]
int ECModelSearchGraph::getBestAlignment(const vector<int> &v){

    int sz = v.size();
    double bestScore = v[0];
    int bestAlignment = 0;
    for(int i = 1; i < sz; i++){
        double score = v[i];
        if(bestScore > score){
            bestAlignment = i;
            bestScore = score;
        }

    }

    return bestAlignment;
}

/*
  computing the edit distance for all of the states in search graph
  */
void  ECModelSearchGraph::computeEditDistances(wstring prefix){

    queue<int> statesQ;
    statesQ.push(0);
    vector<int> mark (states.size(), 0);
    mark[0] = 1;
    vector<int> d;
    for(int i = 0; i < prefix.length() + 1; i++){

        d.push_back(i);
    }

    wstring s = L"";

    processState(0, d, s, prefix);

    while(!statesQ.empty()){

        int stateId = statesQ.front();

  //      cout << stateId << endl;
        statesQ.pop();
        for(int i = 0; i < states[stateId].transitions.size(); i++){
            int nxt = states[stateId].transitions[i].to_state;

//            cout << "nxt ... : "<< nxt << " ";
            if(mark[nxt] == 0 && (states[nxt].back == stateId)){
                mark[nxt] = 1;


                s = states[stateId].transitions[i].outputString;
                states[nxt].best_back_transition = &states[stateId].transitions[i];
                if(nxt == states[stateId].forward)
                    states[stateId].best_transition = &states[stateId].transitions[i];
                processState(nxt, states[stateId].editCost_stateConsumed, s, prefix);

                statesQ.push(nxt);
            }
        }

    }

    for(int i = 0; i < states.size(); i++)
        if(!mark[i])
            cout << "state " << i << " empty" << endl;


}

/*
    computing the minimum edit distance for some state
    with initial distances for each position of prefix in d[]
*/
void  ECModelSearchGraph::processState(int stateId, const vector<int> &d, const wstring &s, const wstring &prefix){


    vector<int> d2;
    for(int i = 0; i < d.size(); i++)
        d2.push_back(d[i]);
    string pr = getBestPrefix(stateId, 0);
    wstring pr2 = L"";
    decode_utf8(pr, pr2);
    if(pr2.length()){
        d2[0] = d[0] + 1;
        for(int i = 1; i < d.size(); i++){
            if(prefix[i - 1] == L' ')
                d2[i] = d[i - 1];
            else
                d2[i] = d[i - 1] + 1;
            d2[i] = min(d2[i], min(d[i] + 1, d2[ i -1] + 1));
        }
    }
    states[stateId].editCost_stateConsumed.clear();
    states[stateId].editCost_prefixConsumed.clear();
    getEditDistance_CharLevel(d2, s, prefix, states[stateId].editCost_prefixConsumed, states[stateId].editCost_stateConsumed);
    states[stateId].editCostSize = s.length();
}

}
