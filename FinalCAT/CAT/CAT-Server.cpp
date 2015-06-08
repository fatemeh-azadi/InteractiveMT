// $Id: MainMT.cpp 3045 2010-04-05 13:07:29Z hieuhoang1972 $

/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2009 University of Edinburgh

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

/**
 * Moses main, for single-threaded and multi-threaded.
 **/

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <exception>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include "util/usage.hh"

#ifdef WIN32
// Include Visual Leak Detector
//#include <vld.h>
#endif

#include "moses-cmd/TranslationAnalysis.h"
#include "moses-cmd/IOWrapper.h"
#include "moses-cmd/mbr.h"

#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/StaticData.h"
#include "moses/Util.h"
#include "moses/Timer.h"
#include "moses/ThreadPool.h"
#include "moses/OutputCollector.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/LM/Base.h"
#include "moses/LM/Implementation.h"
#include "server.h"
#include "TranslationTask.h"
#include "CaitraSearchGraph.h"
#include "searchnbest.h"
#include "CaitraWithWindow.h"
#include "CaitraWordVec.h"
#include "ECModelSearchGraph.h"
#include "SearchGraph.h"
#include "ModifiedCaitra.h"
#include "OnlineLearner.h"
#include "CaitraWithJump.h"
#include "WeightedCaitraWithJump.h"
#ifdef HAVE_PROTOBUF
#include "hypergraph.pb.h"
#endif

using namespace CATServer;
using namespace Caitra;
using namespace CaitraWindow;
using namespace CaitraWordVector;

void split_marker_perl(string str, string marker, vector<string> &array) {
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

}


std::map <string,vector<string> > my_vocab;

void build_vocab(string lmFile)
{
    ifstream infile(lmFile.c_str());
    std::string line;
    bool flag=false;
    vector <string> words;
    string str;
    int count=0;
    string wrd;
    while ( getline(infile, line) )
    {
        count++;
        if (!flag)
        {
            size_t pos=line.find("\\1-grams:");
            if (pos==string::npos)
            {
                continue;
            }
            else
            {
                flag=true;
                continue;
            }
        }
        else
        {

            size_t pos=line.find("\\2-grams:");
            if (pos!=string::npos)
            {
                break;
            }
            else
            {
                boost::split(words, line, boost::is_any_of("\t "));
                str=words[1];
                if (str.find("<s>")==string::npos && str.find("</s>")==string::npos && str.find("|")==string::npos )
                {
                    char ch=(str.c_str())[0];
                   // if (ch>=65 && ch<=122)
                        wrd=str.substr(0,1);
                   // else
                   //     wrd=str.substr(0,2);

                    if (my_vocab.find(wrd)==my_vocab.end())
                    {
                        vector<string> a;
                        a.push_back(str);
                        my_vocab.insert(make_pair(wrd,a));
                    }
                    else
                        my_vocab[wrd].push_back(str);
                    //cout<<wrd<<"    "<<str<<endl;
                }
            }
        }
    }
}

void getAllVocab(std::vector<std::string>& array_words){
    for (map<string, vector<string> >::iterator it = my_vocab.begin(); it != my_vocab.end(); it++){
        vector<string> v = it->second;
        for(int i = 0; i < v.size(); i++)
            array_words.push_back(v[i]);
    }
}

void BackOff(std::vector<std::string> words, std::vector<std::string>& array_words)
{
    string str;
    string wrd;
    double max=-1000;
    int index=-1;
    str=words[words.size()-1];
    char ch=(str.c_str())[0];
//    if (ch>=65 && ch<=122)
        wrd=str.substr(0,1);
//    else
//        wrd=str.substr(0,2);

    if (my_vocab.find(wrd)!=my_vocab.end())
    {
        for(int i=0;i<my_vocab[wrd].size();i++)
        {
            if (my_vocab[wrd][i].find(str)==0 && strcmp(my_vocab[wrd][i].c_str(),str.c_str())!=0)
            {
                array_words.push_back(my_vocab[wrd][i]);
            }
        }

    }
}

