#ifndef MAINCAITRA_H
#define MAINCAITRA_H
#include <vector>
#include <map>
#include <algorithm>
#include <sys/time.h>
#include <boost/algorithm/string.hpp>
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "Distance.h"
#include "utf8.h"
#include "editdistance.h"
#include "SearchGraph.h"



namespace Caitra{

typedef unsigned int Word; // words are stored as integers -- see add_to_lexicon


struct Transition {
public:
    int to_state; // next state
    float score;   // transition score
    vector< Word > output;  // output phrase in this transition


    Transition(int t, float s, vector< Word > o ) {
        to_state = t;
        score = s;
        output = o;

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

    int jumpState; //for CaitraWithJump : 0 -> no jump till now , 1 -> jump started, 2 -> jump finished
    int backJump;
    int numOfJumps;

    BackTransition( float s, int e, int pm, int b, int bm, const Transition *t ) {
        score = s;
        error = e;
        prefix_matched = pm;
        back_state = b;
        back_matched = bm;
        transition = t;
        jumpState = 0;
        backJump = 0;
        numOfJumps = 0;
    }


    BackTransition( float s, int e, int pm, int b, int bm, const Transition *t , int jump, int bjump ) {
        score = s;
        error = e;
        prefix_matched = pm;
        back_state = b;
        back_matched = bm;
        transition = t;
        jumpState = jump;
        backJump = bjump;
        numOfJumps = 0;

    }
};


class State {
public:
    int forward; // next state in the best path
    float forward_score; // best forward score
    float best_score;   // best total score (backward + forward)
    const Transition *best_transition;  // best transition to the next state
    vector< Transition > transitions; // all of the transitions from this state to next ones
    vector< BackTransition > back;

    State(int f,  float fs, float bs ) {
        forward = f;
        forward_score = fs;
        best_score = bs;
    }
};

// matching information in prefix matching search
class Match {
public:
    int error; // number of edits
    int prefixMatched; // number of user prefix words matched
    int transitionMatched; // number of words in last transition matched
    Match( int e, int p, int t ) {
        error = e;
        prefixMatched = p;
        transitionMatched = t;
    }
};


// stores the currently best completion
class Best {
public:
    int from_state;
    const Transition *transition;
    BackTransition *back;
    int output_matched;  // number of words in last transition matched
    int back_matched;   // number of user prefix words matched in backTransition
    int prefix_matched; // number of user prefix words matched
    float score;
    int backJump;
    Best() {
        from_state = -1;
    }
};

typedef vector< BackTransition >::iterator backIter;
typedef vector< Transition >::iterator transIter;
typedef vector< Match >::iterator matchIter;

class MainCaitra: public SearchGraph
{
    float threshold;
    float max_time;

    map< string, Word > lexicon;
    vector< float > word_score;
    vector< string > surface;
    vector< State > states;
    vector<int> stateId2hypId;
    map<int,int> hypId2stateId;

    vector< Word > prefix;
    Best best[20001];
    bool last_word_may_match_partially, case_insensitive_matching, match_last_partial_word_desparately;
    int error_unit;
    float approximate_word_match_threshold;
    int suffix_insensitive_min_match, suffix_insensitive_max_suffix;
    int match_last_word_window;
    set< Word > partially_matches_last_token;
    set< pair< Word, Word > > approximate_word_match, lowercase_word_match, suffix_insensitive_word_match;
    set< Word > already_processed;
    bool prefix_has_final_space;
   int transitionsSize;
    const static double errorWeigth = 0.84;


public:
    MainCaitra();
    ~MainCaitra();
    int getGraphSize() { return states.size(); }
    int getTransitionsSize(){ return transitionsSize; }
    void createGraph(vector<SearchGraphNode> v);
    string getPrefix(string prefix);
    int prefix_matching_search( float max_time, float threshold );
    int letter_string_edit_distance( Word wordId1, Word wordId2 );
    inline vector< Match > string_edit_distance( int alreadyMatched, const vector< Word > &transition ) ;
    inline void process_match( int state, const BackTransition &back, const Match &match, const Transition &transition );
    string lowercase(string mixedcase);
    bool equal_case_insensitive(string word1, string word2);


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

    static double get_wall_time(){
        struct timeval time;
        if (gettimeofday(&time,NULL)){
            //  Handle error
            return 0;
        }
        return (double)time.tv_sec + (double)time.tv_usec * .000001;
    }

};

}

#endif // MAINCAITRA_H
