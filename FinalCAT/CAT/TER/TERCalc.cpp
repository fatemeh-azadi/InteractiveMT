#include "TERCalc.h"
using namespace std;

namespace CATServer {
TERCalc::TERCalc()
{

    dp_len = 100;
    for(int i = 0; i < dp_len; i++){
        vector<int> v(dp_len, 0);
        dp.push_back(v);
        path.push_back(v);
    }

}

double TERCalc::getTER(const std::vector<std::wstring> &hyp, const std::vector<std::wstring> &ref){
     map<Sequence, vector<int> > matchedRefSubstrs = buildWordMatches(hyp, ref);
     Sequence currentHyp = hyp;
     int numOfShifts = 0, numOfEdits = minEditDistance(currentHyp, ref);
     vector<int> bestPath = getPath(currentHyp, ref);

     while (true){

         vector<int> nextPath;
         Sequence bestShift;
         int edits = calcBestShift(currentHyp, ref, matchedRefSubstrs, bestPath, numOfEdits, bestShift, nextPath);

//         cout<< numOfShifts << " " << bestShift.size() << endl;
         if(bestShift.size() == 0)
             break;
         numOfEdits = edits;
         numOfShifts++;

         currentHyp = bestShift;
     }


     numOfEdits += numOfShifts;
     return (double)numOfEdits / (double)ref.size();

 }

Sequence TERCalc::calcMinEdits(const std::vector<std::wstring> &hyp, const std::vector<std::wstring> &ref){
     map<Sequence, vector<int> > matchedRefSubstrs = buildWordMatches(hyp, ref);
     Sequence currentHyp = hyp;
     int numOfShifts = 0, numOfEdits = minEditDistance(currentHyp, ref);
     vector<int> bestPath = getPath(currentHyp, ref);

     while (true){

         vector<int> nextPath;
         Sequence bestShift;
         int edits = calcBestShift(currentHyp, ref, matchedRefSubstrs, bestPath, numOfEdits, bestShift, nextPath);

//         cout<< numOfShifts << " " << bestShift.size() << endl;
         if(bestShift.size() == 0)
             break;
         numOfEdits = edits;
         numOfShifts++;

         currentHyp = bestShift;
     }


     numOfEdits += numOfShifts;
//     return numOfEdits;

     return currentHyp;
 }


map<Sequence, vector<int> > TERCalc::buildWordMatches(const Sequence &hyp, const Sequence &ref){

     map<wstring , bool> hypWords;
     for(int i = 0; i < hyp.size(); i++){
         hypWords[hyp[i]] = true;
     }

     bool *cor = (bool *)calloc(sizeof(bool), ref.size() + 1);
     for(int i = 0; i < ref.size(); i++){
         if(hypWords.find(ref[i]) != hypWords.end())
             cor[i] = true;
         else
             cor[i] = false;
     }

     map<Sequence, vector<int> > toReturn;
     for(int start = 0; start < ref.size(); start++){
         if(cor[start]){
             Sequence toPush;
             for(int end = start; ((end < ref.size()) && (end - start <= MAX_SHIFT_SIZE) && cor[end]); end++){
                 toPush.push_back(ref[end]);

                 if(toReturn.find(toPush) != toReturn.end()){
                     toReturn[toPush].push_back(start);
                 }else{
                     vector<int> v;
                     toReturn[toPush] = v;
                     toReturn[toPush].push_back(start);
                 }

             }
         }
     }

     free(cor);

     return toReturn;
 }



