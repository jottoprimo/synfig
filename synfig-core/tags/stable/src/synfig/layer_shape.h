/* === S Y N F I G ========================================================= */
/*!	\file layer_shape.h
**	\brief Header file for implementation of the "Shape" layer
**
**	$Id$
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2007-2008 Chris Moore
**
**	This package is free software; you can redistribute it and/or
**	modify it under the terms of the GNU General Public License as
**	published by the Free Software Foundation; either version 2 of
**	the License, or (at your option) any later version.
**
**	This package is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
**	General Public License for more details.
**	\endlegal
*/
/* ========================================================================= */

/* === S T A R T =========================================================== */

#ifndef __SYNFIG_LAYER_SHAPE_H
#define __SYNFIG_LAYER_SHAPE_H

/* === H E A D E R S ======================================================= */

#include "layer_composite.h"
#include "color.h"
#include "vector.h"
#include "blur.h"

#include <vector>

/* === M A C R O S ========================================================= */

/* === T Y P E D E F S ===================================================== */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace synfig {

/*!	\class Layer_Shape
**	\brief writeme			*/
class Layer_Shape : public Layer_Composite, public Layer_NoDeform
{
	SYNFIG_LAYER_MODULE_EXT

	enum WindingStyle
	{
	    WINDING_NON_ZERO=0,			//!< less than -1 --> 1;  -1 --> 1;   0 --> 0;   1 --> 1;  greater than 1 --> 1
	    WINDING_EVEN_ODD=1,			//!< add or subtract multiples of 2 to get into range -1:1, then as above

	    WINDING_END=2				//!< \internal
	};

private:

	//internal caching
	struct Intersector;
	Intersector	*edge_table;

	//exported data
	Color 					color;

	Point 	origin;
	bool	invert;
	bool	antialias;

	int		blurtype;
	Real	feather;
	WindingStyle winding_style;

	std::vector< char > 	bytestream;

	//for use in creating the bytestream
	int						lastbyteop;
	int						lastoppos;

protected:

	Layer_Shape(const Real &a = 1.0, const Color::BlendMethod m = Color::BLEND_COMPOSITE);

public:

	~Layer_Shape();

	//! Clears out any data
	/*!	Also clears out the Intersector
	*/
	void clear();
	//void sync();

	void move_to(Real x, Real y);
	void line_to(Real x, Real y);
	void conic_to(Real x1, Real y1, Real x, Real y);
	void conic_to_smooth(Real x, Real y);				//x1,y1 derived from current tangent
	void curve_to(Real x1, Real y1, Real x2, Real y2, Real x, Real y);
	void curve_to_smooth(Real x2, Real y2, Real x, Real y);	//x1,y1 derived from current tangent
	void close();
	void endpath();

	virtual bool set_param(const String & param, const synfig::ValueBase &value);
	virtual ValueBase get_param(const String & param)const;

	virtual Vocab get_param_vocab()const;

	virtual Color get_color(Context context, const Point &pos)const;
	virtual bool accelerated_render(Context context,Surface *surface,int quality, const RendDesc &renddesc, ProgressCallback *cb)const;
	virtual synfig::Layer::Handle hit_check(synfig::Context context, const synfig::Point &point)const;
	virtual Rect get_bounding_rect()const;

private:
	class 		PolySpan;
	bool render_polyspan(Surface *surface,PolySpan &polyspan,
						Color::BlendMethod method,Color::value_type amount)const;
	bool render_polyspan(etl::surface<float> *surface,PolySpan &polyspan)const;
	virtual bool render_shape(Surface *surface,bool useblend,int quality,const RendDesc &renddesc, ProgressCallback *cb)const;
	virtual bool render_shape(etl::surface<float> *surface,int quality,const RendDesc &renddesc, ProgressCallback *cb)const;
}; // END of Layer_Shape

}; // END of namespace synfig
/* === E N D =============================================================== */

#endif
