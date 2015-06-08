
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

#include "TranslationTask.h"
#include "OnlineLearner.h"
namespace CATServer {

// output floats with three significant digits

/** Enforce rounding */
void fix(std::ostream& stream, size_t size) {
    stream.setf(std::ios::fixed);
    stream.precision(size);
}

/** Translates a sentence.
 * - calls the search (Manager)
 * - applies the decision rule
 * - outputs best translation and additional reporting
 **/


      TranslationTask::TranslationTask(size_t lineNumber,
                      InputType* source, OutputCollector* outputCollector, OutputCollector* nbestCollector,
                                       OutputCollector* latticeSamplesCollector,
                                       OutputCollector* wordGraphCollector, OutputCollector* searchGraphCollector,
                                       OutputCollector* detailedTranslationCollector,
                                       OutputCollector* alignmentInfoCollector,
                                       OutputCollector* unknownsCollector,
                                       bool outputSearchGraphSLF,
                                       bool outputSearchGraphHypergraph,
                                       SearchGraph *sg, SearchNBest *nbestsearch, Manager *m_manager) :
                         m_source(source), m_lineNumber(lineNumber),
                         m_outputCollector(outputCollector), m_nbestCollector(nbestCollector),
                         m_latticeSamplesCollector(latticeSamplesCollector),
                         m_wordGraphCollector(wordGraphCollector), m_searchGraphCollector(searchGraphCollector),
                         m_detailedTranslationCollector(detailedTranslationCollector),
                         m_alignmentInfoCollector(alignmentInfoCollector),
                         m_unknownsCollector(unknownsCollector),
                         m_outputSearchGraphSLF(outputSearchGraphSLF),
                         m_outputSearchGraphHypergraph(outputSearchGraphHypergraph),
                         m_searchGraph(sg),
                         m_nbestSearch(nbestsearch),
                         m_Manager(m_manager){}



    /** Translate one sentence
     * gets called by main function implemented at end of this source file */
    void TranslationTask::Run() {

        // report thread number

        Timer translationTime;
        translationTime.start();
        // shorthand for "global data"
        const StaticData &staticData = StaticData::Instance();
        // input sentence
        Sentence sentence;

        // execute the translation
        // note: this executes the search, resulting in a search graph
        //       we still need to apply the decision rule (MAP, MBR, ...)
        //Manager manager(m_lineNumber, *m_source,
         //       staticData.GetSearchAlgorithm());

        m_Manager->ProcessSentence();

        targetSentence = m_Manager->GetBestHypothesis()->GetOutputString();


        // output search graph
        if (m_searchGraphCollector) {
          ostringstream out;
          fix(out,PRECISION);
          m_Manager->OutputSearchGraph(m_lineNumber, out);
          vector<SearchGraphNode> vec;
          m_Manager->GetSearchGraph(vec);

          if(m_searchGraph != NULL)
             m_searchGraph->createGraph(vec);
        //  m_searchGraphCollector->Write(m_lineNumber, out.str());

    #ifdef HAVE_PROTOBUF
          if (staticData.GetOutputSearchGraphPB()) {
            ostringstream sfn;
            sfn << staticData.GetParam("output-search-graph-pb")[0] << '/' << m_lineNumber << ".pb" << ends;
            string fn = sfn.str();
            VERBOSE(2, "Writing search graph to " << fn << endl);
            fstream output(fn.c_str(), ios::trunc | ios::binary | ios::out);
            m_Manager->SerializeSearchGraphPB(m_lineNumber, output);
          }
    #endif
        }

        // apply decision rule and output best translation(s)
        if (m_outputCollector) {
            ostringstream out;
            ostringstream debug;
            fix(debug, PRECISION);

            // all derivations - send them to debug stream
            if (staticData.PrintAllDerivations()) {
                m_Manager->PrintAllDerivations(m_lineNumber, debug);
            }

            // MAP decoding: best hypothesis
            const Hypothesis* bestHypo = NULL;
            if (!staticData.UseMBR()) {
                bestHypo = m_Manager->GetBestHypothesis();
                if (bestHypo) {
                    if (staticData.IsPathRecoveryEnabled()) {
                        OutputInput(out, bestHypo);
                        out << "||| ";
                    }
                    if (staticData.GetParam("print-id").size()
                            && Scan<bool>(staticData.GetParam("print-id")[0])) {
                        out << m_source->GetTranslationId() << " ";
                    }

                    if (staticData.GetReportSegmentation() == 2) {
                        m_Manager->GetOutputLanguageModelOrder(out, bestHypo);
                    }
                    OutputBestSurface(out, bestHypo,
                            staticData.GetOutputFactorOrder(),
                            staticData.GetReportSegmentation(),
                            staticData.GetReportAllFactors());
                    if (staticData.PrintAlignmentInfo()) {
                        out << "||| ";
                        OutputAlignment(out, bestHypo);
                    }

                    OutputAlignment(m_alignmentInfoCollector, m_lineNumber,
                            bestHypo);
                    IFVERBOSE(1) {
                        debug << "BEST TRANSLATION: " << *bestHypo << endl;
                    }
                } else {
                    VERBOSE(1, "NO BEST TRANSLATION" << endl);
                }

                out << endl;
            }


            // report best translation to output collector
            m_outputCollector->Write(m_lineNumber, out.str(), debug.str());
        }

        // output n-best list
        if (m_nbestCollector && !staticData.UseLatticeMBR()) {
            TrellisPathList nBestList;
            ostringstream out;
            m_Manager->CalcNBest(staticData.GetNBestSize(), nBestList,
                    staticData.GetDistinctNBest());
            OutputNBest(out, nBestList, staticData.GetOutputFactorOrder(),
                    m_lineNumber, staticData.GetReportSegmentation());
            m_nbestSearch->setNbests(out.str());
            m_nbestCollector->Write(m_lineNumber, out.str());
        }



        // report additional statistics
        IFVERBOSE(2) {
            PrintUserTime("Sentence Decoding Time:");
        }
        m_Manager->CalcDecoderStatistics();



        VERBOSE(1,
                "Line " << m_lineNumber << ": Translation took " << translationTime << " seconds total" << endl);


    }

//    string searchPrefix(string prefix){
//        return searchGraph->searchPrefix(prefix);
//    }

