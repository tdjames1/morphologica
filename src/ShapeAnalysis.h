#ifndef _SHAPEANALYSIS_H_
#define _SHAPEANALYSIS_H_

#include <vector>
#include <list>
#include <set>
#include <limits>
#include <stdexcept>
#include "Hex.h"
#include "HexGrid.h"
#include "DirichVtx.h"
#include "MorphDbg.h"

using std::vector;
using std::list;
using std::set;
using std::numeric_limits;
using std::runtime_error;
using std::exception;

using morph::Hex;
using morph::HexGrid;
using morph::DirichVtx;

namespace morph {

    /*!
     * Rotational direction Rotn::Clock or Rotn::Anticlock
     */
    enum class Rotn {
        Unknown,
        Clock,
        Anticlock
    };

    /*!
     * A helper class, containing pattern analysis code to analyse patterns within HexGrids.
     */
    template <class Flt>
    class ShapeAnalysis
    {
    public:
        /*!
         * Obtain the contours (as a vector of list<Hex>) in the scalar fields f, where threshold is
         * crossed.
         */
        static vector<list<Hex> > get_contours (HexGrid* hg,
                                                vector<vector<Flt> >& f,
                                                Flt threshold) {

            unsigned int nhex = hg->num();
            unsigned int N = f.size();

            vector<list<Hex> > rtn;
            // Initialise
            for (unsigned int li = 0; li < N; ++li) {
                list<Hex> lh;
                rtn.push_back (lh);
            }

            Flt maxf = -1e7;
            Flt minf = +1e7;
            for (auto h : hg->hexen) {
                if (h.onBoundary() == false) {
                    for (unsigned int i = 0; i<N; ++i) {
                        if (f[i][h.vi] > maxf) { maxf = f[i][h.vi]; }
                        if (f[i][h.vi] < minf) { minf = f[i][h.vi]; }
                    }
                }
            }
            Flt scalef = 1.0 / (maxf-minf);

            // Re-normalize
            vector<vector<Flt> > norm_f;
            norm_f.resize (N);
            for (unsigned int i=0; i<N; ++i) {
                norm_f[i].resize (nhex, 0.0);
            }

            for (unsigned int i = 0; i<N; ++i) {
                for (unsigned int h=0; h<nhex; h++) {
                    norm_f[i][h] = (f[i][h] - minf) * scalef;
                }
            }

            // Collate
            for (unsigned int i = 0; i<N; ++i) {
                for (auto h : hg->hexen) {
                    if (h.onBoundary() == false) {
                        if (norm_f[i][h.vi] > threshold) {
                            if ( (h.has_ne && norm_f[i][h.ne->vi] < threshold)
                                 || (h.has_nne && norm_f[i][h.nne->vi] < threshold)
                                 || (h.has_nnw && norm_f[i][h.nnw->vi] < threshold)
                                 || (h.has_nw && norm_f[i][h.nw->vi] < threshold)
                                 || (h.has_nsw && norm_f[i][h.nsw->vi] < threshold)
                                 || (h.has_nse && norm_f[i][h.nse->vi] < threshold) ) {
                                rtn[i].push_back (h);
                            }
                        }
                    } else { // h.onBoundary() is true
                        if (norm_f[i][h.vi] > threshold) {
                            rtn[i].push_back (h);
                        }
                    }
                }
            }

            return rtn;
        }

        /*!
         * Take a set of variables, @f, for the given HexGrid @hg. Return a vector of Flts (again,
         * based on the HexGrid @hg) which marks each hex with the outer index of the @f which has
         * highest value in that hex, scaled and converted to a float.
         */
        static vector<Flt>
        dirichlet_regions (HexGrid* hg, vector<vector<Flt> >& f) {
            unsigned int N = f.size();

            // Single variable to return
            vector<Flt> rtn (f[0].size(), 0.0);

            // Mark regions first.
            for (auto h : hg->hexen) {

                Flt maxf = -1e7;
                for (unsigned int i = 0; i<N; ++i) {
                    if (f[i][h.vi] > maxf) {
                        maxf = f[i][h.vi];
                        Flt fi = 0.0f;
                        fi = (Flt)i;
                        rtn[h.vi] = (fi/N);
                    }
                }
            }

            return rtn;
        }