 void TERCalc::gatherPossShifts(const Sequence &hyp, const Sequence &ref,
                      const map<Sequence, vector<int> > &matchedRefSubstrs,
                       vector<bool> &herr, vector<bool> &rerr, vector<int> &ralign, vector<vector<TERShift> > &res){


     int allShifts = MAX_SHIFT_SIZE + 1;
     for(int i = 0; i < allShifts; i++)
         res.push_back(vector<TERShift> ());

     int hypLen = hyp.size();
     int refLen = ref.size();

     for(int start = 0; start < hypLen; start++){
         Sequence sub = getSubSequence(hyp, start, start + 1);
         if(matchedRefSubstrs.find(sub) == matchedRefSubstrs.end())
             continue;

         bool ok = false;

         vector<int> itr = (*matchedRefSubstrs.find(sub)).second;
         for (int i = 0; i < itr.size() && !ok; i++){
             int moveto = itr[i];
//             cout << "moveto : " << start << " " << ralign[moveto] << endl;
             if((start != ralign[moveto]) &&
                     (ralign[moveto] - start <= MAX_SHIFT_DIST) &&
                     ((start - ralign[moveto] - 1) <= MAX_SHIFT_DIST))
                 ok = true;
         }

         if(!ok)
             continue;

         for(int end = start; (ok && (end < hypLen) && (end < start + MAX_SHIFT_SIZE)); end++){

             /* check if cand is good if so, add it */
             Sequence cand = getSubSequence(hyp, start, end + 1);
             ok = false;
             if(matchedRefSubstrs.find(cand) == matchedRefSubstrs.end())
                 break;

             bool any_herr = false;
             for(int i = 0; (i <= end - start) && !any_herr; i++){
                 if(herr[start + i])
                     any_herr = true;
             }

             if(any_herr == false){
                 ok = true;
                 continue;
             }

             bool* mark = (bool *)calloc(sizeof(bool), hypLen + 1);
             memset(mark, 0, sizeof mark);
             mark[start] = true;

             vector<int> shiftto = matchedRefSubstrs.find(cand)->second;
             for(int i = 0; i < shiftto.size(); i++){
//                 cout << "MOVETO $$$$$$" << endl;
                 int moveto = shiftto[i];
                 if (! ((ralign[moveto] != start) &&
                          ((ralign[moveto] < start) || (ralign[moveto] > end)) &&
                          ((ralign[moveto] - start) <= MAX_SHIFT_DIST) &&
                          ((start - ralign[moveto]) <= MAX_SHIFT_DIST)))
                     continue;
//                 cout << "MOVED.>>" << endl;
//                 cout << start << " " << moveto << endl;
                 ok = true;
                 bool any_err = false;
                 for(int i = 0; (i <= end - start) && !any_err; i++)
                     if(rerr[moveto + i])
                         any_err = true;
                 if(!any_err)
                     continue;


                 for(int roff = -1; roff <= (end - start); roff++){
//                     cout << "roff : " << moveto << " " << start << " " << ralign[moveto + roff] << endl ;

                     TERShift *shift = NULL;
                     if(roff == -1 && moveto == 0){
                         if(!mark[0]){
                             shift = new TERShift(start, end, 0);
                             mark[0] = true;
                         }

                     }else
                         if((start != ralign[moveto + roff]) && ((roff == 0) || ralign[moveto + roff] != ralign[moveto])){
                             int newLoc = ralign[moveto + roff] + 1;
                             shift = new TERShift(start, end, newLoc);
                             mark[newLoc] = true;
                         }
                     if(shift != NULL)
                         res[end - start].push_back(*shift);
                 }
             }
             free(mark);
         }
     }

 }

 Sequence TERCalc::performShift(const Sequence &hyp, TERShift shift){

     int start = shift.start;
     int end = shift.end;
     int moveto = shift.moveto;

     Sequence shiftedHyp;
     int c = 0;
     if(moveto <= start){

         for(int i = 0; i < moveto; i++)
             shiftedHyp.push_back(hyp[i]);
         for(int i = start; i <= end; i++)
             shiftedHyp.push_back(hyp[i]);
         for(int i = moveto; i < start; i++)
             shiftedHyp.push_back((hyp[i]));
         for(int i = end + 1; i < hyp.size(); i++)
             shiftedHyp.push_back(hyp[i]);

     }else if(moveto > end + 1){
         for(int i = 0; i < start; i++)
             shiftedHyp.push_back(hyp[i]);
         for(int i = end + 1; i < moveto; i++)
             shiftedHyp.push_back(hyp[i]);
         for(int i = start; i <= end; i++)
             shiftedHyp.push_back(hyp[i]);
         for(int i = moveto; i < hyp.size(); i++)
             shiftedHyp.push_back(hyp[i]);

     }else{
         // we are moving inside of a segment
         for(int i = 0; i < start; i++)
             shiftedHyp.push_back(hyp[i]);
         for(int i = end + 1; (i < hyp.size()) && (i <= (end + moveto - start - 1)); i++ )
             shiftedHyp.push_back(hyp[i]);
         for(int i = start; i <= end; i++)
             shiftedHyp.push_back(hyp[i]);
         for(int i = end + moveto - start; i < hyp.size(); i++)
             shiftedHyp.push_back(hyp[i]);
     }

     return shiftedHyp;
 }

 void TERCalc::findAlignErr(const vector<int> &path, vector<bool> &herr, vector<bool> &rerr, vector<int> &ralign){

     int hpos = -1, rpos = -1;

     for(int i = 0; i < path.size(); i++){
         int sym = path[i];
         if(sym == 0){
             hpos++;
             rpos++;
             herr[hpos] = false;
             rerr[hpos] = false;
             ralign[rpos] = hpos;
         }else if(sym == 1){
             hpos++;
             rpos++;
             herr[hpos] = true;
             rerr[rpos] = true;
             ralign[rpos] = hpos;
         }else if(sym == 2){
             hpos++;
             herr[hpos] = true;
         }else if(sym == 3){
             rpos++;
             rerr[rpos] = true;
             ralign[rpos] = hpos;
         }else{
             cout << "Error!  Invalid mini align sequence " << sym
                     << " at pos " << i << "\n";
         }

     }

 }

