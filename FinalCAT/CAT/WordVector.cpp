#include "WordVector.h"

//using namespace CaitraWordVec;
using namespace std;

namespace CaitraWordVector{
WordVector::WordVector()
{

}

void WordVector::readWordVec(std::string vectorsFile)
{
    FILE *f;
    long long a, b;
    float len;
    f = fopen(vectorsFile.c_str(), "rb");
    if (f == NULL) {
        printf("Input file not found\n");
        exit(EXIT_FAILURE);
    }

    fscanf(f, "%lld", &words);
    fscanf(f, "%lld", &size);
    vocab = (char *)malloc((long long)words * max_w * sizeof(char));

    M = (float *)malloc((long long)words * (long long)size * sizeof(float));
    if (M == NULL) {
        printf("Cannot allocate memory: %lld MB    %lld  %lld\n", (long long)words * size * sizeof(float) / 1048576, words, size);
        exit(EXIT_FAILURE);
    }

    for (b = 0; b < words; b++) {
        a = 0;
        while (1) {
            vocab[b * max_w + a] = fgetc(f);
            if (feof(f) || (vocab[b * max_w + a] == ' ')) break;
            if ((a < max_w) && (vocab[b * max_w + a] != '\n')) a++;
        }
        vocab[b * max_w + a] = 0;
        for (a = 0; a < size; a++) fread(&M[a + b * size], sizeof(float), 1, f);
        len = 0;
        for (a = 0; a < size; a++) len += M[a + b * size] * M[a + b * size];
        len = sqrt(len);
        for (a = 0; a < size; a++) M[a + b * size] /= len;
    }
    fclose(f);
    char *tmp = (char *)malloc(max_size * sizeof(char));


    for (b = 0; b < words; b++) {
        //cout << b << endl;
        strcpy(tmp, &vocab[b * max_w]);
        //findVector(string(tmp));
        dic[tmp] = b;
    }


}

vector<int> WordVector::getVocabStartsWith( string str){
    vector<int> res;
   string str2 = lowercase(str);
     char *tmp = (char *)malloc(max_size * sizeof(char));
    for (long long b = 0; b < words; b++){
        strcpy(tmp, &vocab[b * max_w]);
        char * pt = strstr(tmp,str.c_str());
        if(pt != NULL){
            if(pt - tmp == 0)
                res.push_back(b);
            else{
                pt = strstr(tmp, str2.c_str());
                if(pt != NULL && (pt - tmp == 0))
                    res.push_back(b);
            }
        }
    }
    return res;
}

void WordVector::getVector(string s, vector<string> &wordVec, vector<float> &distVec){
    wordVec.clear();
    distVec.clear();
    char *bestw[N];
    float dist, bestd[N], vec[max_size],len;
    long long a, b, c, d, bi;

    for (a = 0; a < N; a++) bestw[a] = (char *)malloc(max_size * sizeof(char));

    for (a = 0; a < N; a++) bestd[a] = 0;
    for (a = 0; a < N; a++) bestw[a][0] = 0;

    for (b = 0; b < words; b++) if (!strcmp(&vocab[b * max_w], s.c_str())) break;
    if (b == words) b = -1;
    bi = b;


    if (bi == -1)
        return;

    for (a = 0; a < size; a++) vec[a] = 0;
    for (a = 0; a < size; a++) vec[a] += M[a + bi * size];

    len = 0;
    for (a = 0; a < size; a++) len += vec[a] * vec[a];
    len = sqrt(len);
    //for (a = 0; a < size; a++) vec[a] /= len;
    for (a = 0; a < N; a++) bestd[a] = -1;
    for (a = 0; a < N; a++) bestw[a][0] = 0;

    for (c = 0; c < words; c++) {
        a = 0;
        if (bi == c) continue;
        dist = 0;
        for (a = 0; a < size; a++) dist += vec[a] * M[a + c * size];
        for (a = 0; a < N; a++) {
            if (dist > bestd[a]) {
                for (d = N - 1; d > a; d--) {
                    bestd[d] = bestd[d - 1];
                    strcpy(bestw[d], bestw[d - 1]);
                }
                bestd[a] = dist;
                strcpy(bestw[a], &vocab[c * max_w]);
                break;
            }
        }
    }

    for (a = 0; a < N; a++)
        if(bestd[a] != -1){
            wordVec.push_back(bestw[a]);
            distVec.push_back(bestd[a]);
        }
    for (a = 0; a < N; a++) free(bestw[a]);


    return;
}

void WordVector::findVector(string s){
    //s = lowercase(s);
    cosineDistance[s].clear();
    char *bestw;
    float dist, vec[max_size];
    long long a, b, c, bi;

    bestw = (char *)malloc(max_size * sizeof(char));

    for (b = 0; b < words; b++) if (!strcmp(&vocab[b * max_w], s.c_str())) break;
    if (b == words) b = -1;
    bi = b;


    cosineDistance[s][s] = 1.;
    if (bi == -1)
        return;

    for (a = 0; a < size; a++) vec[a] = 0;
    for (a = 0; a < size; a++) vec[a] += M[a + bi * size];

    bestw[0] = 0;
    for (c = 0; c < words; c++) {
        a = 0;
        if (bi == c) continue;
        dist = 0;
        for (a = 0; a < size; a++) dist += vec[a] * M[a + c * size];

        if(dist >= 0.4){
            strcpy(bestw, &vocab[c * max_w]);
            cosineDistance[s][string(bestw)] = dist;
        }
    }
    free(bestw);
    return;
}

float WordVector::getDistance(std::string s1, std::string s2){
  //  s1 = lowercase(s1);
  //  s2 = lowercase(s2);
    if(dic.find(s1) == dic.end() || dic.find(s2) == dic.end())
        return 0;
    long long x = dic[s1], y = dic[s2];
    if(x == y)
        return 1;
    float dist, vec[max_size], len;
    long long a, b;
    len = 0;
    for (a = 0; a < size; a++){  vec[a] = M[a + x * size];
        len += vec[a] * vec[a];
    }
    len = sqrt(len);
    dist = 0;
    for (a = 0; a < size; a++) dist += (vec[a] * M[a + y * size]/len);
    return dist;

    /*
    map<string, float> mp;
    if(cosineDistance.find(s1) == cosineDistance.end())
        return 0.;
    mp = cosineDistance[s1];
    if(mp.find(s2) == mp.end())
        return 0.;
    return mp[s2];
    */

}


float WordVector::getDistance(long long x, long long y){

    if(x == -1 || y == -1)
        return 0;
    if(x == y)
        return 1;
    float dist, vec[max_size], len = 0;
    long long a;

    for (a = 0; a < size; a++){  vec[a] = M[a + x * size];
        len += vec[a] * vec[a];
    }
    len = sqrt(len);
    dist = 0;
    for (a = 0; a < size; a++) dist += (vec[a] * M[a + y * size]/len);
    return dist;
}


long long WordVector::getIndex(std::string s){
    s = lowercase(s);
    if(dic.find(s) == dic.end())
        return -1;
    return dic[s];
}

string WordVector::lowercase(string mixedcase) {
    transform(mixedcase.begin(), mixedcase.end(), mixedcase.begin(), ::tolower);
    return mixedcase;
}

}
/*
int main(int argc, char** argv) {

    WordVector wv("/home/fatemeh/Desktop/IMT/word2vec/trunk/vectors.bin");
    vector<string> wordVec;
    vector<float> distVec;
    wv.findVector("hannover");

    for(int i = 0; i < wordVec.size(); i++){
        cout << wordVec[i] << "    " << distVec[i] << endl;
    }

    wv.getVector("good", wordVec, distVec);
    return 0;
}
*/
