#ifndef TERSHIFT_H
#define TERSHIFT_H

class TERShift
{
public:
    TERShift();
    int start, end, moveto;
    TERShift(int s, int e, int m){
        start = s;
        end = e;
        moveto = m;
    }
};

#endif // TERSHIFT_H