 int TERCalc::calcBestShift(const Sequence &hyp, const Sequence &ref,
                   const map<Sequence, vector<int> > &matchedRefSubstrs, const vector<int> &bestPath, int curEdits,
                   Sequence &bestShift, vector<int> &newBestPath){

     /* Arrays that records which hyp and ref words are currently wrong */
     vector<bool> herr(hyp.size(), false);
     vector<bool> rerr(ref.size(), false);
     /* Array that records the alignment between ref and hyp */
     vector<int> ralign(ref.size(), 0);


     findAlignErr(bestPath, herr, rerr, ralign);


//     for(int i = 0; i < hyp.size(); i++)
//         cout << herr[i] << " ";
//     cout << endl;
//     for(int i = 0; i < ref.size(); i++)
//         cout << rerr[i] << " ";
//     cout << endl;



    vector<vector<TERShift> > possShifts;
    gatherPossShifts(hyp, ref, matchedRefSubstrs, herr, rerr, ralign, possShifts);

//    cout << possShifts.size() << endl;
    Sequence x;
    bestShift = x;
    int minEdits = curEdits;
    int curShiftCost = 0;
    vector<int> v;
    newBestPath = v;

    for(int i = possShifts.size() - 1; i >= 0; i--){

//        cout << possShifts[i].size() << endl;
        for(int j = 0; j < possShifts[i].size(); j++){


            TERShift curShift = possShifts[i][j];
            Sequence next = performShift(hyp, curShift);
            int edits = minEditDistance(next, ref);
            double gain = (minEdits + curShiftCost) - (edits + 1);

//            cout << "gain : " << gain << endl;
            if(gain > 0 || (gain == 0 && curShiftCost == 0)){

                minEdits = edits;
                bestShift = next;
                newBestPath = getPath(next, ref);
                curShiftCost = 1;
            }

        }
    }

    return minEdits;

 }

 vector<int> TERCalc::getPath(const Sequence &hyp, const Sequence &ref){

     int i = ref.size(), j = hyp.size();
     vector<int> res;

     int nxt[4][2] = { { -1, -1 }, { -1, -1 }, { 0, -1 }, { -1, 0 } };
     while (i > 0 || j > 0) {
         res.push_back(path[i][j]);
         int x = path[i][j];
         i += nxt[x][0];
         j += nxt[x][1];
     }

     reverse(res.begin(), res.end());

     return res;

 }

 int TERCalc::minEditDistance(const Sequence &hyp, const Sequence &ref){

     double current_best = INF;
     double last_best = INF;
     int first_good = 0;
     int current_first_good = 0;
     int last_good = -1;
     int cur_last_good = 0;
     int last_peak = 0;
     int cur_last_peak = 0;
     int cost, icost, dcost;
     int score;

     int hlen = hyp.size();
     int rlen = ref.size();
     dp.clear();
     path.clear();
     int m = max(hlen, rlen) + 20;
     dp_len = m;

     for(int i = 0; i < m; i++){
         vector<int> v(m, 0);
         dp.push_back(v);
         path.push_back(v);
     }

     for (int i = 0; i <= rlen; i++)
         for (int j = 0; j <= hlen; j++)
             dp[i][j] = -1;
     dp[0][0] = 0;

     for(int j = 0; j <= hlen; j++){
         last_best = current_best;
         current_best = INF;

         first_good = current_first_good;
         current_first_good = -1;

         last_good = cur_last_good;
         cur_last_good = -1;

         last_peak = cur_last_peak;
         cur_last_peak = 0;

         for(int i = first_good; i <= rlen; i++){
             if(i > last_good)
                 break;
             if(dp[i][j] < 0)
                 continue;
             score = dp[i][j];

             if((j < hlen) && (score > last_best + BEAM_WIDTH))
                 continue;
             if(current_first_good == -1)
                 current_first_good = i;

             if((i < rlen) && (j < hlen)){

                 if(ref[i] == hyp[j] || (ref[i].size())){
                     cost = score;

                     if((dp[i + 1][j + 1] == -1) || (cost < dp[i + 1][j + 1])){
                         dp[i + 1][j + 1] = cost;
                         path[i + 1][j + 1] = 0;
                     }

                     if(cost < current_best)
                         current_best = cost;
                     if(current_best == cost)
                         cur_last_peak = i + 1;
                 }else{
                     cost = 1 + score;
                     if((dp[i + 1][j + 1] < 0) || (cost < dp[i + 1][j + 1])){
                         dp[i + 1][j + 1] = cost;
                         path[i + 1][j + 1] = 1;

                         if(cost < current_best)
                             current_best = cost;
                         if(current_best == cost)
                             cur_last_peak = i + 1;
                     }
                 }
             }

             cur_last_good = i + 1;

             if(j < hlen){  // delete from hyp
                 icost = score + 1;
                 if((dp[i][j + 1] < 0) || (dp[i][j + 1] > icost)){
                     dp[i][j + 1] = icost;
                     path[i][j + 1] = 2;
                     if((cur_last_peak < i) && (current_best == icost))
                         cur_last_peak = i;
                 }
             }

             if(i < rlen){  // insert to hyp
                 dcost = score + 1;
                 if((dp[i + 1][j] < 0) || (dp[i + 1][j] > dcost)){
                     dp[i + 1][j] = icost;
                     path[i + 1][j] = 3;
                     if(i >= last_good)
                         last_good = i + 1;
                 }
             }
         }


     }

     return dp[rlen][hlen];
 }

 Sequence TERCalc::getSubSequence(Sequence s, int start, int end){
     Sequence x;
     for(int i = start; i < end; i++)
         x.push_back(s[i]);
     return x;
 }

}