    string TranslationTask::getTargetSentence(){
        return targetSentence;
    }



    TranslationTask::~TranslationTask() {
        delete m_source;
    }






size_t OutputFeatureWeightsForHypergraph(size_t index,
        const FeatureFunction* ff, std::ostream &outputSearchGraphStream) {
    size_t numScoreComps = ff->GetNumScoreComponents();
    if (numScoreComps != 0) {
        vector<float> values =
                StaticData::Instance().GetAllWeights().GetScoresForProducer(ff);
        if (numScoreComps > 1) {
            for (size_t i = 0; i < numScoreComps; ++i) {
                outputSearchGraphStream << ff->GetScoreProducerDescription()
                        << i << "=" << values[i] << endl;
            }
        } else {
            outputSearchGraphStream << ff->GetScoreProducerDescription() << "="
                    << values[0] << endl;
        }
        return index + numScoreComps;
    } else {
        cerr
                << "Sparse features are not yet supported when outputting hypergraph format"
                << endl;
        assert(false);
        return 0;
    }
}

void OutputFeatureWeightsForHypergraph(std::ostream &outputSearchGraphStream) {
    outputSearchGraphStream.setf(std::ios::fixed);
    outputSearchGraphStream.precision(6);

    const StaticData& staticData = StaticData::Instance();
    const vector<const StatelessFeatureFunction*>& slf =
            StatelessFeatureFunction::GetStatelessFeatureFunctions();
    const vector<const StatefulFeatureFunction*>& sff =
            StatefulFeatureFunction::GetStatefulFeatureFunctions();
    size_t featureIndex = 1;
    for (size_t i = 0; i < sff.size(); ++i) {
        featureIndex = OutputFeatureWeightsForHypergraph(featureIndex, sff[i],
                outputSearchGraphStream);
    }
    for (size_t i = 0; i < slf.size(); ++i) {
        /*
         if (slf[i]->GetScoreProducerWeightShortName() != "u" &&
         slf[i]->GetScoreProducerWeightShortName() != "tm" &&
         slf[i]->GetScoreProducerWeightShortName() != "I" &&
         slf[i]->GetScoreProducerWeightShortName() != "g")
         */
        {
            featureIndex = OutputFeatureWeightsForHypergraph(featureIndex,
                    slf[i], outputSearchGraphStream);
        }
    }
    const vector<PhraseDictionary*>& pds = PhraseDictionary::GetColl();
    for (size_t i = 0; i < pds.size(); i++) {
        featureIndex = OutputFeatureWeightsForHypergraph(featureIndex, pds[i],
                outputSearchGraphStream);
    }
    const vector<GenerationDictionary*>& gds = GenerationDictionary::GetColl();
    for (size_t i = 0; i < gds.size(); i++) {
        featureIndex = OutputFeatureWeightsForHypergraph(featureIndex, gds[i],
                outputSearchGraphStream);
    }

}


/*
void OutputFeatureWeights()
{
 // UTIL_THROW(util::Exception, "completely redo. Too many hardcoded ff"); // TODO completely redo. Too many hardcoded ff
  const StaticData& staticData = StaticData::Instance();

  const vector<const StatelessFeatureFunction*>& slf =
          StatelessFeatureFunction::GetStatelessFeatureFunctions();
  const vector<const StatefulFeatureFunction*>& sff =
          StatefulFeatureFunction::GetStatefulFeatureFunctions();
  size_t featureIndex = 1;
  for (size_t i = 0; i < sff.size(); ++i) {
      PrintFeatureWeight(sff[i]);

  }

  size_t numScoreComps = ff->GetNumScoreComponents();
  if (numScoreComps != 0) {
      vector<float> values =
              StaticData::Instance().GetAllWeights().GetScoresForProducer(ff);
      if (numScoreComps > 1) {
          for (size_t i = 0; i < numScoreComps; ++i) {
              cout << ff->GetScoreProducerDescription()
                      << i << "=" << values[i] << endl;
          }
      } else {
          cout << ff->GetScoreProducerDescription() << "="
                  << values[0] << endl;
      }
*/
  // check whether "weight-u" is already set
  /*if (m_parameter->isParamShortNameSpecified("u")) {
    if (m_parameter->GetParamShortName("u").size() < 1 ) {
      PARAM_VEC w(1,"1.0");
      m_parameter->OverwriteParamShortName("u", w);
    }
  }*/

