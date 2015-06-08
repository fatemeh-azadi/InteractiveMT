/*
 * Distance.cpp
 *
 *  Created on: Jun 15, 2014
 *      Author: fatemeh
 */
#include "Distance.h"
#include <string>
#include <malloc.h>
#include <vector>

using namespace std;

//****************************
// Get minimum of three values
//****************************

int Distance::minimum (int a, int b, int c)
{

  int mi;
  mi = a;
  if (b < mi) {
    mi = b;
  }
  if (c < mi) {
    mi = c;
  }
  return mi;

}


//*****************************
// Compute Levenshtein distance
//*****************************

void Distance::LD (vector<std::string> s, vector<std::string> t, vector<vector<int> >& distanceMatrix)
{
int n; // length of s
int m; // length of t
int i; // iterates through s
int j; // iterates through t
std::string s_i; // ith character of s
std::string t_j; // jth character of t
int cost; // cost
int result; // result
int cell; // contents of target cell
int above; // contents of cell immediately above
int left; // contents of cell immediately to left
int diag; // contents of cell immediately above and to left

  // Step 1

  m = s.size();
  n = t.size();
  //if (n == 0) {
 //   return m;
 // }
  //if (m == 0) {
 //   return n;
  //}

  distanceMatrix.resize(m+1);
  for (int i = 0; i <=m; i++)
      distanceMatrix[i].resize(n+1);



  // Step 2

  for (i = 1; i <= n; i++) {
      distanceMatrix[0][i]=i;
  }

  for (i = 1; i <= m; i++) {
      distanceMatrix[i][0]=i;
   }

  distanceMatrix[0][0]=0;
  // Step 3

  for (i = 1; i <= m; i++) {

    s_i = s[i-1];

    // Step 4

    for (j = 1; j <= n; j++) {

      t_j = t[j-1];

      // Step 5

      if (s_i == t_j) {
          cost = 0;
      }
      else {
        cost = 1;
      }

      // Step 6

      above = distanceMatrix[i-1][j];
      left = distanceMatrix[i][j-1];
      diag =  distanceMatrix[i-1][j-1];
      cell = minimum (above + 1, left + 1, diag + cost);
      distanceMatrix[i][j]= cell;
    }
  }

  // Step 7

//  return distanceMatrix;

}

void Distance::LD_final (vector<std::string> s, vector<std::string> t, vector<vector<int> >& distanceMatrix)
{
int n; // length of s
int m; // length of t
int i; // iterates through s
int j; // iterates through t
std::string s_i; // ith character of s
std::string t_j; // jth character of t
int cost; // cost
int result; // result
int cell; // contents of target cell
int above; // contents of cell immediately above
int left; // contents of cell immediately to left
int diag; // contents of cell immediately above and to left

  // Step 1

  m = s.size();
  n = t.size();
  //if (n == 0) {
 //   return m;
 // }
  //if (m == 0) {
 //   return n;
  //}

  distanceMatrix.resize(m+1);
  for (int i = 0; i <=m; i++)
      distanceMatrix[i].resize(n+1);
  int last_word_length;


  // Step 2

  for (i = 1; i <= n; i++) {
      distanceMatrix[0][i]=i;
  }

  for (i = 1; i <= m; i++) {
      distanceMatrix[i][0]=i;
   }

  distanceMatrix[0][0]=0;
  // Step 3

  for (i = 1; i <= m; i++) {

    s_i = s[i-1];

    // Step 4

    for (j = 1; j <= n; j++) {

      t_j = t[j-1];

      // Step 5
if (i!=m)
{
      if (s_i == t_j) {
          cost = 0;
      }
      else {
        cost = 1;
      }
}
else
{
    cost=0;
    const char * s_word= s_i.c_str();
    const char * t_word= t_j.c_str();
    last_word_length=s_i.size();
    for(int k=0;k<last_word_length;k++)
    {
        if(s_word[k]!=t_word[k])
        {
            cost=1;
            break;
        }
    }
}

      // Step 6

      above = distanceMatrix[i-1][j];
      left = distanceMatrix[i][j-1];
      diag =  distanceMatrix[i-1][j-1];
      cell = minimum (above + 1, left + 1, diag + cost);
      distanceMatrix[i][j]= cell;

    }
  }


  // Step 7

  //return last_word_length;

}