        /*!
         * A method to test the hex give by @h, which must live on the HexGrid pointed to by @hg, to
         * see if it is a Dirichlet vertex. If so, a vertex should be created in @vertices.
         */
        static void
        vertex_test (HexGrid* hg, vector<Flt>& f,
                     list<Hex>::iterator h, list<DirichVtx<Flt> >& vertices) {

            // For each hex, examine its neighbours, counting number of different neighbours.
            set<Flt> n_ids;
            n_ids.insert (f[h->vi]);
            for (unsigned int ni = 0; ni < 6; ++ni) {
                if (h->has_neighbour(ni)) {
                    n_ids.insert (f[h->get_neighbour(ni)->vi]);
                }
            }

            // n_ids is WRONG. Need to test for n_ids that are adjoining each vertex. This number
            // will be 1, 2 or 3.
            DBG2 (h->outputRG() << ": boundaryHex:" << h->boundaryHex << ", n_ids.size():" << n_ids.size());

            if ((h->boundaryHex == true && n_ids.size() >= 2)
                || (h->boundaryHex == false && n_ids.size() >= 3)) {
                // Then there's the possibility of a vertex on this hex.

                if (h->boundaryHex == true) { // 1. Test for boundary vertices
                    // Here, I need to set a vertex where two hexes join and we're on the
                    // boundary. This provides information to set the angles to discover the best
                    // center for each domain (see Honda 1983).
                    for (int ni = 0; ni < 6; ++ni) { // ni==0 is neighbour E. 1 is neighbour NE, etc.

                        // If there's a neighbour in direction ni and it has a different ID:
                        if (h->has_neighbour(ni) && f[h->get_neighbour(ni)->vi] != f[h->vi]) {

                            // Examine which direction DOESN'T have a neighbour and that will
                            // determine which hex vertex is the domain vertex.

                            // The first non-identical ID
                            int nii = (ni+1)%6;
                            if (!h->has_neighbour(nii)) {
                                // Then vertex direction is "vertex direction ni"
                                DBG2 ("vtx bh 1");
                                DirichVtx<Flt> a(
                                    h->get_vertex_coord(ni),
                                    hg->getd(),
                                    f[h->vi],
                                    make_pair(-1.0f, f[h->get_neighbour(ni)->vi]),
                                    h);
                                a.onBoundary = true;
                                vertices.push_back (a);

                            } else {
                                nii = ni>0?ni-1:5;
                                if (!h->has_neighbour(nii)) {
                                    // Then vertex dirn is "vertex direction (ni-1) or 5", i.e. nii.
                                    DBG2 ("vtx bh 2");
                                    DirichVtx<Flt> a (
                                        h->get_vertex_coord(nii),
                                        hg->getd(),
                                        f[h->vi],
                                        make_pair(f[h->get_neighbour(ni)->vi], -1.0f),
                                        h);
                                    a.onBoundary = true;
                                    vertices.push_back (a);
                                }
                            }
                        }
                    }
                } // end boundaryHex specific extra test.

                // 2. Test for internal vertices with >2 (i.e. 3) different types in self &
                // neighbouring hexes, so now work out which of the Hex's vertices is the vertex of
                // the domain.
                for (int ni = 0; ni < 6; ++ni) { // ni==0 is neighbour east. 1 is neighbour NE, etc.

                    // If there's a neighbour in direction ni and that neighbour has different ID:
                    if (h->has_neighbour(ni) && f[h->get_neighbour(ni)->vi] != f[h->vi]) {

                        // The first non-identical ID
                        Flt f1 = f[h->get_neighbour(ni)->vi];
                        int nii = (ni+1)%6;

                        // Test all six vertices, no breaking.
                        if (h->has_neighbour(nii)
                            && f[h->get_neighbour(nii)->vi] != f[h->vi]
                            && f[h->get_neighbour(nii)->vi] != f1 // f1 already tested != f[h->vi]
                            ) {
                            // Then vertex is "vertex ni"
                            DBG2 ("vtx ih 1");
                            vertices.push_back (
                                DirichVtx<Flt>(
                                    h->get_vertex_coord(ni),
                                    hg->getd(),
                                    f[h->vi],
                                    make_pair(f[h->get_neighbour(nii)->vi], f[h->get_neighbour(ni)->vi]),
                                    h)
                                );
                        }
                    }
                }
            }
        }

#define DEBUG_WALK 1
#ifdef DEBUG_WALK
# define WALK(s)  DBGSTREAM << "WLK: " << __FUNCTION__ << ": " << s << std::endl;
#else
# define WALK(s)
#endif
//#define DEBUG_WALK2 1
#ifdef DEBUG_WALK2
# define WLK2(s)  DBGSTREAM << "WLK: " << __FUNCTION__ << ": " << s << std::endl;
#else
# define WLK2(s)
#endif
        /*!
         * Walk an edge between two domains. Common code used by walk_to_neighbour and walk_to_next.
         *
         * @hg The HexGrid on which the action takes place
         *
         * @f The map of identities for the HexGrid @hg
         *
         * @v The starting Dirichlet vertex at which the edge starts
         *
         * @path A return variable into which the coordinates of the path will be written
         *
         * @edgedoms The identities of the two domains that the edge should trace a path between.
         *
         * @next_neighb_dom A variable in which to return the next neighbour domain ID that is
         * found. Could we not return the coordinate too?
         *
         * @next_neighb_coord The coordinate of the next hex along the edge. Useful when tracing
         * around domains. An alterative to last_coord.
         *
         * @last_coord A coordinate to AVOID. This information is required for the case that there
         * is ambiguity in the direction of travel along the path. This can occur when there are two
         * hexes ID f4 adjoining hexes f3 (inside domain) and f1 (outside). See as an example,
         * testDirichlet2.cpp, and the section from (0,-0.46188) to (0.1,-0.404145).
         */
        static pair<Flt, Flt>
        walk_common (HexGrid* hg,
                     vector<Flt>& f,
                     DirichVtx<Flt>& v,
                     list<pair<Flt, Flt>>& path,
                     pair<Flt, Flt>& edgedoms,
                     Flt& next_neighb_dom,
                     pair<Flt, Flt>& next_neighb_coord) {

            WALK ("*** Called. edgedoms.first: "
                  << edgedoms.first << ", edgedoms.second: " << edgedoms.second << " ***");

            // Really, we only have coordinates to return.
            pair<Flt, Flt> next_one = {numeric_limits<Flt>::max(), numeric_limits<Flt>::max()};

            // Used later
            int i = 0;
            int j = 0;

            // Walk the edge, with hexit pointing to the hexes on the edgedoms.first
            // side. _Initially_, point hexit at the hex that's on the inside of the domain for
            // which v is a Dirichlet vertex - v.hi. At least, this is what you do when walking OUT
            // to a neighbour vertex that's part of another domain.
            list<Hex>::iterator hexit = v.hi;
            // point hexit_neighb to the hexes on the edgedoms.second side
            list<Hex>::iterator hexit_neighb = v.hi;
            // The first hex, inside the domain.
            list<Hex>::iterator hexit_first = v.hi;
            // Temporary hex pointers
            list<Hex>::iterator hexit_next = v.hi;
            list<Hex>::iterator hexit_last = v.hi;

            // Set true when we find the partner vertex.
            bool partner_found = false;

            pair<Flt, Flt> v_init = v.v;

            // Test if the initial hex itself is on the side of the edge (as it will be when walking
            // from one domain vertex to the next domain vertex). This code is side-stepped when
            // walking OUT to a neighbour vertex that's part of another domain. This looks a bit
            // hacky, but I do have to find out which hex to start from when walking along the edge
            // and so some sort of code like this has to go somewhere.
            if (f[hexit_first->vi] == edgedoms.first) {
                WLK2 ("Hex AT " << hexit_first->outputRG() << " has f=" << f[hexit_first->vi]);
                // Then I need to find out which of the other hexes should swap to hexit_first.
                for (i = 0; i<6; ++i) {
                    if (hexit_first->has_neighbour(i)) {
                        // For each neighbour to hexit_first, check its centre is one long-radius
                        // from v.v. Only two will fulfil this criterion.
                        Flt x_ = hexit_first->get_neighbour(i)->x - v_init.first;
                        Flt y_ = hexit_first->get_neighbour(i)->y - v_init.second;
                        Flt distance = sqrt (x_*x_ + y_*y_);
                        DBG2 ("vertex to hex-centre distance: " << distance);
                        DBG2 ("HexGrid long radius distance: " << hexit_first->getLR());
                        bool correct_distance = distance-hexit_first->getLR() < 0.001 ? true : false;
                        if (correct_distance) {
                            if (f[hexit_first->get_neighbour(i)->vi] != edgedoms.second
                                && f[hexit_first->get_neighbour(i)->vi] != edgedoms.first) {
                                WLK2 ("Found the true hexit_first");
                                hexit = hexit_first;
                                hexit_first = hexit_first->get_neighbour(i);
                                break;
                            }
                        }
                    }
                }
            }

            // Now the main walking algorithm
            while (!partner_found) {

                WALK ("===== while loop. partner_found==false ======");

                // Find the initial direction of the edge and the hex containing edgedoms.first:
                int hexit_first_dirn = numeric_limits<int>::max();
                for (i = 0; i<6; ++i) {
                    DBG2 ("i=" << i << ", Comparing coordinates: ("
                          << hexit_first->get_vertex_coord(i).first << ","
                          << hexit_first->get_vertex_coord(i).second
                          << ") and v_init = (" << v_init.first << "," << v_init.second << ")");

                    if (hexit_first->compare_vertex_coord(i, v_init) == true) {

                        // Then the neighbours are either side of vertex direction i.
                        WALK ("initial *vertex* in direction " << i << "/"
                              << Hex::vertex_name(i) << " wrt current hexit_first: "
                              << hexit_first->outputRG());

                        if (hexit_first->has_neighbour ((i+1)%6)) {
                            WALK ("Hex adjoining " << hexit_first->outputRG()
                                  << " to " << Hex::neighbour_pos((i+1)%6)
                                  << " has f=" << f[hexit_first->get_neighbour ((i+1)%6)->vi] << ", "
                                  << hexit_first->get_neighbour ((i+1)%6)->outputRG());

                            // The next hex to be pointed to by hexit is the one with
                            // f==edgedoms.first
                            hexit = hexit_first->get_neighbour ((i+1)%6);

                            if (f[hexit->vi] == edgedoms.first) {
                                hexit_first_dirn = (i+1)%6;
                                WALK ("Good, hex in direction "
                                      << Hex::neighbour_pos(hexit_first_dirn) << ", which is "
                                      << hexit->outputRG()
                                      << ", has ID = edgedoms.first = " << edgedoms.first);
                                break;
                            } else {
                                WALK ("Hex in direction " << Hex::neighbour_pos((i+1)%6)
                                      << " has ID!=edgedoms.first = " << edgedoms.first);
                            }
                        } // else No neighbour in direction " << ((i+1)%6) << " of a vertex hex.

                        if (hexit_first->has_neighbour ((i>0)?(i-1):5)) {
                            WLK2 ("Hex adjoining " << hexit_first->outputRG()
                                  << " to " << Hex::neighbour_pos((i>0)?(i-1):5)
                                  << " has f=" << f[hexit_first->get_neighbour ((i>0)?(i-1):5)->vi]
                                  << ", " << hexit_first->get_neighbour ((i>0)?(i-1):5)->outputRG());

                            // The next hex to be pointed to by hexit is the one with
                            // f==edgedoms.first
                            hexit = hexit_first->get_neighbour (i>0?i-1:5);

                            if (f[hexit->vi] == edgedoms.first) {
                                hexit_first_dirn = i>0?i-1:5;
                                WLK2 ("Good, hex in direction "
                                      << Hex::neighbour_pos(hexit_first_dirn) << ", which is "
                                      << hexit->outputRG()
                                      << ", has ID = edgedoms.first = " << edgedoms.first
                                      << " (edgedoms.second=" << edgedoms.second << ")");
                                break;
                            } else {
                                WLK2 ("Hex in direction " << Hex::neighbour_pos((i>0)?(i-1):5)
                                      << " has ID!=edgedoms.first = " << edgedoms.first);
                            }
                        } else {
                            // If we get here, then neither hex to each side of the initial hexes
                            // were on the edge. That means that the edge has length 2 vertices
                            // only.
                            hexit_first_dirn = (i>0)?(i-1):5;
                            WLK2 ("Okay, hex in direction "
                                 << Hex::neighbour_pos(hexit_first_dirn) << ", which is "
                                  << hexit->outputRG()
                                 << ", has neither ID. Only 2 vertices in the edge.");
                            break;
                        }

                    }
                }
                WLK2 ("After determining initial direction of edge, i=" << i
                      << " or vertex dirn: " << Hex::vertex_name(i));
                WLK2 ("...and direction to edgedoms.first Hex is " << hexit_first_dirn
                      << " or hex dirn: " << Hex::neighbour_pos(hexit_first_dirn));

                // Now point hexit_neighb at the hex_first containing edgedoms.second_first Look at
                // hex neighbours in directions i+1 and i-1 from hexit_first.
                bool found_second = false;
                // dirn from hexit_first to hexit_neighb which has edgedoms.second identity
                int hexit_second_dirn = numeric_limits<int>::max();
                j = (hexit_first_dirn+1)%6;
                if (hexit_first->has_neighbour (j)) {
                    // If we have a neighbour, then check if it's on the other side of the edge;
                    // i.e. that the initial vertex v.v lies between the neighbour
                    // hexit->get_neighbour(j) and hexit.
                    hexit_neighb = hexit_first->get_neighbour(j);
                    WLK2 ("hexit_neighb, which should be over the edge has f="
                         << f[hexit_neighb->vi] << ", " << hexit_neighb->outputRG()
                         << ". Comparing with edgedoms.second=" << edgedoms.second);

                    // Ok, there IS a neighbour in this direction. Check its identity.
                    if (f[hexit_neighb->vi] == edgedoms.second) {

                        // Match location against next_neighb_coord, too
                        if (next_neighb_coord.first != numeric_limits<Flt>::max()
                            && next_neighb_coord.second != numeric_limits<Flt>::max()) {
                            // We have a set next_neighb_coord. Now make sure we re-found that hex.
                            if (hexit_neighb->compare_coord (next_neighb_coord) == true) {
                                WALK ("1 putative next neighbour hex is in correct location! found_second=true");
                                found_second = true;
                                hexit_second_dirn = j;
                            } else {
                                WALK ("1 putative next neighbour hex is not in correct location.");
                            }

                        } else {
                            // No set next_neighb_coord. Can't decide if this is ambiguous, so say
                            // we found it.
                            WALK ("1 No set next_neighb_coord, assume putative is correct");
                            found_second = true;
                            hexit_second_dirn = j;
                        }
                    }
                }
                if (!found_second) {
                    j = (hexit_first_dirn>0)?(hexit_first_dirn-1):5;
                    if (hexit_first->has_neighbour (j)) {
                        hexit_neighb = hexit_first->get_neighbour(j);
                        WLK2 ("the other hexit_neighb, which should be over the edge has f="
                             << f[hexit_neighb->vi] << ", " << hexit_neighb->outputRG());
                        if (f[hexit_neighb->vi] == edgedoms.second) {

                            // Match location against next_neighb_coord, too
                            if (next_neighb_coord.first != numeric_limits<Flt>::max()
                                && next_neighb_coord.second != numeric_limits<Flt>::max()) {
                                // We have a set next_neighb_coord. Now make sure we re-found that hex.
                                if (hexit_neighb->compare_coord (next_neighb_coord) == true) {
                                    WALK ("2 putative next neighbour hex is in correct location! found_second=true");
                                    found_second = true;
                                    hexit_second_dirn = j;
                                } else {
                                    WALK ("2 putative next neighbour hex is not in correct location.");
                                }

                            } else {
                                // No set next_neighb_coord. Can't decide if this is ambiguous, so say
                                // we found it.
                                WALK ("2 No set next_neighb_coord, assume putative is correct");
                                found_second = true;
                                hexit_second_dirn = j;
                            }
                        }
                    } else {
                        WLK2 ("No neighbour in direction j=" << j);
                    }
                }
                if (!found_second) {
                    throw runtime_error ("Whoop whoop - failed to find the second hex associated with the initial vertex!");
                }

                WALK ("hexit_first: "<< hexit_first->outputRG()
                      << ", hexit: " << hexit->outputRG()
                      << ", hexit_neighb: " << hexit_neighb->outputRG());

                // Can now say whether the edgedoms are in clockwise or anti-clockwise order.
                Rotn rot = Rotn::Unknown;
                int hex_hex_neighb_dirn = numeric_limits<int>::max();
                if (hexit_second_dirn == ((hexit_first_dirn>0)?(hexit_first_dirn-1):5)) {
                    // Rotation of edgedoms.first to edgedoms.second is clockwise around hexit_first.

                    // Direction from hexit to hexit_neighb is the hexit anti-direction + 1
                    hex_hex_neighb_dirn = (((hexit_first_dirn+3)%6)+1)%6;

                    // Rotation from hexit_first to hexit_neighb is therefore ANTI-clockwise.
                    WALK ("Rotate anticlockwise around hexit");
                    rot = Rotn::Anticlock;

                } else if (hexit_second_dirn  == (hexit_first_dirn+1)%6) {
                    // Rotation of edgedoms.first to edgedoms.second is anti-clockwise around
                    // hexit_first

                    // Direction from hexit to hexit_neighb is the hexit anti-direction - 1
                    hex_hex_neighb_dirn = (hexit_first_dirn+3)%6;
                    hex_hex_neighb_dirn = hex_hex_neighb_dirn>0?(hex_hex_neighb_dirn-1):5;

                    // Rotation from hexit_first to hexit_neighb is therefore CLOCKWISE.
                    WALK ("Rotate clockwise around hexit");
                    rot = Rotn::Clock;

                } // else rot == Rotn::Unknown

                // Now hexit and hexit_neighb hexes straddle the edge that I want to walk along, and
                // I know which way to rotate around hexit to find all the edge vertices that
                // surround hexit.

                // It should be that case that hexit_neighb ==
                // hexit->get_neighbour(hex_hex_neighb_dirn);
                if (hexit_neighb != hexit->get_neighbour(hex_hex_neighb_dirn)) {
                    throw runtime_error ("hexit_neighb is in the WRONG direction.");
                }

                // Here we have, in hexit, a hex with value edgedoms.first. Find the neighbour hex
                // with value edgedoms.second and add two vertices to v.edge accordingly.

                // Rotate all the way around each "inner edge hex", starting from
                // hex_hex_neighb_dirn.  Rotation may be clockwise or anticlockwise.
                int last_j;
                if (rot == Rotn::Anticlock) {
                    last_j = hex_hex_neighb_dirn>0?(hex_hex_neighb_dirn-1):5;
                } else {
                    last_j = (hex_hex_neighb_dirn+1)%6;
                }

                // This for loop rotates around each inner edge hexit:
                hexit_last = hexit_neighb;
                for (j  = hex_hex_neighb_dirn;
                     j != last_j;
                     j  = (rot == Rotn::Anticlock)?((j+1)%6):(j>0?j-1:5) ) {

                    if (!hexit->has_neighbour (j)) {
                        WLK2 ("No neighbour in direction " << Hex::neighbour_pos(j));
                        // so edge ends
                        v_init = hexit->get_vertex_coord (j>0?j-1:5);
                        WALK ("Edge ends; push_back very last vertex coordinate "
                              << (j>0?j-1:5) << " for the path: ("
                              << v_init.first << "," << v_init.second << ")");
                        path.push_back (v_init);
                        next_neighb_dom = -1.0;
                        next_neighb_coord = make_pair<Flt, Flt>(numeric_limits<Flt>::max(),
                                                                numeric_limits<Flt>::max());
                        partner_found = true;
                        next_one = v_init;
                        break;
                    } else {
                        // If we have a neighbour, then check if it's on the other side of the edge.
                        hexit_next = hexit->get_neighbour (j);

                        DBG2 ("hexit_next, " << hexit_next->outputRG()
                              << ", which should be over the edge has f=" << f[hexit_next->vi]);
                        if (f[hexit_next->vi] == edgedoms.second) {
                            // hexit_next has identity edgedoms.second, so add vertex j
                            v_init = hexit->get_vertex_coord (j>0?j-1:5);
                            WALK ("push_back vertex coordinate " << (j>0?j-1:5) << " for the path: ("
                                  << v_init.first << "," << v_init.second << ")");
                            path.push_back (v_init);
                            // Update hexit_last
                            hexit_last = hexit_next;

                        } else {
                            WLK2 ("This neighbour does not have identity = edgedoms.second = "
                                  << edgedoms.second);
                            if (f[hexit_next->vi] == edgedoms.first) {
                                WLK2 ("This neighbour DOES have identity = edgedoms.first = "
                                      << edgedoms.first);

                                v_init = hexit->get_vertex_coord (j>0?j-1:5);

                                // This is the time to cycle around the hexes
                                WLK2 ("Setting up next hexits...");
                                WLK2 ("Set hexit_first to " << hexit->outputRG());
                                hexit_first = hexit;
                                WLK2 ("Set hexit to " << hexit_next->outputRG());
                                hexit = hexit_next;
                                WLK2 ("Set hexit_neighb to hexit_last: " << hexit_last->outputRG());
                                // hexit_last is the last neighbour with identity edgedoms.second
                                hexit_neighb = hexit_last;
                                break;
                            } else {
                                WALK ("Neither of the edgedom identities, must be end of the edge");
                                v_init = hexit->get_vertex_coord (j>0?j-1:5);
                                WALK ("Edge ends; push_back final vertex coordinate " << (j>0?j-1:5)
                                      << " for the path: ("
                                      << v_init.first << "," << v_init.second << ")");
                                path.push_back (v_init);
                                next_one = v_init;
                                next_neighb_dom = f[hexit_next->vi];
                                WALK ("Set next_neighb_dom to " << next_neighb_dom
                                      << " with edgedoms "
                                      << edgedoms.first << "," << edgedoms.second);
                                // Record location of next neighbouring hex, to ensure we find the
                                // correct next neighbour when processing the next vertex in the domain.
                                next_neighb_coord.first = hexit_next->x;
                                next_neighb_coord.second = hexit_next->y;

                                partner_found = true;
                                break;
                            }
                        }
                    }
                }
            } // end while !partner_found

            WALK ("*********************  returning **********************");
            return next_one;
        }

