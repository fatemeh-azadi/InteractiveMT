#ifndef SEARCHGRAPH_H
#define SEARCHGRAPH_H
#include <string>
#include <vector>
#include "moses/Hypothesis.h"
#include "moses/Manager.h"

using namespace Moses;
using namespace std;

class SearchGraph{
public:
    virtual void createGraph(vector<SearchGraphNode> v) = 0;
    virtual string getPrefix(string prefix) = 0;
    virtual int getGraphSize() = 0;
    virtual int getTransitionsSize() = 0;

    //~SearchGraph();
};
#endif // SEARCHGRAPH_H