      /*
  //loop over all ScoreProducer to update weights

  std::vector<const ScoreProducer*>::const_iterator iterSP;
  for (iterSP = transSystem.GetFeatureFunctions().begin() ; iterSP != transSystem.GetFeatureFunctions().end() ; ++iterSP) {
    std::string paramShortName = (*iterSP)->GetScoreProducerWeightShortName();
    vector<float> Weights = Scan<float>(m_parameter->GetParamShortName(paramShortName));

    if (paramShortName == "d") { //basic distortion model takes the first weight
      if ((*iterSP)->GetScoreProducerDescription() == "Distortion") {
        Weights.resize(1); //take only the first element
      } else { //lexicalized reordering model takes the other
        Weights.erase(Weights.begin()); //remove the first element
      }
      //			std::cerr << "this is the Distortion Score Producer -> " << (*iterSP)->GetScoreProducerDescription() << std::cerr;
      //			std::cerr << "this is the Distortion Score Producer; it has " << (*iterSP)->GetNumScoreComponents() << " weights"<< std::cerr;
      //  	std::cerr << Weights << std::endl;
    } else if (paramShortName == "tm") {
      continue;
    }
    SetWeights(*iterSP, Weights);
  }

  //	std::cerr << "There are " << m_phraseDictionary.size() << " m_phraseDictionaryfeatures" << std::endl;

  const vector<float> WeightsTM = Scan<float>(m_parameter->GetParamShortName("tm"));
  //  std::cerr << "WeightsTM: " << WeightsTM << std::endl;

  const vector<float> WeightsLM = Scan<float>(m_parameter->GetParamShortName("lm"));
  //  std::cerr << "WeightsLM: " << WeightsLM << std::endl;

  size_t index_WeightTM = 0;
  for(size_t i=0; i<transSystem.GetPhraseDictionaries().size(); ++i) {
    PhraseDictionaryFeature &phraseDictionaryFeature = *m_phraseDictionary[i];

    //		std::cerr << "phraseDictionaryFeature.GetNumScoreComponents():" << phraseDictionaryFeature.GetNumScoreComponents() << std::endl;
    //		std::cerr << "phraseDictionaryFeature.GetNumInputScores():" << phraseDictionaryFeature.GetNumInputScores() << std::endl;

    vector<float> tmp_weights;
    for(size_t j=0; j<phraseDictionaryFeature.GetNumScoreComponents(); ++j)
      tmp_weights.push_back(WeightsTM[index_WeightTM++]);

    //  std::cerr << tmp_weights << std::endl;

    SetWeights(&phraseDictionaryFeature, tmp_weights);
  }


}*/

} //namespace