double CalcNgramLMScore(const LanguageModelImplementation* LM, vector<string> &words, string nextWord, int Ngram, string fullOrNgram){

    string y[10];
    y[1]=nextWord;
    int s = 0, k = 0;
    double res = 0;
    for(int i = 2; i <= Ngram; i++){
        if(i > 1){
            y[i] = "";
        if((int)words.size() >= i - 1)
            y[i] = words[((int)words.size()) - i + 1];
        else if(s == 0){
            y[i] = LM->GetSentenceStartWord().ToString();
            s = 1;
        }else
            k++;
        if(y[i - 1].length() > 0 && y[i].length() > 0)
            y[i] += " ";
        y[i] += y[i - 1];
        }

    Phrase phrase(Output);
    phrase.CreateFromString(Output, StaticData::Instance().GetOutputFactorOrder(), y[i] ,StaticData::Instance().GetFactorDelimiter(), NULL );
    float FullScore;
    float NGramScore;
    size_t oovCount;


    LM->CalcNGramScore(phrase,i - k, FullScore, NGramScore, oovCount);
    if(fullOrNgram == "Full")
    res += NGramScore;
    else
        res = NGramScore;
    }
    if(Ngram == 1){
        Phrase phrase(Output);
        phrase.CreateFromString(Output, StaticData::Instance().GetOutputFactorOrder(), y[1] ,StaticData::Instance().GetFactorDelimiter(), NULL );
        float FullScore;
        float NGramScore;
        size_t oovCount;


        LM->CalcNGramScore(phrase,1, FullScore, NGramScore, oovCount);

        res += NGramScore;
    }

    //  cout << x << "  *****  " << NGramScore << endl;
   // return NGramScore;
    return res;


}

double getWorstScore(const LanguageModelImplementation* LM, int Ngram){

    Phrase phrase(Output);
    phrase.CreateFromString(Output, StaticData::Instance().GetOutputFactorOrder(), "abcd abcd abcd dbbd" ,StaticData::Instance().GetFactorDelimiter(), NULL );
    float FullScore;
    float NGramScore;
    size_t oovCount;


    LM->CalcNGramScore(phrase,Ngram, FullScore, NGramScore, oovCount);

    return NGramScore;
}

