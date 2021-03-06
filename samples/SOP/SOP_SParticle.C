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
 * Simple Particle demonstration code.  Includes sample collision detection...
 */

#include "SOP_SParticle.h"

#include <GU/GU_Detail.h>
#include <GU/GU_RayIntersect.h>

#include <GEO/GEO_PrimPart.h>

#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Director.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>

#include <PRM/PRM_Include.h>

#include <UT/UT_DSOVersion.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_Vector4.h>

using namespace HDK_Sample;

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "hdk_sparticle",
        "Simple Particle",
        SOP_SParticle::myConstructor,
        SOP_SParticle::myTemplateList,
        1,      // Min required sources
        2,      // Maximum sources
        0));
}

// The names here have to match the inline evaluation functions
static PRM_Name names[] = {
    PRM_Name("reset", "Reset Frame"),
    PRM_Name("birth", "Birth Rate"),
    PRM_Name("force", "Force"),
};

static PRM_Default	birthRate(10);

PRM_Template
SOP_SParticle::myTemplateList[] = {
    PRM_Template(PRM_INT,	1, &names[0], PRMoneDefaults),
    PRM_Template(PRM_INT_J,	1, &names[1], &birthRate),
    PRM_Template(PRM_XYZ_J,	3, &names[2]),
    PRM_Template(),
};

int *
SOP_SParticle::myOffsets = 0;

OP_Node *
SOP_SParticle::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_SParticle(net, name, op);
}

SOP_SParticle::SOP_SParticle(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op)
    , mySystem(NULL)
{
    // Make sure that our offsets are allocated.  Here we allow up to 32
    // parameters, no harm in over allocating.  The definition for this
    // function is in OP/OP_Parameters.h
    if (!myOffsets)
	myOffsets = allocIndirect(32);

    // Now, flag that nothing has been built yet...
    myVelocity.clear();
}

SOP_SParticle::~SOP_SParticle() {}

void
SOP_SParticle::birthParticle()
{
    // Strictly speaking, we should be using mySource->getPointMap() for the
    // initial invalid point, but mySource may be NULL.
    GA_Offset srcptoff = GA_INVALID_OFFSET;
    GA_Offset vtxoff = mySystem->giveBirth();
    if (mySource)
    {
	if (mySourceNum >= mySource->getPointMap().indexSize())
	    mySourceNum = 0;
	if (mySource->getPointMap().indexSize() > 0) // No points in the input
	    srcptoff = mySource->pointOffset(mySourceNum);
	mySourceNum++; // Move on to the next source point...
    }
    GA_Offset ptoff = gdp->vertexPoint(vtxoff);
    if (GAisValid(srcptoff))
    {
	gdp->setPos3(ptoff, mySource->getPos3(srcptoff));
	if (mySourceVel.isValid())
            myVelocity.set(ptoff, mySourceVel.get(srcptoff));
	else
            myVelocity.set(ptoff, UT_Vector3(0, 0, 0));
    }
    else
    {
        gdp->setPos3(ptoff, SYSdrand48()-.5, SYSdrand48()-.5, SYSdrand48()-.5);
	myVelocity.set(ptoff, UT_Vector3(0, 0, 0));
    }
    // First index of the life variable represents how long the particle has
    // been alive (set to 0).
    myLife.set(ptoff, 0, 0);
    // The second index of the life variable represents how long the particle
    // will live (in frames)
    myLife.set(ptoff, 1, 30+30*SYSdrand48());
}

int
SOP_SParticle::moveParticle(GA_Offset ptoff, const UT_Vector3 &force)
{
    float life = myLife.get(ptoff, 0);
    float death = myLife.get(ptoff, 1);
    life += 1;
    myLife.set(ptoff, life, 0); // Store back in point
    if (life >= death)
        return 0;               // The particle should die!

    float tinc = 1./30.;        // Hardwire 1/30 of a second time inc...

    // Adjust the velocity (based on the force) - of course, the multiplies
    // can be pulled out of the loop...
    UT_Vector3 vel = myVelocity.get(ptoff);
    vel += tinc*force;
    myVelocity.set(ptoff, vel);

    // Now adjust the point positions

    if (myCollision)
    {
	UT_Vector3 dir = vel * tinc;

	// here, we only allow hits within the length of the velocity vector
	GU_RayInfo info(dir.normalize());

	UT_Vector3 start = gdp->getPos3(ptoff);
	if (myCollision->sendRay(start, dir, info) > 0)
	    return 0;	// We hit something, so kill the particle
    }

    UT_Vector3 pos = gdp->getPos3(ptoff);
    pos += tinc*vel;
    gdp->setPos3(ptoff, pos);

    return 1;
}

