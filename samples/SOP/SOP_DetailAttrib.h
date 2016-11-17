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

#ifndef __SOP_DetailAttrib__
#define __SOP_DetailAttrib__

#include <SOP/SOP_Node.h>

namespace HDK_Sample {
class SOP_DetailAttrib : public SOP_Node
{
public:
	     SOP_DetailAttrib(OP_Network *net, const char *, OP_Operator *entry);
    virtual ~SOP_DetailAttrib();

    static OP_Node	*myConstructor(OP_Network  *net, const char *name,
				       OP_Operator *entry);
    static PRM_Template	 myTemplateList[];


protected:
    virtual OP_ERROR	 cookMySop   (OP_Context &context);

    void	ATTRIBNAME(UT_String &str, fpreal t)
					{ evalString(str, 0, 0, t); }

    fpreal	VALUE(fpreal t)		{ return evalFloat(1, 0, t); }
};
} // End HDK_Sample namespace

#endif