        /*!
         * Walk out to the next vertex from vertx @v on HexGrid @hg for which identities are in @f.
         *
         * @hg The HexGrid on which the action takes place
         *
         * @f The identity variable.
         *
         * @v The Dirichlet Vertex that we're walking from, and into which we're going to write the
         * path to the next neighbour
         *
         * @next_neighb_dom The identity of the next domain neighbour when we find the vertex
         * neighbour.
         */
        static pair<Flt, Flt>
        walk_to_next (HexGrid* hg, vector<Flt>& f, DirichVtx<Flt>& v,
                      Flt& next_neighb_dom, pair<Flt, Flt>& next_neighb_coord) {

            DBG ("Called");

            // Starting from hex v.hi, find neighbours whos f values are v.f/v.neighb.first. Record
            // (in v.path_to_next) a series of coordinates that make up the path between that vertex
            // and the next vertex in the domain.
            pair<Flt, Flt> edgedoms;
            edgedoms.first = v.f;
            edgedoms.second = v.neighb.first;
            DBG ("set edgedoms to [" << edgedoms.first << "," << edgedoms.second << "]");

            return walk_common (hg, f, v, v.pathto_next, edgedoms,
                                next_neighb_dom, next_neighb_coord);
        }

        /*!
         * Walk out to a neighbour from vertex @v.
         *
         * @hg The HexGrid on which the action takes place
         *
         * @f The identity variable.
         *
         * @v The Dirichlet Vertex that we're walking from, and into which we're going to write the
         * path to the next neighbour
         *
         * @next_neighb_dom The identity of the next domain neighbour when we find the vertex
         * neighbour.
         */
        static pair<Flt, Flt>
        walk_to_neighbour (HexGrid* hg, vector<Flt>& f, DirichVtx<Flt>& v,
                           Flt& next_neighb_dom, pair<Flt, Flt>& next_neighb_coord) {

            DBG ("Called");

            // Don't set neighbours for the edge vertices (though
            // edge vertices *can be set* as neighbours for other
            // vertices).
            if (v.neighb.first == -1.0f || v.neighb.second == -1.0f) {
                return make_pair(0.0, 0.0);;
            }

            pair<Flt, Flt> edgedoms = v.neighb;

            return walk_common (hg, f, v, v.pathto_neighbour, edgedoms,
                                next_neighb_dom, next_neighb_coord);
        }