/** main function of the command line version of the decoder **/
int main(int argc, char** argv) {
    try {

#ifdef HAVE_PROTOBUF
        GOOGLE_PROTOBUF_VERIFY_VERSION;
#endif

        // echo command line, if verbose
        IFVERBOSE(1) {
            TRACE_ERR("command: ");
            for (int i = 0; i < argc; ++i)
                TRACE_ERR(argv[i]<<" ");
            TRACE_ERR(endl);
        }

        int port = 9999;


        string method = "Caitra";
        string onlineMetric = "BLEU";
        string wordVecFile = "";
        string stopWords = "";
        float wlr = 0.2;
        double errorWeigth = -0.5;
        string onlineMethod = "none";
        string v;
        double stpError = 0.5;
        int nbSize = 100;
        int backOff = 0;
        int backOffNgram = 4;
        double backOffWeight = 0.5;
        int jumpLenTHR = 10000;
        string fullorngram="ngram";
        if(getline(cin, v)){
            vector<string> line;
            split_marker_perl(v, " ", line);

            for(int i = 0; i < line.size(); i+= 2){
                if(line[i] == "-p"){
                    port = atoi(line[i + 1].c_str());
                }else if(line[i] == "--method" || line[i] == "-m")  /* Caitra , ModifiedCaitra, WeightedCaitra, CaitraWithJump, WeightedCaitraWithJump, WordVector, ECModel , StopWords, both*/
                    method = line[i + 1];
                else if(line[i] == "--onlineMethod" || line[i] == "-o")  // none, Mira
                    onlineMethod = line[i + 1];
                else if(line[i] == "--onlineMetric")
                    onlineMetric = line[i + 1];
                else if(line[i] == "-wlr")
                    wlr = atof(line[i + 1].c_str());
                else if(line[i] == "--ErrorWeight" || line[i] == "-errw")
                    errorWeigth = atof(line[i + 1].c_str());
                else if(line[i] == "--jumpLen")
                    jumpLenTHR = atoi(line[i + 1].c_str());
                else if(line[i] == "--StopWords")
                    stopWords = line[i + 1];
                else if(line[i] == "--WordVector")
                    wordVecFile = line[i + 1];
                else if(line[i] == "--StopWords-Error")
                    stpError = atof(line[i + 1].c_str());
                else if(line[i] == "--nbest-size" )
                    nbSize = atoi(line[i + 1].c_str());
                else if(line[i] == "--backOff")   // 0 = no backoff, 1 = LM Backoff , 2 = wordvector Backoff , 3 = LM then WordVector Backoff
                    // 4 = weighted WordVector BackOff , 5 = LM then Weighted WordVector
                    backOff = atoi(line[i + 1].c_str());
                else if(line[i] == "--backOffNgram")
                    backOffNgram = atoi(line[i + 1].c_str());
                else if(line[i] == "--backOffWeight")
                    backOffWeight = atof(line[i + 1].c_str());
                else if(line[i] == "--FullOrNgram")
                    fullorngram = line[i + 1];
                else {
                    cerr << "Error : invalid input ...." << endl;
                    exit(1);
                }

            }
        }



        // set number of significant decimals in output
        fix(cout, PRECISION);
        fix(cerr, PRECISION);

        // load all the settings into the Parameter class
        // (stores them as strings, or array of strings)
        Parameter params;
        if (!params.LoadParam(argc, argv)) {
            exit(1);
        }

        // initialize all "global" variables, which are stored in StaticData
        // note: this also loads models such as the language model, etc.
        if (!StaticData::LoadDataStatic(&params, argv[0])) {
            exit(1);
        }

        // shorthand for accessing information in StaticData
        const StaticData& staticData = StaticData::Instance();

        // setting "-show-weights" -> just dump out weights and exit
        if (params.isParamSpecified("show-weights")) {

            CATServer::ShowWeights();
            exit(0);
        }

        //initialise random numbers
        srand(time(NULL));

        // set up read/writing class
        IOWrapper* ioWrapper = GetIOWrapper(staticData);
        if (!ioWrapper) {
            cerr << "Error; Failed to create IO object" << endl;
            exit(1);
        }

        // check on weights
        const ScoreComponentCollection& weights = staticData.GetAllWeights();
        IFVERBOSE(2) {
            TRACE_ERR("The global weight vector looks like this: ");
            TRACE_ERR(weights);
            TRACE_ERR("\n");
        }
        if (staticData.GetOutputSearchGraphHypergraph()) {
            ofstream* weightsOut = new std::ofstream;
            stringstream weightsFilename;
            if (staticData.GetParam("output-search-graph-hypergraph").size()
                    > 3) {
                weightsFilename
                        << staticData.GetParam("output-search-graph-hypergraph")[3];
            } else {
                string nbestFile = staticData.GetNBestFilePath();
                if (!nbestFile.empty() && nbestFile != "-"
                        && !boost::starts_with(nbestFile, "/dev/stdout")) {
                    boost::filesystem::path nbestPath(nbestFile);
                    weightsFilename << nbestPath.parent_path().filename()
                                    << "/weights";
                } else {
                    weightsFilename << boost::filesystem::current_path()
                                    << "/hypergraph/weights";
                }
            }
            boost::filesystem::path weightsFilePath(weightsFilename.str());
            if (!boost::filesystem::exists(weightsFilePath.parent_path())) {
                boost::filesystem::create_directory(
                            weightsFilePath.parent_path());
            }
            TRACE_ERR("The weights file is " << weightsFilename.str() << "\n");
            weightsOut->open(weightsFilename.str().c_str());
            OutputFeatureWeightsForHypergraph(*weightsOut);
            weightsOut->flush();
            weightsOut->close();
            delete weightsOut;
        }

        // initialize output streams
        // note: we can't just write to STDOUT or files
        // because multithreading may return sentences in shuffled order
        auto_ptr<OutputCollector> outputCollector; // for translations
        auto_ptr<OutputCollector> nbestCollector;  // for n-best lists
        auto_ptr<OutputCollector> latticeSamplesCollector; //for lattice samples
        auto_ptr<ofstream> nbestOut;
        auto_ptr<ofstream> latticeSamplesOut;
        size_t nbestSize = staticData.GetNBestSize();
        string nbestFile = staticData.GetNBestFilePath();
        bool output1best = true;
        if (nbestSize) {
            if (nbestFile == "-" || nbestFile == "/dev/stdout") {
                // nbest to stdout, no 1-best
                nbestCollector.reset(new OutputCollector());
                output1best = false;
            } else {
                // nbest to file, 1-best to stdout
                nbestOut.reset(new ofstream(nbestFile.c_str()));
                if (!nbestOut->good()) {
                    TRACE_ERR(
                                "ERROR: Failed to open " << nbestFile << " for nbest lists" << endl);
                    exit(1);
                }
                nbestCollector.reset(new OutputCollector(nbestOut.get()));
            }
        }
        size_t latticeSamplesSize = staticData.GetLatticeSamplesSize();
        string latticeSamplesFile = staticData.GetLatticeSamplesFilePath();
        if (latticeSamplesSize) {
            if (latticeSamplesFile == "-"
                    || latticeSamplesFile == "/dev/stdout") {
                latticeSamplesCollector.reset(new OutputCollector());
                output1best = false;
            } else {
                latticeSamplesOut.reset(
                            new ofstream(latticeSamplesFile.c_str()));
                if (!latticeSamplesOut->good()) {
                    TRACE_ERR(
                                "ERROR: Failed to open " << latticeSamplesFile << " for lattice samples" << endl);
                    exit(1);
                }
                latticeSamplesCollector.reset(
                            new OutputCollector(latticeSamplesOut.get()));
            }
        }
        if (output1best) {
            outputCollector.reset(new OutputCollector());
        }

        // initialize stream for word graph (aka: output lattice)
        auto_ptr<OutputCollector> wordGraphCollector;
        if (staticData.GetOutputWordGraph()) {
            wordGraphCollector.reset(
                        new OutputCollector(
                            &(ioWrapper->GetOutputWordGraphStream())));
        }

        // initialize stream for search graph
        // note: this is essentially the same as above, but in a different format
        auto_ptr<OutputCollector> searchGraphCollector;
        if (staticData.GetOutputSearchGraph()) {
            searchGraphCollector.reset(
                        new OutputCollector(
                            &(ioWrapper->GetOutputSearchGraphStream())));
        }

        // initialize stream for details about the decoder run
        auto_ptr<OutputCollector> detailedTranslationCollector;
        if (staticData.IsDetailedTranslationReportingEnabled()) {
            detailedTranslationCollector.reset(
                        new OutputCollector(
                            &(ioWrapper->GetDetailedTranslationReportingStream())));
        }

        // initialize stream for word alignment between input and output
        auto_ptr<OutputCollector> alignmentInfoCollector;
        if (!staticData.GetAlignmentOutputFile().empty()) {
            alignmentInfoCollector.reset(
                        new OutputCollector(ioWrapper->GetAlignmentOutputStream()));
        }

        //initialise stream for unknown (oov) words
        auto_ptr<OutputCollector> unknownsCollector;
        auto_ptr<ofstream> unknownsStream;
        if (!staticData.GetOutputUnknownsFile().empty()) {
            unknownsStream.reset(
                        new ofstream(staticData.GetOutputUnknownsFile().c_str()));
            if (!unknownsStream->good()) {
                TRACE_ERR(
                            "Unable to open " << staticData.GetOutputUnknownsFile() << " for unknowns");
                exit(1);
            }
            unknownsCollector.reset(new OutputCollector(unknownsStream.get()));
        }


        Server *server = new Server(port);

        //    cout << "server.....";

        // main loop over set of input sentences
        size_t lineCount = staticData.GetStartTranslationId();

        TranslationTask* task = NULL;
        SearchNBest * nbestSearch = NULL;
        SearchGraph* searchGraph = NULL;
        OnlineLearner* onlineLearner = new OnlineLearner(wlr, nbSize, onlineMethod, onlineMetric);
        Manager *m_manager = NULL;
        WordVector *wordVector = NULL;
        if(method == "StopWords" || method == "WordVector" || method == "both"){
            searchGraph = new CaitraWordVec(method, errorWeigth,stpError);
            ((CaitraWordVec*)searchGraph)->readStopWords(stopWords);
            if(method == "WordVector" || method == "both")
                ((CaitraWordVec*)searchGraph)->readWordVec(wordVecFile);
        }

        int interaction = 0;
        double interactionTimes = 0;
        const LanguageModelImplementation* LM = NULL;
        if(backOff != 0 || method == "CaitraWithJump" || method == "WeightedCaitraWithJump"){
            const std::vector<const StatefulFeatureFunction*> &statefulFFs = StatefulFeatureFunction::GetStatefulFeatureFunctions();
            for (size_t i = 0; i < statefulFFs.size(); ++i) {
                const StatefulFeatureFunction *ff = statefulFFs[i];
                if (const LanguageModel *lm = dynamic_cast<const LanguageModel*>(ff)) {


                    LM = dynamic_cast<const LanguageModelImplementation*> (lm);
                    if(backOff != 0)
                        build_vocab(LM->GetFilePath());
                }
            }
            if(backOff > 1){
                wordVector = new WordVector();
                wordVector->readWordVec(wordVecFile);
            }
        }

        int sgSize = 0, transitionsSize = 0;
        int LMBackOff = 0, WVecBackOff = 0, emptySuffix = 0, secondLMBackOff = 0, totalBackOff = 0;
        while (1) {

            const char* s1 = server->get_message();
            if (strcmp(s1, "new_sentence") == 0) {

                server->set_message("ok");
                const char* s2 = server->get_message();

                string s3 = string(s2) + "\n";

                Sentence* source = new Sentence();
                const vector<FactorType> &inputFactorOrder =
                        staticData.GetInputFactorOrder();
                stringstream in(s3);

                source->Read(in, inputFactorOrder);

                IFVERBOSE(1) {
                    ResetUserTime();
                }

                if(nbestSearch)
                    delete nbestSearch;
                delete task;
                if(staticData.GetOutputSearchGraph()){
                    if(method != "WordVector" && method != "StopWords" && method != "both" && searchGraph)
                        delete searchGraph;
                    if(method == "ECModel")
                        searchGraph = new ECModelSearchGraph();
                    else if(method == "Caitra")
                        searchGraph = new MainCaitra();
                    else if(method == "Window")
                        searchGraph = new CaitraWithWindow();
                    else if(method == "WeightedCaitra")
                        searchGraph = new CaitraSearchGraph(errorWeigth);
                    else if(method == "ModifiedCaitra")
                        searchGraph = new ModifiedCaitra();
                    else if(method == "CaitraWithJump")
                        searchGraph = new CaitraWithJump(LM);
                    else if(method == "WeightedCaitraWithJump")
                        searchGraph = new WeightedCaitraWithJump(errorWeigth, LM, jumpLenTHR);

                }
                else if(nbestSize)
                    nbestSearch = new SearchNBest();
                if(m_manager)
                    delete m_manager;
                m_manager = new Manager(lineCount,*source, staticData.GetSearchAlgorithm());

                task = new TranslationTask(lineCount,source, outputCollector.get(),
                                           nbestCollector.get(),
                                           latticeSamplesCollector.get(),
                                           wordGraphCollector.get(),
                                           searchGraphCollector.get(),
                                           detailedTranslationCollector.get(),
                                           alignmentInfoCollector.get(),
                                           unknownsCollector.get(),
                                           staticData.GetOutputSearchGraphSLF(),
                                           staticData.GetOutputSearchGraphHypergraph(),
                                           searchGraph, nbestSearch, m_manager);

                task->Run();

                ++lineCount;

                sgSize += searchGraph->getGraphSize();
                transitionsSize += searchGraph->getTransitionsSize();
                //    cout << "Translation Finished" << endl;



            }else if(strcmp(s1, "prefix") == 0 || strcmp(s1, "no-prefix") == 0){


                Timer searchTime;
                searchTime.start();
                server->set_message("ok");
                //    cout << "get prefix: " << s1 << endl;

                string s4 = "", s3 = "ABDC";
                if(strcmp(s1,"prefix") == 0){

                    s4 = server->get_message();
                    TRACE_ERR("Prefix : " + s4 + "\n");
                    //      cout << s4 << endl;
                    //if(method == "Caitra" || method == "Window" || method == "WordVector" || method == "ECModel")
                    s3 = searchGraph->getPrefix(s4);
                    //else if(nbestSize){
                    //                        s3 = nbestSearch->searchPrefix_WordLevel(s4);
                    //   s3 = nbestSearch->searchPrefix_TER(s4);
                    //}

                    //                    s3 = task -> searchPrefix(s4);
                    TRACE_ERR("Translation : " + s3 + "\n");
                    interactionTimes += searchTime.get_elapsed_time();

                    interaction ++;
                    TRACE_ERR("Interaction Time = " << searchTime << "\n" << "\n");
                }
                else{
                    TRACE_ERR("No Prefix \n");
                    s3 = task->getTargetSentence();
                    TRACE_ERR("Translation : " + s3 + "\n");
                }



                /*****
                 * adding LM backoff
                 *
                 ****/

                if(s3 == "" && backOff != 0){
                    totalBackOff++;
                    Timer searchTime;
                    searchTime.start();


                    std::vector<std::string> words;

                    std::vector<std::string> possibleWords;

                    bool space_flag = false;
                    //  boost::split(ref_words, line, boost::is_any_of(" "));
                    boost::split(words, s4, boost::is_any_of(" "));
                    while (words[words.size()-1]=="")
                    {
                        words.pop_back();
                        space_flag=true;
                    }
                    int t_pos=words.size();
                    if (space_flag){
                        t_pos=t_pos+1;
                        words.push_back("");
                    }

                    if(backOff == 1 || backOff == 3 || backOff == 5){
                        if(space_flag)
                            getAllVocab(possibleWords);
                        else
                            BackOff(words, possibleWords);

                        if(possibleWords.size()>0)
                        {
                            //   string str=words[words.size()-1];
                            string prev_Word = "";
                            string bestWord = "";
                            float bestNGramScore = -100000;
                            string str = words.back();
                            if(words.size() > 0)
                                words.pop_back();

                            vector<pair<double, string> > LMScores;
                            for(int i = 0; i < possibleWords.size(); i++){
                                double ngramScore =CalcNgramLMScore(LM, words, possibleWords[i], backOffNgram, fullorngram);
                                LMScores.push_back(pair<double, string> (ngramScore, possibleWords[i]));
                            }
                            sort(LMScores.rbegin(), LMScores.rend());


                            if(LMScores.size() > 0){

                                double NGramScore = LMScores[0].first;

                                //cout << (prev_Word + " " + possibleWords[i]) << "   :    " << " " << NGramScore << endl;
                                if(NGramScore > bestNGramScore){
                                    if(backOff == 1 || NGramScore > getWorstScore(LM, backOffNgram) * 0.7){
                                    bestNGramScore = NGramScore;
                                    bestWord = LMScores[0].second;
                                    }

                                }
                            }
                            if(bestWord.length() > 0){
                            s3 = bestWord.substr(str.length());
                            LMBackOff++;
                            }
                            words.push_back(str);
                        }

                    }
                    if(backOff == 2 || (backOff == 3 && s3 == "")){

                        getAllVocab(possibleWords);
                        vector<pair<double, string> > LMScores;
                        string str = words.back();
                        if(words.size() > 0)
                            words.pop_back();
                        for(int i = 0; i < possibleWords.size(); i++){
                            double ngramScore =CalcNgramLMScore(LM, words, possibleWords[i], backOffNgram, fullorngram);
                            LMScores.push_back(pair<double, string> (ngramScore, possibleWords[i]));
                        }
                        string bestWord = "";
                        float bestNGramScore = -100000;
                        sort(LMScores.rbegin(), LMScores.rend());
                        vector<int> wordsHavingLastWord = wordVector->getVocabStartsWith(str);
                        for(int i = 0; i < LMScores.size(); i++){
                            //string x = wordVector->findBestComplete(LMScores[i].second, wordsHavingLastWord);
                            if(LMScores[i].second.find(str) == 0 && strcmp(LMScores[i].second.c_str(),str.c_str())!=0){
                                bestWord = LMScores[i].second;
                                secondLMBackOff++;
                                break;
                            }
                            int idx = wordVector->getIndex(LMScores[i].second);
                            for(int j = 0; j < wordsHavingLastWord.size(); j++){
                                double sc = wordVector->getDistance(idx, wordsHavingLastWord[j]);
                                if(sc > 0.7 && sc > bestNGramScore){
                                    bestNGramScore = sc;
                                    bestWord = wordVector->getWord(wordsHavingLastWord[j]);
                                }

                            }
                            if(bestWord.length() > 0){
                                break;
                            }
                        }
                        if(bestWord.length() > 0){
                            s3 = bestWord.substr(str.length());
                            WVecBackOff++;
                        }
                        words.push_back(str);
                    }
                    if(backOff == 4 || (backOff == 5 && s3 == "")){

                        getAllVocab(possibleWords);
                        vector<pair<double, string> > LMScores;
                        string str = words.back();
                        if(words.size() > 0)
                            words.pop_back();

                        for(int i = 0; i < possibleWords.size(); i++){
                            double ngramScore =CalcNgramLMScore(LM, words, possibleWords[i], backOffNgram,fullorngram);
                            LMScores.push_back(pair<double, string> (ngramScore, possibleWords[i]));
                        }
                        string bestWord = "";
                        float bestNGramScore = -100000;
                        sort(LMScores.rbegin(), LMScores.rend());

                        vector<int> wordsHavingLastWord = wordVector->getVocabStartsWith(str);
                        int lmTriggers = 0;
                        for(int i = 0; i < LMScores.size(); i++){
                            //double ngramScore =CalcNgramLMScore(LM, words, possibleWords[i], backOffNgram);
                            double ngramScore = LMScores[i].first;
                            if(ngramScore < LMScores[0].first * 2)
                                continue;
                            //cout << "ngramScore : " << ngramScore << endl;
                            //LMScores.push_back(pair<double, string> (ngramScore, possibleWords[i]));
                            if(LMScores[i].second.find(str) == 0 && strcmp(LMScores[i].second.c_str(),str.c_str())!=0){
                                double totalScore = (1 - backOffWeight) * ngramScore;
                                if(totalScore > bestNGramScore){
                                    totalScore = bestNGramScore;
                                    bestWord = LMScores[i].second;
                                    lmTriggers = 1;
                                }

                            }
                            else{
                                int idx = wordVector->getIndex(LMScores[i].second);
                                for(int j = 0; j < wordsHavingLastWord.size(); j++){
                                    double sc = wordVector->getDistance(idx, wordsHavingLastWord[j]);
                                    if(sc < 0.6)
                                        continue;
                                    double totalScore = (1 - backOffWeight) * ngramScore + backOffWeight * log(sc) * 2;
                                    if(totalScore > bestNGramScore){
                                        bestNGramScore = totalScore;
                                        bestWord = wordVector->getWord(wordsHavingLastWord[j]);
                                        lmTriggers = 0;
                                    }

                                }
                            }



                        }

                        if(bestWord.length() > 0){
                            s3 = bestWord.substr(str.length());
                            WVecBackOff++;
                            secondLMBackOff +=  lmTriggers;
                        }

                    }

                    interactionTimes += searchTime.get_elapsed_time();


                }

                if(s3 == "")
                    emptySuffix++;
                server -> set_message(s3.c_str());

            }else if (strcmp(s1,"close")==0){

                server->s_close();
                break;

            }else if(strcmp(s1, "update") == 0){

                server->set_message("ok");
                string ref = server->get_message();
                //cout << ref << endl;
                if(onlineMethod != "none"){
                    onlineLearner->RemoveJunk();
                    onlineLearner->SetPostEditedSentence(ref);
                    onlineLearner->RunOnlineLearning(m_manager);


                }
                //
                //  ShowWeights();
                server->set_message("ok");
            }


        }


        if(m_manager)
            delete m_manager;
        if(task)
            delete task;
        if(wordVector)
            delete wordVector;
        if(searchGraph)
            delete searchGraph;

        server->s_close();


        TRACE_ERR("Total Interaction Time = " << interactionTimes << " Total Interactions = " << interaction << "\n");
        TRACE_ERR("Average Search Time = " << (double)interactionTimes / interaction << "\n");
        TRACE_ERR("Total Search Graphs Sizes = " << sgSize << "\n");
        TRACE_ERR("Total Transitions Size = " << transitionsSize << "\n");
        TRACE_ERR("LM BackOff Usage = " << LMBackOff << " , WordVector BackOff Usage = " << WVecBackOff << " , LM Usage in WordVector BackOff = " << secondLMBackOff << " , Empty Suffixes = " << emptySuffix << "\n");
        TRACE_ERR("Total BackOff Calls = " << totalBackOff << "\n");
        delete ioWrapper;

    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }


    IFVERBOSE(1)
            util::PrintUsage(std::cerr);

#ifndef EXIT_RETURN
    //This avoids that destructors are called (it can take a long time)
    exit(EXIT_SUCCESS);
#else
    return EXIT_SUCCESS;
#endif
}

