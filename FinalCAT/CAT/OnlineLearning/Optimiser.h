/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

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
#ifndef _MIRA_OPTIMISER_H_
#define _MIRA_OPTIMISER_H_

#include <vector>

#include "moses/ScoreComponentCollection.h"
#include "SparseVec.h"

using namespace Moses;
using namespace std;
namespace Optimizer {
  
  class MiraOptimiser {
  public:
  MiraOptimiser() {
      m_slack = 0.001;
      m_scale_margin = false;
      m_scale_margin_precision = false;
      m_scale_update = false;
      m_scale_update_precision = false;
      m_boost = false;
      m_normaliseMargin = true;
      m_sigmoidParam = 1.;

  }

  MiraOptimiser(
	float slack, bool scale_margin, bool scale_margin_precision,
	bool scale_update, bool scale_update_precision, bool boost, bool normaliseMargin, float sigmoidParam, bool onlyOnlineScoreProducerUpdate) :
      m_slack(slack),
      m_scale_margin(scale_margin),
      m_scale_margin_precision(scale_margin_precision),
      m_scale_update(scale_update),
      m_scale_update_precision(scale_update_precision),
      m_precision(1),
      m_boost(boost),
      m_normaliseMargin(normaliseMargin),
      m_sigmoidParam(sigmoidParam),
      m_onlyOnlineScoreProducerUpdate(onlyOnlineScoreProducerUpdate) { }

      size_t updateWeights(
              ScoreComponentCollection& weightUpdate,
              const vector<vector<ScoreComponentCollection> >& featureValues,
              const vector<vector<float> >& losses,
              const vector<vector<float> >& bleuScores,
              const vector<vector<float> >& modelScores,
              const vector<ScoreComponentCollection>& oracleFeatureValues,
              const vector<float> oracleBleuScores,
              const vector<float> oracleModelScores,
              float learning_rate,
              size_t rank,
              size_t epoch);

     void setSlack(float slack) {
       m_slack = slack;
     }
     
     void setPrecision(float precision) {
       m_precision = precision;
     }
     
  private:
     // regularise Hildreth updates
     float m_slack;
     
     // scale margin with BLEU score or precision
     bool m_scale_margin, m_scale_margin_precision;
     
     // scale update with oracle BLEU score or precision
     bool m_scale_update, m_scale_update_precision;
     
     // update only online score producer
     bool m_onlyOnlineScoreProducerUpdate;

     float m_precision;
     
     // boosting of updates on misranked candidates
     bool m_boost;
     
     // squash margin between 0 and 1 (or depending on m_sigmoidParam)
     bool m_normaliseMargin;
     
     // y=sigmoidParam is the axis that this sigmoid approaches
     float m_sigmoidParam ;
  };
}

#endif
