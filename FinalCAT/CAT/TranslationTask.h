#ifndef TRANSLATIONTASK_H
#define TRANSLATIONTASK_H



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

#include "searchnbest.h"
#include "SearchGraph.h"
#ifdef HAVE_PROTOBUF
#include "hypergraph.pb.h"
#endif
using namespace std;
using namespace Moses;
using namespace MosesCmd;
namespace CATServer {

// output floats with three significant digits
static const size_t PRECISION = 3;
/** Enforce rounding */
void fix(std::ostream& stream, size_t size);

/** Translates a sentence.
 * - calls the search (Manager)
 * - applies the decision rule
 * - outputs best translation and additional reporting
 **/
class TranslationTask: public Task {

public:

    TranslationTask(size_t lineNumber,
                    InputType* source,
                    OutputCollector* outputCollector,
                    OutputCollector* nbestCollector,
                    OutputCollector* latticeSamplesCollector,
                    OutputCollector* wordGraphCollector, OutputCollector* searchGraphCollector,
                    OutputCollector* detailedTranslationCollector,
                    OutputCollector* alignmentInfoCollector,
                    OutputCollector* unknownsCollector,
                    bool outputSearchGraphSLF,
                    bool outputSearchGraphHypergraph,
                    SearchGraph *sg ,SearchNBest* nbestsearch, Manager* m_manager);
    /** Translate one sentence
     * gets called by main function implemented at end of this source file */
    void Run() ;

//    string searchPrefix(string prefix){
//        return searchGraph->searchPrefix(prefix);
//    }

    string getTargetSentence();

    vector<SearchGraphNode> getSearchGraph();

    ~TranslationTask() ;


private:
    InputType* m_source;
    size_t m_lineNumber;
    OutputCollector* m_outputCollector;
    OutputCollector* m_nbestCollector;
    OutputCollector* m_latticeSamplesCollector;
    OutputCollector* m_wordGraphCollector;
    OutputCollector* m_searchGraphCollector;
    OutputCollector* m_detailedTranslationCollector;
    OutputCollector* m_alignmentInfoCollector;
    OutputCollector* m_unknownsCollector;
    bool m_outputSearchGraphSLF;
    bool m_outputSearchGraphHypergraph;
    std::ofstream *m_alignmentStream;
    Manager *m_Manager;
    SearchGraph* m_searchGraph;
    SearchNBest* m_nbestSearch;
    string targetSentence; // the best translation output




};

static void PrintFeatureWeight(const FeatureFunction* ff) {
    cout << ff->GetScoreProducerDescription() << "=";
    size_t numScoreComps = ff->GetNumScoreComponents();
    vector<float> values =
            StaticData::Instance().GetAllWeights().GetScoresForProducer(ff);
    for (size_t i = 0; i < numScoreComps; ++i) {
        cout << " " << values[i];
    }
    cout << endl;
}

static void ShowWeights() {
    //TODO: Find a way of ensuring this order is synced with the nbest
    fix(cout, 6);
//    const StaticData& staticData = StaticData::Instance();
    const vector<const StatelessFeatureFunction*>& slf =
            StatelessFeatureFunction::GetStatelessFeatureFunctions();
    const vector<const StatefulFeatureFunction*>& sff =
            StatefulFeatureFunction::GetStatefulFeatureFunctions();

    for (size_t i = 0; i < sff.size(); ++i) {
        const StatefulFeatureFunction *ff = sff[i];
        if (ff->IsTuneable()) {
            PrintFeatureWeight(ff);
        }
    }
    for (size_t i = 0; i < slf.size(); ++i) {
        const StatelessFeatureFunction *ff = slf[i];
        if (ff->IsTuneable()) {
            PrintFeatureWeight(ff);
        }
    }
}

static void ChangeFeatureWeight(const FeatureFunction* ff) {
    size_t numScoreComps = ff->GetNumScoreComponents();

     vector<float> values;
    for (size_t i = 0; i < numScoreComps; ++i) {
        values.push_back((1.));
    }

    StaticData::InstanceNonConst().SetWeights(ff,values);
}

static void ChangeWeights() {
    //TODO: Find a way of ensuring this order is synced with the nbest
    fix(cout, 6);
//    const StaticData& staticData = StaticData::Instance();
    const vector<const StatelessFeatureFunction*>& slf =
            StatelessFeatureFunction::GetStatelessFeatureFunctions();
    const vector<const StatefulFeatureFunction*>& sff =
            StatefulFeatureFunction::GetStatefulFeatureFunctions();

    for (size_t i = 0; i < sff.size(); ++i) {
        const StatefulFeatureFunction *ff = sff[i];
        if (ff->IsTuneable()) {
            ChangeFeatureWeight(ff);
        }
    }
    for (size_t i = 0; i < slf.size(); ++i) {
        const StatelessFeatureFunction *ff = slf[i];
        if (ff->IsTuneable()) {
            ChangeFeatureWeight(ff);
        }
    }
}


size_t OutputFeatureWeightsForHypergraph(size_t index,
        const FeatureFunction* ff, std::ostream &outputSearchGraphStream);
void OutputFeatureWeightsForHypergraph(std::ostream &outputSearchGraphStream) ;

} //namespace




#endif // TRANSLATIONTASK_H
