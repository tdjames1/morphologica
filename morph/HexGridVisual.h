#pragma once

#include "GL3/gl3.h"
#include "morph/tools.h"
#include "morph/VisualDataModel.h"
#include "morph/ColourMap.h"
#include "morph/HexGrid.h"
#include "morph/MathAlgo.h"
#include "morph/Vector.h"
#include <iostream>
#include <vector>
#include <array>

/*!
 * Macros for testing neighbours. The step along for neighbours on the
 * rows above/below is given by:
 *
 * Dest  | step
 * ----------------------
 * NNE   | +rowlen
 * NNW   | +rowlen - 1
 * NSW   | -rowlen
 * NSE   | -rowlen + 1
 */
//@{
#define NE(hi) (this->hg->d_ne[hi])
#define HAS_NE(hi) (this->hg->d_ne[hi] == -1 ? false : true)

#define NW(hi) (this->hg->d_nw[hi])
#define HAS_NW(hi) (this->hg->d_nw[hi] == -1 ? false : true)

#define NNE(hi) (this->hg->d_nne[hi])
#define HAS_NNE(hi) (this->hg->d_nne[hi] == -1 ? false : true)

#define NNW(hi) (this->hg->d_nnw[hi])
#define HAS_NNW(hi) (this->hg->d_nnw[hi] == -1 ? false : true)

#define NSE(hi) (this->hg->d_nse[hi])
#define HAS_NSE(hi) (this->hg->d_nse[hi] == -1 ? false : true)

#define NSW(hi) (this->hg->d_nsw[hi])
#define HAS_NSW(hi) (this->hg->d_nsw[hi] == -1 ? false : true)
//@}

#define IF_HAS_NE(hi, yesval, noval)  (HAS_NE(hi)  ? yesval : noval)
#define IF_HAS_NNE(hi, yesval, noval) (HAS_NNE(hi) ? yesval : noval)
#define IF_HAS_NNW(hi, yesval, noval) (HAS_NNW(hi) ? yesval : noval)
#define IF_HAS_NW(hi, yesval, noval)  (HAS_NW(hi)  ? yesval : noval)
#define IF_HAS_NSW(hi, yesval, noval) (HAS_NSW(hi) ? yesval : noval)
#define IF_HAS_NSE(hi, yesval, noval) (HAS_NSE(hi) ? yesval : noval)

namespace morph {

    //! The template argument Flt is the type of the data which this HexGridVisual
    //! will visualize.
    template <class Flt>
    class HexGridVisual : public VisualDataModel<Flt>
    {
    public:
        //! Constructor which does not set colour map
        HexGridVisual(GLuint sp,
                      const HexGrid* _hg,
                      const Vector<float> _offset,
                      const std::vector<Flt>* _data) {
            // Set up...
            this->shaderprog = sp;
            this->offset = _offset;
            this->viewmatrix.translate (this->offset);

            // Defaults here. After init, you can reset (at computational cost) to get desired params
            this->zScale.setParams (1, 0);
            this->colourScale.do_autoscale = true;

            this->hg = _hg;
            this->scalarData = _data;

            this->initializeVertices();
            this->postVertexInit();
        }

        //! Constructor which does not set colour map and takes an std::array for
        //! _offset for backwards compatibility
        HexGridVisual(GLuint sp,
                      const HexGrid* _hg,
                      const std::array<float, 3> _offset,
                      const std::vector<Flt>* _data) {
            // Set up...
            this->shaderprog = sp;
            this->offset.set_from (_offset);
            this->viewmatrix.translate (this->offset);

            // Defaults here. After init, you can reset (at computational cost) to get desired params
            this->zScale.setParams (1, 0);
            this->colourScale.do_autoscale = true;

            this->hg = _hg;
            this->scalarData = _data;

            this->initializeVertices();
            this->postVertexInit();
        }

