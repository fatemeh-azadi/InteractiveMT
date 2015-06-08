#pragma once

#include "InputFeature.h"
#include "StatelessFeatureFunction.h"

namespace Moses
{


class InputFeature : public StatelessFeatureFunction
{
protected:
  static InputFeature *s_instance;

  size_t m_numInputScores;
  size_t m_numRealWordCount;
  bool m_legacy;

public:
  static const InputFeature& Instance() {
    return *s_instance;
  }
  static InputFeature& InstanceNonConst() {
    return *s_instance;
  }

  InputFeature(const std::string &line);

  void Load();

  void SetParameter(const std::string& key, const std::string& value);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  size_t GetNumInputScores() const {
    return m_numInputScores;
  }
  size_t GetNumRealWordsInInput() const {
    return m_numRealWordCount;
  }

  void Evaluate(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const
  {}
  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const;

  void Evaluate(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const
  {}
  void EvaluateChart(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const
  {}


};


}

