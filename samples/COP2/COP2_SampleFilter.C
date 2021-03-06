/*
 * Copyright (c) 2015
 *	Side Effects Software Inc.  All rights reserved.
 *
 * Redistribution and use of Houdini Development Kit samples in source and
 * binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. The name of Side Effects Software may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE `AS IS' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *----------------------------------------------------------------------------
 */
#include <UT/UT_DSOVersion.h>
#include <OP/OP_OperatorTable.h>

#include <PRM/PRM_Include.h>
#include <PRM/PRM_Parm.h>

#include <SYS/SYS_Math.h>

#include <TIL/TIL_Region.h>
#include <TIL/TIL_Tile.h>
#include <TIL/TIL_TileList.h>

#include <PXL/PXL_Pixel.h>
#include <RU/RU_Algorithm.h>
#include <COP2/COP2_CookAreaInfo.h>

#include "COP2_SampleFilter.h"

using namespace HDK_Sample;

COP_MASK_SWITCHER(4, "HDK Sample Filter");

static PRM_Name names[] =
{
    PRM_Name("left",		"Left Enhance"),
    PRM_Name("right",		"Right Enhance"),
    PRM_Name("top",		"Top Enhance"),
    PRM_Name("bottom",		"Bottom Enhance"),
};


PRM_Template
COP2_SampleFilter::myTemplateList[] =
{
    PRM_Template(PRM_SWITCHER,	3, &PRMswitcherName, switcher),

    PRM_Template(PRM_FLT_J,	TOOL_PARM, 1, &names[0],PRMzeroDefaults),
    PRM_Template(PRM_FLT_J,	TOOL_PARM, 1, &names[1],PRMzeroDefaults),
    PRM_Template(PRM_FLT_J,	TOOL_PARM, 1, &names[2],PRMzeroDefaults),
    PRM_Template(PRM_FLT_J,	TOOL_PARM, 1, &names[3],PRMzeroDefaults),
    
    PRM_Template(),
};

OP_TemplatePair COP2_SampleFilter::myTemplatePair( COP2_SampleFilter::myTemplateList,
					     &COP2_MaskOp::myTemplatePair);

OP_VariablePair COP2_SampleFilter::myVariablePair( 0,&COP2_Node::myVariablePair );

const char *	COP2_SampleFilter::myInputLabels[] =
{
    "Image to Enhance",
    "Mask Input",
    0
};
     
   
OP_Node *
COP2_SampleFilter::myConstructor(OP_Network	*net,
				 const char	*name,
				 OP_Operator	*op)
{
    return new COP2_SampleFilter(net, name, op);
}

COP2_SampleFilter::COP2_SampleFilter(OP_Network *parent,
			       const char *name,
			       OP_Operator *entry)
    : COP2_MaskOp(parent, name, entry)
{}

COP2_SampleFilter::~COP2_SampleFilter()
{}

// -----------------------------------------------------------------------
 
// No cookSequenceInfo override - we'll let COP2_MaskOp do our work for us.
// This node doesn't alter any of the sequence parms - res, range or planes.



// Stash some of our data in our shiny new context data.
COP2_ContextData *
COP2_SampleFilter::newContextData(const TIL_Plane *, int ,
				  float t, int xres, int yres,
				  int , int)
{
    // Necessary since parameters cannot be evaluated in doCookMyTile
    
    cop2_SampleFilterContext *data = new cop2_SampleFilterContext;
    float scx, scy;

    // The frame/scope effect allows the user to dial down the entire operation.
    int		index = mySequence.getImageIndex(t);
    float	effect = getFrameScopeEffect(index);

    // If cooking at a reduced res, scale down the effect for a closer
    // approximation.
    getScaleFactors(xres,yres, scx, scy);
    effect *= SYSmin(scx,scy);
    
    data->myLeft   = LEFT(t)	* effect;
    data->myRight  = RIGHT(t)	* effect;
    data->myTop    = TOP(t)	* effect;
    data->myBottom = BOTTOM(t)	* effect;

    data->myKernel = new float[9];
    // Kernel positions:
    // 0 1 2
    // 3 4 5 
    // 6 7 8

    data->myKernel[0] = -data->myLeft -data->myTop;
    data->myKernel[1] = -data->myTop;
    data->myKernel[2] = -data->myRight -data->myTop;

    data->myKernel[3] = -data->myLeft;
    data->myKernel[5] = -data->myRight;

    data->myKernel[6] = -data->myLeft -data->myBottom;
    data->myKernel[7] = -data->myBottom;
    data->myKernel[8] = -data->myRight -data->myBottom;

    // center
    data->myKernel[4] = 1.0f + 3.0f * (data->myLeft + data->myRight +
				       data->myTop + data->myBottom);
    return data;
};

// Our filter expands the image bounds by 1 in each direction.
void
COP2_SampleFilter::computeImageBounds(COP2_Context &context)
{
    int x1,y1,x2,y2;

    // Grab the bounds from the mask op, which combines the mask with the input
    // bounds.
    COP2_MaskOp::computeImageBounds(context);

    // Now enlarge the bounds by 1 in each direction to account for the 3x3
    // kernel.
    context.getImageBounds(x1,y1,x2,y2);
    context.setImageBounds(x1-1, y1-1, x2+1, y2+1);
}

