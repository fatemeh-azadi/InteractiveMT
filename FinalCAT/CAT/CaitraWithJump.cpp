#include "CaitraWithJump.h"

using namespace std;


namespace Caitra{
CaitraWithJump::CaitraWithJump(const LanguageModelImplementation *lm)
{
    threshold = 0;
    max_time = 0;
    last_word_may_match_partially = true;
    match_last_partial_word_desparately = false;
    error_unit = 1;
    case_insensitive_matching = false;
    suffix_insensitive_min_match = 0;
    suffix_insensitive_max_suffix = 0;
    approximate_word_match_threshold = 1.0;
    match_last_word_window = 0;
    LM = lm;
}

CaitraWithJump::~CaitraWithJump(){

}

void  CaitraWithJump::createGraph(vector<SearchGraphNode> v) {

    map<int, int> recombination;
    for (int i = 0; i < v.size(); i++) {
        int id = v[i].hypo->GetId();

        int toState;
        int forward = v[i].forward;
        float fscore = v[i].fscore;

        if(id == 0){
            State newState = State(forward, fscore, fscore);
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

        vector<Word> o = tokenize( out, backwardScore + fscore );
        Transition newTransition( toState, transitionScore, o);
        int thisKey = hypId2stateId[ fromState ];
        states[ thisKey ].transitions.push_back( newTransition );

        if (recombined == -1) {

            State newState(forward, fscore, backwardScore+fscore );
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
        }

    }
    transitionsSize = v.size();

    TRACE_ERR("graph has " << states.size() << " states, pruned down from " << v.size() << "\n");


}

string CaitraWithJump::getPrefix(string pr){

    double start_time = get_wall_time();
    prefix_has_final_space = (pr[pr.length()-1] == ' ');
    prefix = tokenize(pr, 0);
    string last_token = surface[prefix[prefix.size()-1]];

    // allow partial matching of last token in prefix matching search
    if (last_word_may_match_partially && !prefix_has_final_space) {
        // also allow case-insensitive match
        string last_token_lowercase = lowercase(last_token);

        partially_matches_last_token.clear();
        // for all words in vocabulary
        for (map<string, Word>::iterator iter = lexicon.begin(); iter != lexicon.end(); iter++) {
            string word = lowercase(iter->first);
            // check if could be a partial match
            if (last_token_lowercase.length() < word.length() &&
                    last_token_lowercase == word.substr(0,last_token.length())) {
                partially_matches_last_token.insert( iter->second );
            }
        }

    }
    // allow case-insensitive matching
    if (case_insensitive_matching) {
        // for all words in prefix
        for(int p=0; p<prefix.size(); p++) {
            if (already_processed.count( prefix[p] )) {
                continue;
            }
            // for all words in vocabulary
            for (map<string, Word>::iterator iter = lexicon.begin(); iter != lexicon.end(); iter++) {
                // if they match case-insensitive, take note
                if (equal_case_insensitive( surface[ prefix[p] ], iter->first )) {
                    lowercase_word_match.insert( make_pair( prefix[p], iter->second ) );
                }
            }
        }
    }

    // consider mismatches of similarly spelled words as half an error
    if (approximate_word_match_threshold < 1.0) {
        // for all words in prefix
        for(int p=0; p<prefix.size(); p++) {
            if (already_processed.count( prefix[p] )) {
                continue;
            }
            int length_prefix_word = surface[ prefix[p] ].size();
            // for all words in vocabulary
            for (map<string, Word>::iterator iter = lexicon.begin(); iter != lexicon.end(); iter++) {
                int distance = letter_string_edit_distance( prefix[p], iter->second );
                int length_vocabulary_word = iter->first.size();
                int min_length = length_prefix_word < length_vocabulary_word ? length_prefix_word : length_vocabulary_word;
                if (distance <= min_length * approximate_word_match_threshold) {
                    approximate_word_match.insert( make_pair( prefix[p], iter->second ) );
                }
            }
        }
    }


    // consider mismatches in word endings (presumably morphological variants) as half an error
    if (suffix_insensitive_max_suffix > 0) {
        // for all words in prefix
        for(int p=0; p<prefix.size(); p++) {
            if (already_processed.count( prefix[p] )) {
                continue;
            }
            // for all words in vocabulary
            int length_prefix_word = surface[ prefix[p] ].size();
            for (map<string, Word>::iterator iter = lexicon.begin(); iter != lexicon.end(); iter++) {
                int length_vocabulary_word = iter->first.size();
                if (abs(length_vocabulary_word-length_prefix_word) <= suffix_insensitive_max_suffix &&
                        length_prefix_word >= suffix_insensitive_min_match &&
                        length_vocabulary_word >= suffix_insensitive_min_match) {
                    int specific_min_match = ( length_prefix_word > length_vocabulary_word ) ? length_prefix_word : length_vocabulary_word;
                    specific_min_match -= suffix_insensitive_max_suffix;
                    if (suffix_insensitive_min_match > specific_min_match) {
                        specific_min_match = suffix_insensitive_min_match;
                    }
                    if (iter->first.substr(0,specific_min_match) ==
                            surface[ prefix[p] ].substr(0,specific_min_match)) {
                        suffix_insensitive_word_match.insert( make_pair( prefix[p], iter->second ) );
                    }
                }
            }
        }
    }


    // record seen words for caching pre-processing across requests
    if (case_insensitive_matching ||
            approximate_word_match_threshold < 1.0 ||
            suffix_insensitive_max_suffix > 0) {
        for(int p=0; p<prefix.size(); p++) {
            if (!already_processed.count( prefix[p] )) {
                already_processed.insert( prefix[p] );
            }
        }
    }
    TRACE_ERR("preparation took " << (get_wall_time() - start_time) << " seconds\n");


    // call the main search loop
    int errorAllowed = prefix_matching_search( max_time, 0 );
    if (max_time>0 && errorAllowed == -1) {
        errorAllowed = prefix_matching_search( 0, 0.000001 );
    }

    if(errorAllowed == 20000)
        return "";
    // we found the best completion, now construct suffix for output
    Best &b = best[errorAllowed];
    vector< Word > matchedPrefix, predictedSuffix, jumpedPrefix;


    // add words from final prediction
    for(int i=b.output_matched-1;i>=0;i--) {
        matchedPrefix.push_back( b.transition->output[i] );
    }
    for(int i=b.output_matched;i<b.transition->output.size();i++) {
        predictedSuffix.push_back( b.transition->output[i] );
    }

    // add suffix words (best path forward)
    int suffixState = b.transition->to_state;
    while (states[suffixState].forward > 0) {
        Transition *transition = NULL;
        float best_score = -999;
        vector< Transition > &transitions = states[suffixState].transitions;
        for(int t=0; t<transitions.size(); t++) {
            if (transitions[t].to_state == states[suffixState].forward &&
                    transitions[t].score > best_score) {
                transition = &transitions[t];
                best_score = transition->score;
            }
        }
        for(int i=0;i<transition->output.size();i++) {
            predictedSuffix.push_back( transition->output[i] );
        }
        suffixState = states[suffixState].forward;
    }

    // add prefix words (following back transitions)
    int prefixState = b.from_state;
    int prefix_matched = b.back_matched;
    int jumpState = b.backJump;
    while (prefixState > 0) {
        backIter back = states[prefixState].back.begin();
        for(; back != states[prefixState].back.end(); back++ ) {
            if (back->prefix_matched == prefix_matched && back->jumpState == jumpState) {
                break;
            }
        }
        const vector< Word > &output = back->transition->output;
        if(back->jumpState != 1)
        for(int i=output.size()-1; i>=0; i--) {
            matchedPrefix.push_back( output[i] );
        }
        else
            for(int i=output.size()-1; i>=0; i--) {
                jumpedPrefix.push_back( output[i] );
            }

        back->transition->output.size();
        prefixState = back->back_state;
        prefix_matched = back->back_matched;
        jumpState = back->backJump;
    }

    string res = "";

    // handle final partial word (normal case)
    bool successful_partial_word_completion = false;
    if (!successful_partial_word_completion && last_word_may_match_partially && !prefix_has_final_space && matchedPrefix.size()>0) {
        Word last_matched_word = matchedPrefix[ 0 ];
        if (partially_matches_last_token.count( last_matched_word )) {
            res += surface[ last_matched_word ].substr(last_token.length());
            successful_partial_word_completion = true;
        }
    }

    // try a bit harder to match the last word
    if (!successful_partial_word_completion && prefix.size()>0 && match_last_word_window>0 &&
            (matchedPrefix.size() == 0 || matchedPrefix[ 0 ] != prefix[prefix.size()-1])) {
        // if we match it case-insensitive, that's okay
        bool is_okay = false;
        if (matchedPrefix.size()>0) {
            if (equal_case_insensitive(last_token, surface[ matchedPrefix[0] ])) {
                is_okay = true;
            }
        }
        // look for last word in window around current matched path position
        for(int i=0; !is_okay && i<=match_last_word_window; i++) {
            // is the word in the predicted suffix?
            if (predictedSuffix.size() > i &&
                    (equal_case_insensitive(last_token, surface[ predictedSuffix[i]]) ||
                     (last_word_may_match_partially && !prefix_has_final_space && partially_matches_last_token.count( predictedSuffix[i] )))) {
                // move predicted suffix words into matched prefix
                for(int j=0; j<=i; j++) {
                    matchedPrefix.insert( matchedPrefix.begin(), predictedSuffix[0] );
                    predictedSuffix.erase( predictedSuffix.begin() );
                }
                is_okay = true;
            }
            // is the word in the macthed prefix?
            else if (i>0 && matchedPrefix.size() > i &&
                     (equal_case_insensitive(last_token, surface[ matchedPrefix[i]]) ||
                      (last_word_may_match_partially && !prefix_has_final_space && partially_matches_last_token.count( matchedPrefix[i] )))) {
                // move matched prefix words into predicted suffix
                for(int j=0; j<i; j++) {
                    predictedSuffix.insert( predictedSuffix.begin(), matchedPrefix[0] );
                    matchedPrefix.erase( matchedPrefix.begin() );
                }
                is_okay = true;
            }
        }
    }


    // desparation word completion: matching word with best path score
    if (match_last_partial_word_desparately && !prefix_has_final_space && !successful_partial_word_completion && word_score[ prefix[prefix.size()-1] ] == 0) {
        string best;
        float best_score = -9e9;
        bool best_case_sensitive = false;
        // for all words in vocabulary

        for (map<string, Word>::iterator iter = lexicon.begin(); iter != lexicon.end(); iter++) {
            // word known in different casing, use it
            if (equal_case_insensitive(iter->first, last_token) &&
                    word_score[ iter->second ] != 0) {
                best = iter->first;
                best_score = 0;
                break;
            }
            if (iter->first.length() >= last_token.length() &&
                    word_score[ iter->second ] != 0 &&
                    word_score[ iter->second ] > best_score &&
                    equal_case_insensitive(iter->first.substr(0,last_token.length()), last_token)) {
                // prefer case-sensitive match
                if (iter->first.substr(0,last_token.length()) == last_token) {
                    best_case_sensitive = true;
                    best = iter->first;
                    best_score = word_score[ iter->second ];
                }
                if (!best_case_sensitive) {
                    best = iter->first;
                    best_score = word_score[ iter->second ];
                }
            }
        }
        if (best_score > -8e9) {
            cout << best.substr(last_token.length());
            successful_partial_word_completion = true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    string suffix = "", jumpedPart = "";

    // output results
    for( int i=0; i<predictedSuffix.size(); i++) {
        if (i>0 || !prefix_has_final_space) { suffix += " "; }
        suffix += surface[ predictedSuffix[i] ];
    }

    for(int i = (int)jumpedPrefix.size() - 1; i >= 0; i--){
        if (i<((int)jumpedPrefix.size() - 1) || !prefix_has_final_space) { jumpedPart += " "; }
        jumpedPart += surface[ jumpedPrefix[i] ];
    }

    string res1 = res + suffix;
    string res2 = res + jumpedPart;
    if(prefix_has_final_space){
        if(jumpedPart.length() > 0 && res1.length() > 0)
        res1 += " ";
        if(suffix.length() > 0 && res2.length() > 0)
        res2 += " ";
    }

    if (predictedSuffix.size() == 0 && jumpedPrefix.size() == 0) {
        res += " ";
    }
    else{
    res1 += jumpedPart;
    res2 += suffix;


    double lmScore1 = getLMScore(pr + res1);
    double lmScore2 = getLMScore(pr + res2);

    cout << "Res1 *** " << pr + res1 <<  "  ****  " << lmScore1 << endl;
    cout << "Res2 *** " << pr + res2 <<  "  ****  " << lmScore2 << endl;

    if(lmScore1 > lmScore2)
        res = res1;
    else
        res= res2;
    }
    // clear out search
    for( int state = 0; state < states.size(); state++ ) {
        states[state].back.clear();
    }
    for (int errorAllowed = 0; errorAllowed < 20001; errorAllowed++ ) {
        best[errorAllowed].from_state = -1;
    }

    return res;

}



// helper function to get BackTranstition that reached
// * a particular state
// * with a certain number of prefix words matched

// main search loop
int CaitraWithJump::prefix_matching_search( float max_time, float threshold ) {
    double start_time = get_wall_time();

    // intialize search - initial back transition (state in prefix matching search)
    BackTransition initialBack( 0.0, 0, 0, -1, 0, NULL);
    // ... associated with initial hypothesis
    states[0].back.push_back( initialBack );
    // start search with maximum error 0, then increase maximum error one by one
    int errorAllowed = 0;

    while( errorAllowed <= prefix.size() * error_unit ){
        // printf("error level %d\n",errorAllowed);
        // process decoder search graph, it is ordered, so we can just sequentially loop through states
        int valid_states = 0;
        int back_count = 0;
        int transition_count = 0;
        int match_count = 0;
        for( int state = 0; state < states.size(); state++ ) {

            // ignore state if it is too bad
            if (threshold > 0 && states[state].best_score < states[0].best_score+threshold) {
                continue;
            }
            valid_states++;
            // abort search if maximum time exceeded
            if (state % 100 == 0 && max_time > 0 && (get_wall_time()-start_time) > max_time) {
                return -1;
            }

            // if it has back transitions, it is reachable, so we have to process each
            for ( backIter back = states[state].back.begin(); back != states[state].back.end(); back++ ) {
                // only need to process back transitions with current error level
                // the ones with lower error have been processed in previous iteration

                /*************************/
                if (back->error == errorAllowed) {
                    back_count++;
                    // loop through transitions out of this state
                    for ( transIter transition = states[state].transitions.begin(); transition != states[state].transitions.end(); transition++ ) {

                        if (threshold > 0 && states[transition->to_state].best_score < states[0].best_score+threshold) {
                            continue;
                        }
                        transition_count++;

                        if(back->jumpState == 0 || back->jumpState == 1){ // jump this transition
                            process_jump(state, *back, *transition);

                        }
                        // try to match this transition's phrase
                        // starting at end word prefix position of previous back transition
                        vector< Match > matches = string_edit_distance( back->prefix_matched, transition->output );
                        // process all matches
                        for ( matchIter match = matches.begin(); match != matches.end(); match++ ) {
                            match_count++;
                            // check if match leads to valid new back transition
                            process_match( state, *back, *match, *transition );

                        }
                    }
                }
            }
        }
        TRACE_ERR("explored " << valid_states << " valid states, " << back_count << " backs, " <<  transition_count << " transitions, " << match_count << " matches at error level " << errorAllowed << endl);
          // found a completion -> we are done
        if (best[errorAllowed].from_state != -1) {
            cerr << "search took " << (get_wall_time()-start_time) << " seconds.\n";
            return errorAllowed;
        }
        errorAllowed++;
    }

    // cout << discover_Error << endl;
    return 20000;
}

int CaitraWithJump::letter_string_edit_distance( Word wordId1, Word wordId2 ) {
    wstring word1;
    decode_utf8(surface[ wordId1 ], word1);
    wstring word2;
    decode_utf8(surface[ wordId2 ], word2);
    int **cost = (int**) calloc( sizeof( int* ), word1.size() );
    for( int i=0; i<word1.size(); i++ ) {
        cost[i] = (int*) calloc( sizeof( int ), word2.size() );
        for( int j=0; j<word2.size(); j++ ) {
            if (i==0 && j==0) {
                cost[i][j] = 0;
            }
            else {
                cost[i][j] = 999;
                if (j>0 && cost[i][j-1]+1 < cost[i][j]) {
                    cost[i][j] = cost[i][j-1]+1;
                }
                if (i>0 && cost[i-1][j]+1 < cost[i][j]) {
                    cost[i][j] = cost[i-1][j]+1;
                }
                if (i>0 && j>0) {
                    if (word1[i] != word2[j]) {
                        if (cost[i-1][j-1]+1 < cost[i][j]) {
                            cost[i][j] = cost[i-1][j-1]+1;
                        }
                    }
                    else {
                        if (cost[i-1][j-1] < cost[i][j]) {
                            cost[i][j] = cost[i-1][j-1];
                        }
                    }
                }

            }
        }

    }
    int distance = cost[word1.size()-1][word2.size()-1];
    for( int i=0; i<word1.size(); i++ ) {
        free( cost[i] );
    }
    free( cost );
    return distance;
}


// match a phrase (output of a transition) against the prefix
// return a vector of matches with best error (multiple due to different number of prefix words matched)
inline vector< Match > CaitraWithJump::string_edit_distance( int alreadyMatched, const vector< Word > &transition ) {
    vector< Match > matches;
    int toMatch = prefix.size() - alreadyMatched;
    int **cost = (int**) calloc( sizeof( int* ), toMatch+1 );

    //for( int j=1; j<=transition.size(); j++ )
    //printf("\t%s",surface[transition[ j-1 ]].c_str());
    for( int i=0; i<=toMatch; i++ ) {
        //if (i==0) printf("\n\t\t");
        //else printf("\n\t\t%s",surface[prefix[alreadyMatched+i-1]].c_str());
        cost[i] = (int*) calloc( sizeof(int), transition.size()+1 );
        for( int j=0; j<=transition.size(); j++ ) {
            if (i==0 && j==0) { // origin
                cost[i][j] = 0;
                //printf("\t0");
            }
            else {
                int lowestError =error_unit * (prefix.size()*2+2);

                if (i>0) { // deletion
                    lowestError = cost[i-1][j] + error_unit;
                }
                if (j>0) { // insertion
                    int thisError = cost[i][j-1] + error_unit;
                    if (thisError < lowestError) {
                        lowestError = thisError;
                    }
                }

                if (i>0 && j>0) { // match or subsitution
                    int thisError = cost[i-1][j-1];
                    if (prefix[ alreadyMatched + i-1 ] != transition[ j-1 ]) {
                        // mismatch -> substitution
                        // ... unless partially matching last prefix token
                        if (! (last_word_may_match_partially &&
                               ((alreadyMatched + i-1) == (prefix.size()-1)) &&
                               partially_matches_last_token.count( transition[ j-1 ] )) &&
                                // ... and unless allowing case-insensitive matching
                                ! (case_insensitive_matching &&
                                   lowercase_word_match.count( make_pair( prefix[ alreadyMatched + i-1 ], transition[ j-1 ] )))) {
                            // if allowing approximate matching, count as half an error

                            if ((approximate_word_match_threshold > 0.0 &&
                                 approximate_word_match.count( make_pair( prefix[ alreadyMatched + i-1 ], transition[ j-1 ] ))) ||
                                (suffix_insensitive_max_suffix > 0 &&
                                 suffix_insensitive_word_match.count( make_pair( prefix[ alreadyMatched + i-1 ], transition[ j-1 ] )))) {
                                thisError += 1;
                            }
                            else {
                                // really is a mismatch
                                thisError += error_unit;
                            }

                        }
                    }
                    if (thisError < lowestError) {
                        lowestError = thisError;
                    }
                }


                cost[i][j] = lowestError;
                //printf("\t%d",lowestError);
            }
        }
    }

    // matches that consumed the prefix
        for(int j=1; j<=transition.size(); j++ ) {
            if((last_word_may_match_partially && !prefix_has_final_space &&
                    partially_matches_last_token.count( transition[ j-1 ] )) || !last_word_may_match_partially || prefix_has_final_space){
            Match newMatch( cost[toMatch][j], prefix.size(), j );
            matches.push_back( newMatch );
            }
        }

        // matches that consumed the transition
        for(int i=1; i<toMatch; i++ ) {
            Match newMatch( cost[i][transition.size()], alreadyMatched + i, transition.size() );
            matches.push_back( newMatch );
        }

    for( int i=0; i<=toMatch; i++ ) {
        free( cost[i] );
    }
    free( cost );

    return matches;
}

inline void CaitraWithJump::process_jump(int state, const BackTransition &back, const Transition &transition){

    int transition_to_state = transition.to_state;
    float score = back.score + transition.score;
    int error = back.error + (back.jumpState == 0 ? 1 : 0);
    //int error = back.error + 1;
    for( backIter oldBack = states[transition_to_state].back.begin(); oldBack != states[transition_to_state].back.end(); oldBack++ ) {
        if (oldBack->prefix_matched == back.prefix_matched && oldBack->jumpState == 1) { // already a back path with same prefix match?
            // if better, overwrite
              if (oldBack->error > error ||
                    (oldBack->error == error && (oldBack->score < score ||
                                                 (oldBack->score == score && oldBack->back_matched < back.prefix_matched)))) {


                oldBack->error = error;
                oldBack->score = score;
                oldBack->back_state = state;
                oldBack->back_matched = back.prefix_matched;
                oldBack->prefix_matched = back.prefix_matched;
                oldBack->transition = &transition;
                oldBack->backJump = back.jumpState;
                oldBack->numOfJumps = back.numOfJumps + 1;
                // cerr << "\t\t\toverwriting\n";
            }
            // if worse, ignore
            // done in any case
            return ;
        }
    }


    BackTransition newBack( score, error, back.prefix_matched, state, back.prefix_matched, &transition , 1, back.jumpState);
    newBack.numOfJumps = back.numOfJumps + 1;
    states[transition_to_state].back.push_back( newBack );

}

// given a back transition (state in the prefix matching search)
// and match (information how it can be extended with matching a transition)
// create a new back transition (new state in the prefix matching search)
inline void CaitraWithJump::process_match( int state, const BackTransition &back, const Match &match, const Transition &transition ) {
    int transition_to_state = transition.to_state;
    float score = back.score + transition.score;
    int error = back.error + match.error;
    int jumpState = (back.jumpState == 0 ? 0 : 2);

    // common case: prefix is not yet fully matched
    if (match.prefixMatched < prefix.size() ) {
        // check how this new back transition compares against existing ones
        for( backIter oldBack = states[transition_to_state].back.begin(); oldBack != states[transition_to_state].back.end(); oldBack++ ) {
            if (oldBack->prefix_matched == match.prefixMatched && oldBack->jumpState == jumpState) { // already a back path with same prefix match?
                // if better, overwrite
                  if (oldBack->error > error ||
                        (oldBack->error == error && (oldBack->score < score ||
                                                     (oldBack->score == score && oldBack->back_matched < match.prefixMatched)))) {


               /* if ((oldBack->score - errorWeigth * oldBack->error < score - errorWeigth * error) ||
                        ((oldBack->score - errorWeigth * oldBack->error) == (score - errorWeigth * error) && oldBack->back_matched < match.prefixMatched)){
*/

                    oldBack->error = error;
                    oldBack->score = score;
                    oldBack->back_state = state;
                    oldBack->back_matched = back.prefix_matched;
                    oldBack->prefix_matched = match.prefixMatched;
                    oldBack->transition = &transition;
                    oldBack->backJump = back.jumpState;
                    // cerr << "\t\t\toverwriting\n";
                }
                // if worse, ignore
                // done in any case
                return ;
            }
        }

        // not recombinable with existing back translation -> just add it
        BackTransition newBack( score, error, match.prefixMatched, state, back.prefix_matched, &transition , jumpState, back.jumpState);
        states[transition_to_state].back.push_back( newBack );

        //cerr << "\t\t\tadding\n";
    }
    // special case: all of the prefix is consumed
    else {
        // add score to complete path
        score += states[transition_to_state].forward_score;

        // first completion ... or ... better than currently best completion?
        if ( best[error].from_state == -1 ||
             score > best[error].score ||
             (score == best[error].score && match.prefixMatched > best[error].prefix_matched) ) {
            best[error].score = score;
            best[error].from_state = state;
            best[error].transition = &transition;
            best[error].output_matched = match.transitionMatched;
            best[error].back_matched = back.prefix_matched;
            best[error].prefix_matched = match.prefixMatched;
            best[error].backJump = back.jumpState;
            //cerr << "\t\t\tnew best\n";

        }

    }
}

inline double CaitraWithJump::getLMScore(string s){

    Phrase phrase(Output);
    phrase.CreateFromString(Output, StaticData::Instance().GetOutputFactorOrder(), s ,StaticData::Instance().GetFactorDelimiter(), NULL );
    float FullScore;
    float NGramScore;
    size_t oovCount;


    LM->CalcScore(phrase, FullScore, NGramScore, oovCount);

    return FullScore;
}

string CaitraWithJump::lowercase(string mixedcase) {
    transform(mixedcase.begin(), mixedcase.end(), mixedcase.begin(), ::tolower);
    return mixedcase;
}

bool CaitraWithJump::equal_case_insensitive(string word1, string word2) {
    transform(word1.begin(), word1.end(), word1.begin(), ::tolower);
    transform(word2.begin(), word2.end(), word2.begin(), ::tolower);
    return word1 == word2;
}

}