        //! Constructor which sets default colour map
        HexGridVisual(GLuint sp,
                      const HexGrid* _hg,
                      const Vector<float> _offset,
                      const std::vector<Flt>* _data,
                      ColourMapType _cmt,
                      const float _hue = 0.0f) {
            // Set up...
            this->shaderprog = sp;
            this->offset = _offset;
            this->viewmatrix.translate (this->offset);

            // Defaults here. After init, you can reset (at computational cost) to get desired params
            this->zScale.setParams (1, 0);
            this->colourScale.do_autoscale = true;

            this->hg = _hg;
            this->scalarData = _data;

            this->cm.setHue (_hue);
            this->cm.setType (_cmt);

            this->initializeVertices();
            this->postVertexInit();
        }

        //! Constructor which sets default colour map and takes an std::array for
        //! _offset for backwards compatibility
        HexGridVisual(GLuint sp,
                      const HexGrid* _hg,
                      const std::array<float, 3> _offset,
                      const std::vector<Flt>* _data,
                      ColourMapType _cmt,
                      const float _hue = 0.0f) {
            // Set up...
            this->shaderprog = sp;
            this->offset.set_from (_offset);
            this->viewmatrix.translate (this->offset);

            // Defaults here. After init, you can reset (at computational cost) to get desired params
            this->zScale.setParams (1, 0);
            this->colourScale.do_autoscale = true;

            this->hg = _hg;
            this->scalarData = _data;

            this->cm.setHue (_hue);
            this->cm.setType (_cmt);

            this->initializeVertices();
            this->postVertexInit();

        }

        //! Constructor which sets default colour map and z/colour Scale objects
        HexGridVisual(GLuint sp,
                      const HexGrid* _hg,
                      const Vector<float> _offset,
                      const std::vector<Flt>* _data,
                      const Scale<Flt>& zscale,
                      const Scale<Flt>& cscale,
                      ColourMapType _cmt,
                      const float _hue = 0.0f) {
            // Set up...
            this->shaderprog = sp;
            this->offset = _offset;
            this->viewmatrix.translate (this->offset);

            this->zScale = zscale;
            this->colourScale = cscale;

            this->hg = _hg;
            this->scalarData = _data;

            this->cm.setHue (_hue);
            this->cm.setType (_cmt);

            this->initializeVertices();
            this->postVertexInit();
        }

        //! Constructor which sets default colour map and z/colour Scale objects and
        //! takes an std::array for _offset for backwards compatibility
        HexGridVisual(GLuint sp,
                      const HexGrid* _hg,
                      const std::array<float, 3> _offset,
                      const std::vector<Flt>* _data,
                      const Scale<Flt>& zscale,
                      const Scale<Flt>& cscale,
                      ColourMapType _cmt,
                      const float _hue = 0.0f) {
            // This (subcalling another constructor) failed:
            //Vector<float> offset_vec;
            //offset_vec.set_from(_offset);
            // HexGridVisual (sp, _hg, offset_vec, _data, zscale, cscale, _cmt, _hue);
            //
            // So do it long-hand:
            this->shaderprog = sp;
            this->offset.set_from (_offset);
            this->viewmatrix.translate (this->offset);

            this->zScale = zscale;
            this->colourScale = cscale;

            this->hg = _hg;
            this->scalarData = _data;

            this->cm.setHue (_hue);
            this->cm.setType (_cmt);

            this->initializeVertices();
            this->postVertexInit();
        }

        //! Do the computations to initialize the vertices that will represent the
        //! HexGrid.
        void initializeVertices (void) {
            this->initializeVerticesHexesInterpolated();
            // or:
            // this->initializeVerticesTris();
        }

        // Initialize vertex buffer objects and vertex array object.

        //! Initialize as triangled. Gives a smooth surface with much
        //! less compute than initializeVerticesHexesInterpolated.
        void initializeVerticesTris (void) {
            unsigned int nhex = this->hg->num();
            for (unsigned int hi = 0; hi < nhex; ++hi) {
                // Scale z:
                Flt datumC = this->zScale.transform ((*this->data)[hi]);
                // Scale colour
                Flt datum = this->colourScale.transform ((*this->data)[hi]);
                // And turn it into a colour:
                std::array<float, 3> clr = this->cm.convert (datum);
                this->vertex_push (this->hg->d_x[hi], this->hg->d_y[hi], datumC, this->vertexPositions);
                this->vertex_push (clr, this->vertexColors);
                this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);
            }