        /*!
         * Given an iterator into the list of DirichVtxs @vertices, find the next vertex in the
         * domain, along with the vertex neighbours, and recurse until @domain has been populated
         * with all the vertices that define it.
         *
         * Return true for success, false for failure, and leave dv pointing to the next vertex in
         * vertices so that @domain can be stored, reset and the next Dirichlet domain can be found.
         */
        static bool
        process_domain (HexGrid* hg, vector<Flt>& f,
                        typename list<DirichVtx<Flt>>::iterator dv,
                        list<DirichVtx<Flt>>& vertices,
                        list<DirichVtx<Flt>>& domain,
                        DirichVtx<Flt> first_vtx,
                        pair<Flt, Flt>& next_neighb_coord) {

            DBG ("Called");

            // Domain ID is set in dv as dv->f;
            DirichVtx<Flt> v = *dv;

            // On the first call, first_vtx should have been set to vertices.end()
            if (first_vtx.unset()) {
                // Mark the first vertex in our domain
                DBG ("Mark first vertex at (" << v.v.first << "," << v.v.second << ")");
                first_vtx = v;
                DBG ("First vertex  has v.f=" << v.f << ", and v.neighb.first/second="
                     << v.neighb.first << "/" << v.neighb.second);
            } else {
                DBG ("Don't update first_vtx as it was already set.");
            }

            // Find the neighbour of this vertex, if possible. Can't
            // do this if it's a boundary vertex, but nothing happens
            // in that case.
            Flt next_neighb_dom = numeric_limits<Flt>::max();
#if 0
            DBG ("walk_to_neighbour...");
            pair<Flt, Flt> neighb_vtx = walk_to_neighbour (hg, f, v, next_neighb_dom, next_neighb_coord);
            DBG ("walk_to_neighbour returned with vertex (" << neighb_vtx.first
                 << "," << neighb_vtx.second << ")");
#endif
            // Walk to the next vertex
            next_neighb_dom = numeric_limits<Flt>::max();
            pair<Flt, Flt> next_vtx = walk_to_next (hg, f, v, next_neighb_dom, next_neighb_coord);
            DBG ("starting from (" << v.v.first << "," << v.v.second
                 << "), walk_to_next returned with vertex ("
                 << next_vtx.first << "," << next_vtx.second << ")");

            dv->closed = true;
            domain.push_back (v);

            typename list<DirichVtx<Flt>>::iterator dv2 = vertices.begin();
            if (first_vtx.compare (next_vtx) == false) {
                DBG ("next_vtx (" << next_vtx.first << "," << next_vtx.second << ") != first_vtx ("
                     << first_vtx.v.first << "," << first_vtx.v.second <<  "), right?");
                // Find a dv which matches next_vtx.
                bool matched_next_vertex = false;
                DBG ("Search vertices for (" << next_vtx.first << "," << next_vtx.second << ")");
                while (dv2 != vertices.end()) {
                    // Instead of: dv2 = next_vtx;, do:
                    if (dv2->closed == false && dv2->compare (next_vtx) == true) {
                        // vertex has correct coordinate. Check it has correct neighbours.
                        DBG ("Coordinate match. Is dv2->f == v.f? " << dv2->f << " == " << v.f << "?");
                        DBG ("Is dv2->neighb.first == next_neighb_dom? " << dv2->neighb.first
                             << " == " << next_neighb_dom << "?");
                        DBG ("Is dv2->neighb.second == v.neighb.first? " << dv2->neighb.second
                             << " == " << v.neighb.first << "?");
                        DBG ("(v.neighb.second = " << v.neighb.second << ")");
                        if (dv2->f == v.f
                            && dv2->neighb.second == v.neighb.first
                            && dv2->neighb.first == next_neighb_dom) {
                            // Match for current dv2
                            DBG ("Match");
                            matched_next_vertex = true;
                            dv = dv2;
                            break;
                        } else {
                            DBG ("No match");
                        }
#if 0
                    } else {
                        DBG ("Closed vertex or vertex with INcorrect coordinate; ("
                             << dv2->v.first << "," << dv2->v.second << ")");
#endif
                    }
                    ++dv2;
                }
                if (!matched_next_vertex) {
                    DBG ("Failed to find a match for the next_vtx which walk_to_next found. Return false.");
                    return false;
                }

            } else {
                cout << "Walk to next arrived back at the first vertex. Return true" << endl;
                return true;
            }

            // Now move on to the next vertex in the domain, re-calling process_domain recursively,
            // or exiting if we got to the start of the domain perimeter. We shouldn't get anywhere
            // close to the recursion limit in this system.
            if (dv->onBoundary == false) {
                DBG ("Recursively call process_domain...");
                bool result = process_domain (hg, f, dv, vertices, domain, first_vtx, next_neighb_coord);
                DBG ("At end, return result=" << result);
                return result;
            }

            DBG ("Arrived at a boundary vertex. Return false (because we didn't find a full domain");
            return false;
        }