void
SOP_SParticle::timeStep(fpreal now)
{
    UT_Vector3 force(FX(now), FY(now), FZ(now));
    int nbirth = BIRTH(now);

    if (error() >= UT_ERROR_ABORT)
	return;

    for (int i = 0; i < nbirth; ++i)
	birthParticle();

    for (GA_Size i = 0; i < mySystem->getNumParticles(); i++)
    {
	if (!moveParticle(mySystem->vertexPoint(i), force))
	    mySystem->deadParticle(i);
    }
    mySystem->deleteDead();
}

void
SOP_SParticle::initSystem()
{
    if (!gdp) gdp = new GU_Detail;

    // Check to see if we really need to reset everything
    if (gdp->getPointMap().indexSize() > 0 || myVelocity.isInvalid())
    {
	mySourceNum = 0;
	gdp->clearAndDestroy();
	mySystem = (GEO_PrimParticle *)gdp->appendPrimitive(GEO_PRIMPART);
	mySystem->clearAndDestroy();

	// A vector attribute will be transformed correctly by following
	//	SOPs.  Use float types for stuff like color...
	myVelocity = GA_RWHandleV3(gdp->addFloatTuple(GA_ATTRIB_POINT, "v", 3));
	if (myVelocity.isValid())
	    myVelocity.getAttribute()->setTypeInfo(GA_TYPE_VECTOR);
	myLife = GA_RWHandleF(gdp->addFloatTuple(GA_ATTRIB_POINT, "life", 2));
    }
}

OP_ERROR
SOP_SParticle::cookMySop(OP_Context &context)
{
    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    // Now, indicate that we are time dependent (i.e. have to cook every
    // time the current frame changes).
    OP_Node::flags().timeDep = 1;

    // Channel manager has time info for us
    CH_Manager *chman = OPgetDirector()->getChannelManager();

    // This is the frame that we're cooking at...
    fpreal currframe = chman->getSample(context.getTime());
    fpreal reset = RESET(); // Find our reset frame...

    if (currframe <= reset || !mySystem)
    {
	myLastCookTime = reset;
	initSystem();
    }
    else
    {
	// Set up the collision detection object
	const GU_Detail *collision = inputGeo(1, context);
	if (collision)
	{
	    myCollision = new GU_RayIntersect;
	    myCollision->init(collision);
	}
	else myCollision = 0;

	// Set up our source information...
	mySource = inputGeo(0, context);
	if (mySource)
	{
	    mySourceVel = GA_ROHandleV3(mySource->findFloatTuple(GA_ATTRIB_POINT, "v", 3));

	    // If there's no velocity, pick up the velocity from the normal
	    if (mySourceVel.isInvalid())
		mySourceVel = GA_ROHandleV3(mySource->findFloatTuple(GA_ATTRIB_POINT, "N", 3));
	}

        // This is where we notify our handles (if any) if the inputs have
        // changed.  This is normally done in cookInputGroups, but since there
        // is no input group, NULL is passed as the fourth argument.
        notifyGroupParmListeners(0, -1, mySource, NULL);

	// Now cook the geometry up to our current time
	// Here, we could actually re-cook the source input to get a moving
	// source...  But this is just an example ;-)

	currframe += 0.05;	// Add a bit to avoid floating point error
	while (myLastCookTime < currframe)
	{
	    // Here we have to convert our frame number to the actual time.
	    timeStep(chman->getTime(myLastCookTime));
	    myLastCookTime += 1;
	}

	if (myCollision) delete myCollision;

	// Set the node selection for the generated particles. This will 
	// highlight all the points generated by this node, but only if the 
	// highlight flag is on and the node is selected.
	select(GA_GROUP_POINT);
    }

    return error();
}

const char *
SOP_SParticle::inputLabel(unsigned inum) const
{
    switch (inum)
    {
	case 0: return "Particle Source Geometry";
	case 1:	return "Collision Object";
    }
    return "Unknown source";
}
