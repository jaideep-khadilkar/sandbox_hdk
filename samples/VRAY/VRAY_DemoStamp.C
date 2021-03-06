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
 * This is a sample procedural DSO
 */

#include <UT/UT_DSOVersion.h>
#include <GU/GU_Detail.h>
#include "VRAY_DemoStamp.h"

// The vray_ChildBox is a "private" procedural.
// The user can't allocate one of these directly.
namespace HDK_Sample {
/// @brief Procedural used in @ref VRAY/VRAY_DemoStamp.C to render a box
/// @see @ref VRAY/VRAY_DemoBox.C
class vray_ChildBox : public VRAY_Procedural {
public:
    vray_ChildBox(UT_Vector3 &center, fpreal size)
	: myCenter(center),
	  mySize(size)
    {}
    virtual ~vray_ChildBox() {}

    virtual const char	*className() const	{ return "vray_ChildBox"; }
			 /// Since the procedural is generated by stamp, this
			 /// method will never be called.
    virtual int		 initialize(const UT_BoundingBox *)
			 {
			     return 0;
			 }
    virtual void	 getBoundingBox(UT_BoundingBox &box)
			 {
			     box.initBounds(myCenter);
			     box.expandBounds(0, mySize);
			 }
    virtual void	 render()
			 {
			     VRAY_ProceduralGeo	geo = createGeometry();
			     geo->cube(
				     myCenter.x()-mySize, myCenter.x()+mySize,
				     myCenter.y()-mySize, myCenter.y()+mySize,
				     myCenter.z()-mySize, myCenter.z()+mySize);
			     VRAY_ProceduralChildPtr	child = createChild();
			     child->addGeometry(geo);
			 }
private:
    UT_Vector3	myCenter;
    fpreal	mySize;
};
}	// End HDK_Sample namespace

using namespace HDK_Sample;

// Arguments for the stamp procedural
static VRAY_ProceduralArg	theArgs[] = {
    VRAY_ProceduralArg("minbound",	"real",	"-1 -1 -1"),
    VRAY_ProceduralArg("maxbound",	"real",	"1 1 1"),
    VRAY_ProceduralArg("size",		"real",	".1"),
    VRAY_ProceduralArg("npoints",	"int",	"10"),
    VRAY_ProceduralArg("seed",		"int",	"1"),
    VRAY_ProceduralArg()
};

VRAY_Procedural *
allocProcedural(const char *)
{
    return new VRAY_DemoStamp();
}

const VRAY_ProceduralArg *
getProceduralArgs(const char *)
{
    return theArgs;
}

VRAY_DemoStamp::VRAY_DemoStamp()
{
    myBox.initBounds(0, 0, 0);
}

VRAY_DemoStamp::~VRAY_DemoStamp()
{
}

const char *
VRAY_DemoStamp::className() const
{
    return "VRAY_DemoStamp";
}

int
VRAY_DemoStamp::initialize(const UT_BoundingBox *)
{
    fpreal	val[3];

    // Evaluate parameters and store the information
    val[0] = val[1] = val[2] = -1;
    import("minbound", val, 3);
    myBox.initBounds(val[0], val[1], val[2]);

    val[0] = val[1] = val[2] = 1;
    import("maxbound", val, 3);
    myBox.enlargeBounds(val[0], val[1], val[2]);

    if (!import("size", &mySize, 1))
	mySize = 0.1;
    if (!import("npoints", &myCount, 1))
	myCount = 10;
    if (!import("seed", &mySeed, 1))
	mySeed = 1;

    mySize = SYSabs(mySize);

    return 1;
}

void
VRAY_DemoStamp::getBoundingBox(UT_BoundingBox &box)
{
    // Initialize with the bounding box of the stamp procedural
    box = myBox;

    // Each child box can also enlarge the bounds by the size of the child
    box.expandBounds(0, mySize);
}

void
VRAY_DemoStamp::render()
{
    int			i;
    UT_Vector3		c;
    unsigned int	seed;
    float		cx, cy, cz;

    seed = mySeed;
    for (i = 0; i < myCount; i++)
    {
	// Compute random numbers deterministically.  If they were computed in
	// the assign operation, compilers could compute them in a different
	// order.
	cx = SYSfastRandom(seed);
	cy = SYSfastRandom(seed);
	cz = SYSfastRandom(seed);
	c.assign( SYSfit(cx, 0.0f, 1.0f, myBox.xmin(), myBox.xmax()),
		  SYSfit(cy, 0.0f, 1.0f, myBox.ymin(), myBox.ymax()),
		  SYSfit(cz, 0.0f, 1.0f, myBox.zmin(), myBox.zmax()) );

	// Create a new procedural object
	VRAY_ProceduralChildPtr	obj = createChild();
	// We create the procedural ourselves.
	// Alternatively, it's also possible to create a procedural by
	// passing arguments.  This would allow the user to control which
	// procedural gets built.
	obj->addProcedural( new vray_ChildBox(c, mySize) );
    }
}
