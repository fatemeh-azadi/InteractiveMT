
#pragma once

#include "PhraseDictionary.h"
#include <boost/thread/tss.hpp>

namespace Moses
{
class ChartParser;
class ChartCellCollectionBase;
class ChartRuleLookupManager;
class InputPath;

class PhraseDictionaryTransliteration : public PhraseDictionary
{
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryTransliteration&);

public:
  PhraseDictionaryTransliteration(const std::string &line);

  void Load();

  virtual void CleanUpAfterSentenceProcessing(const InputType& source);

  // for phrase-based model
  void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  // for syntax/hiero model (CKY+ decoding)
  ChartRuleLookupManager* CreateRuleLookupManager(const ChartParser&, const ChartCellCollectionBase&);

  void SetParameter(const std::string& key, const std::string& value);

  TO_STRING();

protected:
  std::string m_mosesDir, m_scriptDir, m_externalDir, m_inputLang, m_outputLang;

  std::vector<TargetPhrase*> CreateTargetPhrases(const Phrase &sourcePhrase, const std::string &outDir) const;
  bool SatisfyBackoff(const InputPath &inputPath) const;

  void GetTargetPhraseCollection(InputPath &inputPath) const;

};

}  // namespace Moses
