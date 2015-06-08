#pragma once

#include "FeatureFunction.h"

namespace Moses
{

/** base class for all stateless feature functions.
 * eg. phrase table, word penalty, phrase penalty
 */
class StatelessFeatureFunction: public FeatureFunction
{
  //All stateless FFs, except those that cache scores in T-Option
  static std::vector<const StatelessFeatureFunction*> m_statelessFFs;

public:
  static const std::vector<const StatelessFeatureFunction*>& GetStatelessFeatureFunctions() {
    return m_statelessFFs;
  }

  StatelessFeatureFunction(const std::string &line);
  StatelessFeatureFunction(size_t numScoreComponents, const std::string &line);
  /**
    * This should be implemented for features that apply to phrase-based models.
    **/
  virtual void Evaluate(const Hypothesis& hypo,
                        ScoreComponentCollection* accumulator) const = 0;

  /**
    * Same for chart-based features.
    **/
  virtual void EvaluateChart(const ChartHypothesis &hypo,
                             ScoreComponentCollection* accumulator) const = 0;

  virtual bool IsStateless() const {
    return true;
  }

};


} // namespace

