#include "HexGrid.h"
#include "BezCurve.h"
#include <utility>
#include <iostream>
#include <fstream>
#include <math.h>
#include <limits>

using namespace std;
using morph::BezCoord;
using morph::BezCurve;
using morph::HexGrid;

int main()
{
    int rtn = -1;

    pair<float,float> v1 = make_pair (-0.28f, 0.0f);
    pair<float,float> v2 = make_pair (0.28f, 0.0f);
    pair<float,float> v3 = make_pair (0.28f, 0.45f);
    pair<float,float> v4 = make_pair (-0.28f, 0.45f);
    vector<pair<float,float>> c;
    c.push_back (v1);
    c.push_back (v2);
    c.push_back (v3);
    c.push_back (v4);

    BezCurve cv (c);

    BezCoord bm = cv.computePointMatrix (0.4);
    BezCoord bg = cv.computePointGeneral (0.4);
    cout << "matrix method: " << bm << endl;
    cout << "general method: " << bg << endl;

    float xdiff = bm.x() - bg.x();
    float ydiff = bm.y() - bg.y();
    cout << "x points differ by: " << xdiff << endl;
    cout << "y points differ by: " << ydiff << endl;

    if (xdiff < numeric_limits<float>::epsilon() && ydiff < numeric_limits<float>::epsilon()) {
        cout << "General & matrix methods compute same point" << endl;
        rtn = 0;
    }

    return rtn;
}