// Tell the scheduler which part of the inputs' image data we require. 
void
COP2_SampleFilter::getInputDependenciesForOutputArea(
    COP2_CookAreaInfo &output_area,
    const COP2_CookAreaList &input_areas,
    COP2_CookAreaList &needed_areas)
{
    // Add dependencies on the first input and the mask plane.
    COP2_MaskOp::getInputDependenciesForOutputArea(output_area, input_areas,
						   needed_areas);

    // If bypassed, don't do anything else.
    if (getBypass())
	return;

    // Enlarge the needed area of the first input by 1 pixel in all directions.
    COP2_CookAreaInfo *inarea =
	makeOutputAreaDependOnMyPlane(0, output_area,input_areas,needed_areas);

    // It may not exist if the input node has an error.
    if(inarea)
	inarea->expandNeededArea(1, 1, 1, 1);
}



// Some friendly node-info help. You can evaluate parms here and present even
// more specific help, like "Apply [Box/Gaussian]" blur (see Blur COP info)
const char  *
COP2_SampleFilter::getOperationInfo()
{
    return "This operation enhances individual edges.";
}

namespace HDK_Sample {

// This class allows the various data types to be abstracted out.
class cop2_EdgeEnhance : public RU_Algorithm
{
public:
	     cop2_EdgeEnhance(const float *kernel) : myKernel(kernel) {}
    virtual ~cop2_EdgeEnhance() {}

    DECLARE_FILTER_OP(cop2_EdgeEnhanceOp);

    const float *myKernel;
};

// This is the template class which defines the image operation
template<class Type,int fast> class cop2_EdgeEnhanceOp
    : public RU_FilterOp<Type,fast>
{
public:
	         cop2_EdgeEnhanceOp(RU_Algorithm *alg)
		     : RU_FilterOp<Type,fast>(alg)
		 { ; }
    virtual      ~cop2_EdgeEnhanceOp() {;}

    virtual int	filter(TIL_TileList *output,
		       const TIL_Region *input, float t,
		       int thread=-1, void *data=0);
};

IMPLEMENT_FILTER_OP(cop2_EdgeEnhance, cop2_EdgeEnhanceOp);


template<class Type,int fast> int
cop2_EdgeEnhanceOp<Type,fast>::filter(TIL_TileList *output,
				      const TIL_Region *input, float t,
				      int thread, void *data)
{
    PXL_Pixel<Type,fast> pixel(output->myBlack, output->myWhite);
    cop2_EdgeEnhance *parm = 
	static_cast<cop2_EdgeEnhance *>(RU_FilterOp<Type,fast>::myAlg);
    const float *kernel = parm->myKernel;
    TIL_Tile	*itr=0;
    const Type	*source_data, *iscan1, *iscan2, *iscan3;
    Type	*dest_data, *scan;
    int		 ti;
    int		 stride, istride;
    float	 sum;
    int		 x,y;
    int		 w,h;

    w = output->myX2 - output->myX1 + 1;
    h = output->myY2 - output->myY1 + 1;
    stride = w;
    istride = w + 2;
    
    FOR_EACH_UNCOOKED_TILE(output, itr, ti)
    {
	dest_data     = (Type *) itr->getImageData();
	source_data   = (Type *) input->getImageData(ti);

	// 3 scanlines for a 3x3 kernel
	iscan1 = source_data + 1;
	iscan2 = iscan1 + istride;
	iscan3 = iscan2 + istride;

	scan = dest_data;

	for(y=0; y<h; y++)
	{
	    for(x=0; x<w; x++)
	    {
		pixel.set(iscan1[x-1]);
		sum  = (float)pixel * kernel[0];
		
		pixel.set(iscan1[x]);
		sum += (float)pixel * kernel[1];
		
		pixel.set(iscan1[x+1]);
		sum += (float)pixel * kernel[2];

		pixel.set(iscan2[x-1]);
		sum += (float)pixel * kernel[3];
		
		pixel.set(iscan2[x]);
		sum += (float)pixel * kernel[4];
		
		pixel.set(iscan2[x+1]);
		sum += (float)pixel * kernel[5];

		pixel.set(iscan3[x-1]);
		sum += (float)pixel * kernel[6];
		
		pixel.set(iscan3[x]);
		sum += (float)pixel * kernel[7];
		
		pixel.set(iscan3[x+1]);
		sum += (float)pixel * kernel[8];

		// Assign to the output array
		pixel = sum;
		scan[x] = pixel.getValue();
	    }
	    
	    scan += stride;
	    iscan1 += istride;
	    iscan2 += istride;
	    iscan3 += istride;
	}
    }
    return 1;
}

} // namespace HDK_Sample



OP_ERROR
COP2_SampleFilter::doCookMyTile(COP2_Context &context, TIL_TileList *tiles)
{
    // Grab our context data.
    cop2_SampleFilterContext *data =
	static_cast<cop2_SampleFilterContext *>(context.data());

    // Grab the input image data that we need for our tile area.
    TIL_Region *in = inputRegion(0, context,
				 tiles->myX1 -1,
				 tiles->myY1 -1,
				 tiles->myX2 +1,
				 tiles->myY2 +1,
				 TIL_HOLD); // streak edges when outside canvas
    if(!in)
    {
	tiles->clearToBlack();
	return error();
    }

    // call the templated operation
    cop2_EdgeEnhance op(data->myKernel);
    op.filter(tiles, in, context.getTime(), NULL, context.myThreadIndex);

    releaseRegion(in);

    // done - return any errors. 
    return error();
}


void
newCop2Operator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator("hdksamplefilt",
				       "HDK Sample Filter",
				       COP2_SampleFilter::myConstructor,
				       &COP2_SampleFilter::myTemplatePair,
				       1, 
				       2, // optional mask input.
				       &COP2_SampleFilter::myVariablePair,
				       0, // not generator
				       COP2_SampleFilter::myInputLabels));
}


