#ifndef ECMODELSEARCHGRAPH_H
#define ECMODELSEARCHGRAPH_H

#include <vector>
#include <map>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "Distance.h"
#include "utf8.h"
#include "editdistance.h"
#include "SearchGraph.h"

namespace CATServer{


typedef unsigned int Word; // words are stored as integers -- see add_to_lexicon

const static double eps = 1e-9;

struct Transition {
public:
    int to_state; // next state
    float score;   // transition score
    vector< Word > output;  // output phrase in this transition


    wstring outputString;
    Transition(int t, float s, vector<Word> o , string out) {
        to_state = t;
        score = s;
        output = o;
        decode_utf8(out.c_str(), outputString);

    }
};


class BackTransition {
public:
    float score;
    int error;
    int prefix_matched;
    int back_state;
    int back_matched;
    const Transition *transition;
    BackTransition( float s, int e, int pm, int b, int bm, const Transition *t ) {
        score = s;
        error = e;
        prefix_matched = pm;
        back_state = b;
        back_matched = bm;
        transition = t;
    }
};


class State {
public:
    int forward; // next state in the best path
    int back;
    float forward_score; // best forward score
    float best_score;   // best total score (backward + forward)
    const Transition *best_transition;  // best transition to the next state

    const Transition *best_back_transition; // best transition to this state
    vector< Transition > transitions; // all of the transitions from this state to next ones
//    vector< BackTransition > back;

    vector<int> editCost_prefixConsumed;
    vector<int> editCost_stateConsumed;

    int editCostSize;

    State(int b, int f,  float fs, float bs ) {
        back = b;
        forward = f;
        forward_score = fs;
        best_score = bs;
        editCostSize = 0;
        best_back_transition = NULL;
        best_transition = NULL;
    }
};


typedef vector< BackTransition >::iterator backIter;
typedef vector< Transition >::iterator transIter;


class ECModelSearchGraph : public SearchGraph{

//    map<int, SearchGraphNode> nodes;  // map each node to it's id
//    map<int, map<int, double> > graph;  //  the edges of the graph

    map< string, Word > lexicon;
    vector< float > word_score;
    vector< string > surface;
    vector< State > states;
    vector<int> stateId2hypId;
    map<int,int> hypId2stateId;

    const static double errorWeigth = 1.5;

public:
    ECModelSearchGraph() {}
~ECModelSearchGraph() {}
    int getGraphSize() { return states.size(); }
    int getTransitionsSize(){ return 0; }
    void createGraph(vector<SearchGraphNode> v);

    void print();
    string getPrefix(string prefix);
    void computeEditDistances(wstring prefix);
    void processState(int stateId, const vector<int> &d, const wstring &s, const wstring &prefix);

    string getBestSuffix(int stateId, int alignment);
    int selectBestState(wstring p, int& bestAlignment);
    int getBestAlignment(int stateId, int p_len);
    string getBestPrefix(int stateId, int alignment);

    int getBestAlignment(const vector<int> &v);

    Word add_to_lexicon( string wordstring, float score) {
        map<string, Word>::iterator lookup = lexicon.find( wordstring );
        if (lookup != lexicon.end()) {
            if (score > 0 && word_score[ lookup->second ] > score) {
                word_score[ lookup->second ] = score;
            }
            return lookup->second;
        }
//         printf("[%d:%d:%s]",surface.size(),lexicon.size(),wordstring.c_str());
        lexicon[ wordstring ] = surface.size();
        word_score.push_back( score );
        surface.push_back( wordstring );
        return lexicon.size()-1;
    }

    inline vector<Word> tokenize(const string& str, float score)
    {
        const string& delimiters = " \t\n\r";
        vector<Word> tokens;
        // Skip delimiters at beginning.
        string::size_type lastPos = str.find_first_not_of(delimiters, 0);
        // Find first "non-delimiter".
        string::size_type pos     = str.find_first_of(delimiters, lastPos);

        while (string::npos != pos || string::npos != lastPos)
        {
            // Found a token, add it to the vector.
            tokens.push_back(add_to_lexicon(str.substr(lastPos, pos - lastPos),score));
            // Skip delimiters.  Note the "not_of"
            lastPos = str.find_first_not_of(delimiters, pos);
            // Find next "non-delimiter"
            pos = str.find_first_of(delimiters, lastPos);
        }
        return tokens;
    }


};
}

#endif // ECMODELSEARCHGRAPH_H
