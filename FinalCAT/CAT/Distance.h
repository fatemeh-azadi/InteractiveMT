/*
 * Distance.h
 *
 *  Created on: Jun 15, 2014
 *      Author: fatemeh
 */

#ifndef DISTANCE_H_
#define DISTANCE_H_

#include <string>
#include <vector>
using namespace std;
class Distance
{
  public:
    void LD (vector<std::string> s, vector<std::string> t, vector<vector<int> >& distanceMatrix);
    void LD_final (vector<std::string> s, vector<std::string> t, vector<vector<int> >& distanceMatrix);
    int minimum (int a, int b, int c);

};


#endif /* DISTANCE_H_ */
