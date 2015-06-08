#ifndef CAITRASEARCHGRAPH_H
#define CAITRASEARCHGRAPH_H


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
#include "MainCaitra.h"

namespace Caitra{


class CaitraSearchGraph: public SearchGraph
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

    double best_Score;
    double errorWeigth;
    const static double p_Error = 0.005;
    int transitionsSize;


public:
    CaitraSearchGraph();
    int getGraphSize() { return states.size(); }
    int getTransitionsSize(){ return transitionsSize; }

    CaitraSearchGraph(double erw);
    ~CaitraSearchGraph(){}
    void createGraph(vector<SearchGraphNode> v);
    string getPrefix(string prefix);
    int prefix_matching_search( float max_time, float threshold );
    int letter_string_edit_distance( Word wordId1, Word wordId2 );
    inline vector< Match > string_edit_distance( int alreadyMatched, const vector< Word > &transition ) ;
    inline int process_match( int state, const BackTransition &back, const Match &match, const Transition &transition );
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

    static double getBinomialErrorCost(int error, int p_len){

        double res = 0;
        for(int i = 1; i <= error; i++)
            res += log(p_Error);
        for(int i = 1; i <= p_len - error; i++){
            res += (log(i + error) - log((double)i) + log(1. - p_Error));
        }

        return res;

    }

    inline static double getLogErrorCost(int error){

        double res = error;
        return res;

    }

};

}

#endif // CAITRASEARCHGRAPH_H