            // Build indices based on neighbour relations in the HexGrid
            for (unsigned int hi = 0; hi < nhex; ++hi) {
                if (HAS_NNE(hi) && HAS_NE(hi)) {
                    //std::cout << "1st triangle " << hi << "->" << NNE(hi) << "->" << NE(hi) << std::endl;
                    this->indices.push_back (hi);
                    this->indices.push_back (NNE(hi));
                    this->indices.push_back (NE(hi));
                }

                if (HAS_NW(hi) && HAS_NSW(hi)) {
                    //std::cout << "2nd triangle " << hi << "->" << NW(hi) << "->" << NSW(hi) << std::endl;
                    this->indices.push_back (hi);
                    this->indices.push_back (NW(hi));
                    this->indices.push_back (NSW(hi));
                }
            }
        }

        //! Initialize as hexes, with z position of each of the 6
        //! outer edges of the hexes interpolated, but a single colour
        //! for each hex. Gives a smooth surface.
        void initializeVerticesHexesInterpolated (void) {
            float sr = this->hg->getSR();
            float vne = this->hg->getVtoNE();
            float lr = this->hg->getLR();

            unsigned int nhex = this->hg->num();
            unsigned int idx = 0;

            std::vector<Flt> dcopy = *(this->scalarData);
            this->zScale.transform (*(this->scalarData), dcopy);
            std::vector<Flt> dcolour = *(this->scalarData);
            this->colourScale.transform (*(this->scalarData), dcolour);

            Flt datumC = static_cast<Flt>(0.0);   // datum at the centre
            Flt datumNE = static_cast<Flt>(0.0);  // datum at the hex to the east.
            Flt datumNNE = static_cast<Flt>(0.0); // etc
            Flt datumNNW = static_cast<Flt>(0.0);
            Flt datumNW = static_cast<Flt>(0.0);
            Flt datumNSW = static_cast<Flt>(0.0);
            Flt datumNSE = static_cast<Flt>(0.0);

            Flt datum = static_cast<Flt>(0.0);
            Flt third = static_cast<Flt>(0.33333333333333);
            Flt half = static_cast<Flt>(0.5);
            for (unsigned int hi = 0; hi < nhex; ++hi) {

                // Use the linear scaled copy of the data, dcopy.
                datumC   = dcopy[hi];
                datumNE  = HAS_NE(hi)  ? dcopy[NE(hi)]  : datumC; // datum Neighbour East
                datumNNE = HAS_NNE(hi) ? dcopy[NNE(hi)] : datumC; // datum Neighbour North East
                datumNNW = HAS_NNW(hi) ? dcopy[NNW(hi)] : datumC; // etc
                datumNW  = HAS_NW(hi)  ? dcopy[NW(hi)]  : datumC;
                datumNSW = HAS_NSW(hi) ? dcopy[NSW(hi)] : datumC;
                datumNSE = HAS_NSE(hi) ? dcopy[NSE(hi)] : datumC;

                // Use a single colour for each hex, even though hex z positions are
                // interpolated. Do the _colour_ scaling:
                std::array<float, 3> clr = this->cm.convert (dcolour[hi]);

                // First push the 7 positions of the triangle vertices, starting with the centre
                this->vertex_push (this->hg->d_x[hi], this->hg->d_y[hi], datumC, this->vertexPositions);

                // NE vertex
                if (HAS_NNE(hi) && HAS_NE(hi)) {
                    // Compute mean of this->data[hi] and NE and E hexes
                    datum = third * (datumC + datumNNE + datumNE);
                } else if (HAS_NNE(hi) || HAS_NE(hi)) {
                    if (HAS_NNE(hi)) {
                        datum = half * (datumC + datumNNE);
                    } else {
                        datum = half * (datumC + datumNE);
                    }
                } else {
                    datum = datumC;
                }
                this->vertex_push (this->hg->d_x[hi]+sr, this->hg->d_y[hi]+vne, datum, this->vertexPositions);

                // SE vertex
                if (HAS_NE(hi) && HAS_NSE(hi)) {
                    datum = third * (datumC + datumNE + datumNSE);
                } else if (HAS_NE(hi) || HAS_NSE(hi)) {
                    if (HAS_NE(hi)) {
                        datum = half * (datumC + datumNE);
                    } else {
                        datum = half * (datumC + datumNSE);
                    }
                } else {
                    datum = datumC;
                }
                this->vertex_push (this->hg->d_x[hi]+sr, this->hg->d_y[hi]-vne, datum, this->vertexPositions);

                // S
                if (HAS_NSE(hi) && HAS_NSW(hi)) {
                    datum = third * (datumC + datumNSE + datumNSW);
                } else if (HAS_NSE(hi) || HAS_NSW(hi)) {
                    if (HAS_NSE(hi)) {
                        datum = half * (datumC + datumNSE);
                    } else {
                        datum = half * (datumC + datumNSW);
                    }
                } else {
                    datum = datumC;
                }
                this->vertex_push (this->hg->d_x[hi], this->hg->d_y[hi]-lr, datum, this->vertexPositions);

                // SW
                if (HAS_NW(hi) && HAS_NSW(hi)) {
                    datum = third * (datumC + datumNW + datumNSW);
                } else if (HAS_NW(hi) || HAS_NSW(hi)) {
                    if (HAS_NW(hi)) {
                        datum = half * (datumC + datumNW);
                    } else {
                        datum = half * (datumC + datumNSW);
                    }
                } else {
                    datum = datumC;
                }
                this->vertex_push (this->hg->d_x[hi]-sr, this->hg->d_y[hi]-vne, datum, this->vertexPositions);

                // NW
                if (HAS_NNW(hi) && HAS_NW(hi)) {
                    datum = third * (datumC + datumNNW + datumNW);
                } else if (HAS_NNW(hi) || HAS_NW(hi)) {
                    if (HAS_NNW(hi)) {
                        datum = half * (datumC + datumNNW);
                    } else {
                        datum = half * (datumC + datumNW);
                    }
                } else {
                    datum = datumC;
                }
                this->vertex_push (this->hg->d_x[hi]-sr, this->hg->d_y[hi]+vne, datum, this->vertexPositions);

                // N
                if (HAS_NNW(hi) && HAS_NNE(hi)) {
                    datum = third * (datumC + datumNNW + datumNNE);
                } else if (HAS_NNW(hi) || HAS_NNE(hi)) {
                    if (HAS_NNW(hi)) {
                        datum = half * (datumC + datumNNW);
                    } else {
                        datum = half * (datumC + datumNNE);
                    }
                } else {
                    datum = datumC;
                }
                this->vertex_push (this->hg->d_x[hi], this->hg->d_y[hi]+lr, datum, this->vertexPositions);

                // All normals point up
                this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);
                this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);
                this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);
                this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);
                this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);
                this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);
                this->vertex_push (0.0f, 0.0f, 1.0f, this->vertexNormals);

                // Seven vertices with the same colour
                this->vertex_push (clr, this->vertexColors);
                this->vertex_push (clr, this->vertexColors);
                this->vertex_push (clr, this->vertexColors);
                this->vertex_push (clr, this->vertexColors);
                this->vertex_push (clr, this->vertexColors);
                this->vertex_push (clr, this->vertexColors);
                this->vertex_push (clr, this->vertexColors);

                // Define indices now to produce the 6 triangles in the hex
                this->indices.push_back (idx+1);
                this->indices.push_back (idx);
                this->indices.push_back (idx+2);

                this->indices.push_back (idx+2);
                this->indices.push_back (idx);
                this->indices.push_back (idx+3);

                this->indices.push_back (idx+3);
                this->indices.push_back (idx);
                this->indices.push_back (idx+4);

                this->indices.push_back (idx+4);
                this->indices.push_back (idx);
                this->indices.push_back (idx+5);

                this->indices.push_back (idx+5);
                this->indices.push_back (idx);
                this->indices.push_back (idx+6);

                this->indices.push_back (idx+6);
                this->indices.push_back (idx);
                this->indices.push_back (idx+1);

                idx += 7; // 7 vertices (each of 3 floats for x/y/z), 18 indices.
            }
        }

        //! Initialize as hexes, with a step quad between each
        //! hex. Might look cool. Writeme.
        void initializeVerticesHexesStepped (void) {}

    private:
        //! The HexGrid to visualize
        const HexGrid* hg;
    };

} // namespace morph
