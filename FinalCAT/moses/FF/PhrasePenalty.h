#pragma once

#include "StatelessFeatureFunction.h"

namespace Moses
{

class PhrasePenalty : public StatelessFeatureFunction
{
public:
  PhrasePenalty(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , ScoreComponentCollection &scoreBreakdown
                        , ScoreComponentCollection &estimatedFutureScore) const;

  void Evaluate(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const
  {}
  void EvaluateChart(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const
  {}

  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const
  {}

};

} //namespace