        /*!
         * Determine the locations of the vertices on a Hex grid which are surrounded by three
         * different values of @f. @f is indexed by the HexGrid @hg. Return a list containing lists
         * of the vertices, each of which define a domain.
         */
        static list<list<DirichVtx<Flt> > >
        dirichlet_vertices (HexGrid* hg, vector<Flt>& f, list<DirichVtx<Flt> >& vertices) {

            // 1. Go though and find a list of all vertices, in no particular order.  This will lead
            // to duplications because >1 domain for a given ID, f, is possible early in
            // simulations. From this list, I can find vertex sets, whilst deleting from the list
            // until it is empty, and know that I will have discovered all the domain vertex sets.
            // list<DirichVtx<Flt> > vertices;
            list<Hex>::iterator h = hg->hexen.begin();
            while (h != hg->hexen.end()) {
                vertex_test (hg, f, h, vertices);
                // Move on to the next Hex in hexen
                ++h;
            }

            // 2. Delete from the list<DirichVtx> and construct a list<list<DirichVtx>> of all the
            // domains. The list<DirichVtx> for a single domain should be ordered so that the
            // perimeter of the domain is traversed. I have to do Dirichlet domain boundary walks to
            // achieve this (to disambiguate between vertices from separate, but same-ID domains).
            list<list<DirichVtx<Flt>>> dirich_domains;
            typename list<DirichVtx<Flt>>::iterator dv = vertices.begin();
            unsigned int domcount = 0;
            while (dv != vertices.end() && domcount++ < 3) {
                list<DirichVtx<Flt>> one_domain;
                DirichVtx<Flt> first_vtx;
                if (dv->hi->boundaryHex == true) {
                    DBG ("Don't process hexes on the boundary");
                    dv->closed = true;
                    dv++;
                } else {
                    pair<Flt, Flt> next_neighb_coord = {numeric_limits<Flt>::max(), numeric_limits<Flt>::max()};
                    bool success = process_domain (hg, f, dv, vertices, one_domain, first_vtx, next_neighb_coord);
                    dv++;
                    if (success) {
                        dirich_domains.push_back (one_domain);
                    } else {
                        DBG ("process_domain failed to find the outline of a domain");
                    }
                }
            }

            DBG ("Returning dirich_domains, which has size " << dirich_domains.size() << " domcount=" << domcount);
            return dirich_domains;
        }

        /*!
         * Save all the information contained in a set of dirichlet vertices to HDF5 into the
         * HdfData @data. The set(list?) of Dirichlet vertices is for one single Dirichlet domain.
         */
        static void
        dirichlet_save_vertex_set (HdfData& data /* container of domains */) {

        }

    }; // ShapeAnalysis

} // namespace morph

#endif // SHAPEANALYSIS