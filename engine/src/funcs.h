/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

//
// MCFunction class declarations
//
#ifndef	FUNCTIONS_H
#define	FUNCTIONS_H

#include "express.h"
#include "executionerrors.h"
#include "parseerrors.h"
#include "mcerror.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

class MCFunction : public MCExpression
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	Parse_stat parsetarget(MCScriptPoint &spt, Boolean the,
	                       Boolean needone, MCChunk *&chunk);
	Exec_stat params_to_doubles(MCExecPoint& ep, MCParameter *p_params, real64_t*& r_doubles, uindex_t& r_count);
	
	// General method for compiling a function that maps to a single method.
	// The variadic argument list should be the MCExpression's the function
	// takes as arguments.
	virtual void compile_with_args(MCSyntaxFactoryRef ctxt, MCExecMethodInfo *method, ...);
};

// Helper class that simplifies compiling of functions not taking any arguments.
class MCConstantFunction: public MCFunction
{
public:
	virtual MCExecMethodInfo *getmethodinfo(void) const = 0;
	virtual void compile(MCSyntaxFactoryRef ctxt);
};

// Helper class that simplifies compiling of functions taking one arguments.
class MCUnaryFunction: public MCFunction
{
public:
	virtual MCExecMethodInfo *getmethodinfo(void) const = 0;
	virtual MCExpression *getmethodarg(void) const = 0;
    virtual void compile(MCSyntaxFactoryRef ctxt);
};

// Helper class that simplifies compiling of functions taking a variable number of of MCParameters.
class MCParamFunction: public MCFunction
{
public:
	virtual MCExecMethodInfo *getmethodinfo(void) const = 0;
	virtual MCParameter *getmethodarg(void) const = 0;
	virtual void compile(MCSyntaxFactoryRef ctxt);
};

////////////////////////////////////////////////////////////////////////////////

// Helper class that simplifies evaluation of functions not taking any arguments.
extern MCError *MCperror;

template<typename ReturnType, void (*EvalFunction)(MCExecContext&, typename MCExecValueTraits<ReturnType>::out_type)> class MCConstantFunctionCtxt: public MCConstantFunction
{
public:
	void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
	{
		ReturnType t_result;
		EvalFunction(ctxt, t_result);
		if (!ctxt . HasError())
			MCExecValueTraits<ReturnType>::set(r_value, t_result);
	}
};

template<typename ParamType,
         typename ReturnType,
         bool (MCExecContext::*EvalExpression)(MCExpression*, Exec_errors, typename MCExecValueTraits<ParamType>::out_type),
         void (*EvalFunction)(MCExecContext&, typename MCExecValueTraits<ParamType>::in_type, typename MCExecValueTraits<ReturnType>::out_type),
         Exec_errors EvalError,
         Parse_errors ParseError,
         MCExecMethodInfo*& MethodInfo>
class MCUnaryFunctionCtxt: public MCUnaryFunction
{
public:
    MCUnaryFunctionCtxt() { m_expression = nil; }

    virtual ~MCUnaryFunctionCtxt() { delete m_expression; }

    virtual Parse_stat parse(MCScriptPoint &sp, Boolean the)
    {
        if (get1param(sp, &m_expression, the) != PS_NORMAL)
        {
            MCperror -> add(ParseError, sp);
            return PS_ERROR;
        }

        return PS_NORMAL;
    }

    void eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
    {
        ReturnType t_result;
        ParamType t_param;

        if (!(ctxt .* EvalExpression)(m_expression, EvalError, t_param))
            return;

        EvalFunction(ctxt, t_param, t_result);

        MCExecValueTraits<ParamType>::free(t_param);

        if (!ctxt . HasError())
            MCExecValueTraits<ReturnType>::set(r_value, t_result);
    }

    virtual MCExecMethodInfo *getmethodinfo(void) const { return MethodInfo; }
    virtual MCExpression *getmethodarg(void) const { return m_expression; }

protected:
    MCExpression *m_expression;
};

////////////////////////////////////////////////////////////////////////////////

class MCArrayDecode: public MCUnaryFunctionCtxt<MCStringRef, MCArrayRef, &MCExecContext::EvalExprAsStringRef, MCArraysEvalArrayDecode, EE_ARRAYDECODE_BADSOURCE, PE_ARRAYDECODE_BADPARAM, kMCArraysEvalArrayDecodeMethodInfo>
{
public:
    MCArrayDecode(){}
    virtual ~MCArrayDecode(){}
};

class MCArrayEncode: public MCUnaryFunctionCtxt<MCArrayRef, MCStringRef, &MCExecContext::EvalExprAsArrayRef, MCArraysEvalArrayEncode, EE_ARRAYENCODE_BADSOURCE, PE_ARRAYENCODE_BADPARAM, kMCArraysEvalArrayEncodeMethodInfo>
{
public:
    MCArrayEncode(){}
    virtual ~MCArrayEncode(){}
};

class MCBase64Decode : public MCUnaryFunctionCtxt<MCStringRef, MCDataRef, &MCExecContext::EvalExprAsStringRef, MCFiltersEvalBase64Decode, EE_BASE64DECODE_BADSOURCE, PE_BASE64DECODE_BADPARAM, kMCFiltersEvalBase64DecodeMethodInfo>
{
public:
    MCBase64Decode() {}
    virtual ~MCBase64Decode(){}
};

class MCBase64Encode : public MCUnaryFunctionCtxt<MCDataRef, MCStringRef, &MCExecContext::EvalExprAsDataRef, MCFiltersEvalBase64Encode, EE_BASE64ENCODE_BADSOURCE, PE_BASE64ENCODE_BADPARAM, kMCFiltersEvalBase64EncodeMethodInfo>
{
	MCExpression *source;
public:
    MCBase64Encode(){}
    virtual ~MCBase64Encode(){}
};

class MCBaseConvert : public MCFunction
{
	MCExpression *source;
	MCExpression *sourcebase;
    MCExpression *destbase;
public:
	MCBaseConvert()
	{
		source = sourcebase = destbase = NULL;
	}
	virtual ~MCBaseConvert();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCBinaryEncode : public MCFunction
{
	MCParameter *params;
public:
	MCBinaryEncode()
	{
		params = NULL;
	}
	virtual ~MCBinaryEncode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCBinaryDecode : public MCFunction
{
	MCParameter *params;
public:
	MCBinaryDecode()
	{
		params = NULL;
	}
	virtual ~MCBinaryDecode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCBuildNumber : public MCConstantFunctionCtxt<integer_t, MCEngineEvalBuildNumber>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalBuildNumberMethodInfo; }
};

class MCCachedUrls : public MCConstantFunctionCtxt<MCStringRef, MCNetworkEvalCachedUrls>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalCachedUrlsMethodInfo; }
};

class MCCapsLockKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalCapsLockKey>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalCapsLockKeyMethodInfo; }
};

class MCCharToNum : public MCUnaryFunctionCtxt<MCStringRef, MCValueRef, &MCExecContext::EvalExprAsStringRef, MCStringsEvalCharToNum, EE_CHARTONUM_BADSOURCE, PE_CHARTONUM_BADPARAM, kMCStringsEvalCharToNumMethodInfo>
{
public:
    MCCharToNum(){}
    virtual ~MCCharToNum(){}
};

class MCByteToNum : public MCUnaryFunctionCtxt<MCStringRef, integer_t, &MCExecContext::EvalExprAsStringRef, MCStringsEvalByteToNum, EE_BYTETONUM_BADSOURCE, PE_BYTETONUM_BADPARAM, kMCStringsEvalByteToNumMethodInfo>
{
public:
    MCByteToNum(){}
    virtual ~MCByteToNum(){}
};

class MCChunkOffset : public MCFunction
{
	MCExpression *part;
	MCExpression *whole;
	MCExpression *offset;
protected:
	Chunk_term delimiter;
public:
	MCChunkOffset()
	{
		part = whole = offset = NULL;
	}
	virtual ~MCChunkOffset();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCClipboard : public MCConstantFunctionCtxt<MCNameRef, MCPasteboardEvalClipboard>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCPasteboardEvalClipboardMethodInfo; }
};

class MCCompress : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCCompress()
	{
		source = NULL;
	}
	virtual ~MCCompress();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalCompressMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCConstantNames : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalConstantNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalConstantNamesMethodInfo; }

};

class MCItemOffset : public MCChunkOffset
{
public:
	MCItemOffset()
	{
		delimiter = CT_ITEM;
	}
};

class MCLineOffset : public MCChunkOffset
{
public:
	MCLineOffset()
	{
		delimiter = CT_LINE;
	}
};

class MCOffset : public MCChunkOffset
{
public:
	MCOffset()
	{
		delimiter = CT_CHARACTER;
	}
};

class MCWordOffset : public MCChunkOffset
{
public:
	MCWordOffset()
	{
		delimiter = CT_WORD;
	}
};

class MCClickChar : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickChar>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickCharMethodInfo; }
};

class MCClickCharChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickCharChunk>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickCharChunkMethodInfo; }
};

class MCClickChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickChunk>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickChunkMethodInfo; }
};

class MCClickField : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickField>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickFieldMethodInfo; }
};

class MCClickH : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalClickH>
{
public:
	//virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickHMethodInfo; }
};

class MCClickLine : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickLine>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickLineMethodInfo; }
};

class MCClickLoc : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickLoc>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickLocMethodInfo; }
};

class MCClickStack : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickStack>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickStackMethodInfo; }
};

class MCClickText : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalClickText>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickTextMethodInfo; }
};

class MCClickV : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalClickV>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalClickVMethodInfo; }
};

class MCColorNames : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalColorNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalColorNamesMethodInfo; }

};

class MCCommandNames : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalCommandNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalCommandNamesMethodInfo; }

};

class MCCommandKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalCommandKey>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalCommandKeyMethodInfo; }
};

class MCControlKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalControlKey>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalControlKeyMethodInfo; }
};

class MCDate : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalDate>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalDateMethodInfo; }

};

class MCDateFormat : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalDateFormat>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalDateFormatMethodInfo; }

};

class MCDecompress : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCDecompress()
	{
		source = NULL;
	}
	virtual ~MCDecompress();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	static Exec_stat do_decompress(MCExecPoint& ep, uint2, uint2);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalDecompressMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCDirectories : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalDirectories>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalDirectoriesMethodInfo; }

};

class MCDiskSpace : public MCConstantFunctionCtxt<double, MCFilesEvalDiskSpace>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalDiskSpaceMethodInfo; }

};

class MCDNSServers : public MCConstantFunctionCtxt<MCStringRef, MCNetworkEvalDNSServers>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalDNSServersMethodInfo; }

};

class MCDragDestination: public MCConstantFunctionCtxt<MCStringRef, MCPasteboardEvalDragDestination>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCPasteboardEvalDragDestinationMethodInfo; }

};

class MCDragSource: public MCConstantFunctionCtxt<MCStringRef, MCPasteboardEvalDragSource>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCPasteboardEvalDragSourceMethodInfo; }

};

class MCDriverNames : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalDriverNames>
{
	MCExpression *type;
public:
	MCDriverNames()
	{
		type = NULL;
	}
	virtual ~MCDriverNames();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalDriverNamesMethodInfo; }

};

class MCDrives : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalDrives>
{
	MCExpression *type;
public:
	MCDrives()
	{
		type = NULL;
	}
	virtual ~MCDrives();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalDrivesMethodInfo; }

};

class MCDropChunk: public MCConstantFunctionCtxt<MCStringRef, MCPasteboardEvalDropChunk>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCPasteboardEvalDropChunkMethodInfo; }

};

class MCQTEffects : public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalQTEffects>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalQTEffectsMethodInfo; }

};

class MCRecordCompressionTypes: public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalRecordCompressionTypes>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalRecordCompressionTypesMethodInfo; }

};

class MCRecordLoudness : public MCConstantFunctionCtxt<integer_t, MCMultimediaEvalRecordLoudness>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalRecordLoudnessMethodInfo; }

};

class MCEnvironment : public MCConstantFunctionCtxt<MCNameRef, MCEngineEvalEnvironment>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalEnvironmentMethodInfo; }

};

class MCEncrypt : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCEncrypt()
	{
		source = NULL;
	}
	virtual ~MCEncrypt();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCSecurityEvalEncryptMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCExists : public MCFunction
{
	MCChunk *object;
public:
	MCExists()
	{
		object = NULL;
	}
	virtual ~MCExists();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual void compile(MCSyntaxFactoryRef);
};

class MCExtents : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCExtents()
	{
		source = NULL;
	}
	virtual ~MCExtents();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCArraysEvalExtentsMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCTheFiles : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalFiles>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalFilesMethodInfo; }

};

class MCFlushEvents : public MCUnaryFunction
{
	MCExpression *type;
public:
	MCFlushEvents()
	{
		type = NULL;
	}
	virtual ~MCFlushEvents();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFlushEventsMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return type; }
};

class MCFocusedObject : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFocusedObject>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFocusedObjectMethodInfo; }

};

class MCFontNames : public MCUnaryFunction
{
	MCExpression *type;
public:
	MCFontNames()
	{
		type = NULL;
	}
	virtual ~MCFontNames();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCTextEvalFontNamesMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return type; }
};

class MCFontLanguage : public MCUnaryFunction
{
	MCExpression *fontname;
public:
	MCFontLanguage()
	{
		fontname = NULL;
	}
	virtual ~MCFontLanguage();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCTextEvalFontLanguageMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return fontname; }
};


class MCFontSizes : public MCUnaryFunction
{
	MCExpression *fontname;
public:
	MCFontSizes()
	{
		fontname = NULL;
	}
	virtual ~MCFontSizes();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCTextEvalFontSizesMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return fontname; }
};

class MCFontStyles : public MCFunction
{
	MCExpression *fontname;
	MCExpression *fontsize;
public:
	MCFontStyles()
	{
		fontname = fontsize = NULL;
	}
	virtual ~MCFontStyles();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	
	virtual void compile(MCSyntaxFactoryRef);
};

class MCFormat : public MCFunction
{
	MCParameter *params;
public:
	MCFormat()
	{
		params = NULL;
	}
	virtual ~MCFormat();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCFoundChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundChunk>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundChunkMethodInfo; }

};

class MCFoundField : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundField>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundFieldMethodInfo; }
};

class MCFoundLine : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundLine>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundLineMethodInfo; }

};

class MCFoundLoc : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundLoc>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundLocMethodInfo; }

};

class MCFoundText : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalFoundText>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalFoundTextMethodInfo; }


};

class MCFunctionNames : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalFunctionNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalFunctionNamesMethodInfo; }
};

class MCGlobalLoc : public MCUnaryFunction
{
	MCExpression *point;
public:
	MCGlobalLoc()
	{
		point = NULL;
	}
	virtual ~MCGlobalLoc();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalGlobalLocMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return point; }
};

class MCGlobals : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalGlobalNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalGlobalNamesMethodInfo; }

};

class MCHasMemory : public MCUnaryFunction
{
	MCExpression *amount;
public:
	MCHasMemory()
	{
		amount = NULL;
	}
	virtual ~MCHasMemory();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalHasMemoryMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return amount; }
};

class MCHeapSpace : public MCConstantFunctionCtxt<integer_t, MCLegacyEvalHeapSpace>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalHeapSpaceMethodInfo; }

};

class MCHostAddress : public MCUnaryFunction
{
	MCExpression *socket;
public:
	MCHostAddress()
	{
		socket = NULL;
	}
	virtual ~MCHostAddress();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalHostAddressMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return socket; }
};

class MCHostAtoN : public MCUnaryFunction
{
	MCExpression *address;
public:
	MCHostAtoN()
	{
		address = NULL;
	}
	virtual ~MCHostAtoN();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalHostAddressToNameMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return address; }
};

class MCHostName : public MCConstantFunctionCtxt<MCStringRef, MCNetworkEvalHostName>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalHostNameMethodInfo; }

};

class MCHostNtoA : public MCFunction
{
	MCExpression *name;
	MCExpression *message;
public:
	MCHostNtoA()
	{
		name = NULL;
		message = NULL;
	}
	virtual ~MCHostNtoA();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual void compile(MCSyntaxFactoryRef);
};

class MCInsertScripts : public MCConstantFunction
{
protected:
	Boolean front;
public:
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return front == True ? kMCEngineEvalFrontScriptsMethodInfo : kMCEngineEvalBackScriptsMethodInfo; }
};

class MCBackScripts : public MCInsertScripts
{
public:
	MCBackScripts()
	{
		front = False;
	}
};

class MCFrontScripts : public MCInsertScripts
{
public:
	MCFrontScripts()
	{
		front = True;
	}
};

class MCInterrupt : public MCConstantFunctionCtxt<bool, MCEngineEvalInterrupt>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalInterruptMethodInfo; }

};

class MCIntersect : public MCFunction
{
	MCChunk *o1;
	MCChunk *o2;
	// MW-2011-09-23: [[ Collision ]] The threshold value to use for alpha -> sharp conversion.
	MCExpression *threshold;
public:
	MCIntersect()
	{
		o1 = o2 = NULL;
		threshold = NULL;
	}
	virtual ~MCIntersect();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCIsoToMac : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCIsoToMac()
	{
		source = NULL;
	}
	virtual ~MCIsoToMac();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalIsoToMacMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCIsNumber : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCIsNumber()
	{
		source = NULL;
	}
	virtual ~MCIsNumber();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalIsNumberMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCKeys : public MCFunction
{
	MCExpression *source;
	Properties which;
public:
	MCKeys()
	{
		source = NULL;
		which = P_UNDEFINED;
	}
	virtual ~MCKeys();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual void compile(MCSyntaxFactoryRef);
};

class MCKeysDown : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalKeysDown>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalKeysDownMethodInfo; }

};

class MCLength : public MCUnaryFunctionCtxt<MCStringRef, integer_t, &MCExecContext::EvalExprAsStringRef, MCStringsEvalLength, EE_LENGTH_BADSOURCE, PE_LENGTH_BADPARAM, kMCStringsEvalLengthMethodInfo>
{
public:
    MCLength(){}
    virtual ~MCLength(){}
};

class MCLicensed : public MCConstantFunctionCtxt<bool, MCLegacyEvalLicensed>
{
	MCExpression *source;
public:
	MCLicensed()
	{
		source = NULL;
	}
	virtual ~MCLicensed();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalLicensedMethodInfo; }
};

class MCLocalLoc : public MCUnaryFunction
{
	MCExpression *point;
public:
	MCLocalLoc()
	{
		point = NULL;
	}
	virtual ~MCLocalLoc();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalLocalLocMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return point; }
};

class MCLocals : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalLocalNames>
{
	MCHandler *h;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalLocalNamesMethodInfo; }
};

class MCMachine : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalMachine>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalMachineMethodInfo; }
};

class MCMacToIso : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCMacToIso()
	{
		source = NULL;
	}
	virtual ~MCMacToIso();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalMacToIsoMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCMainStacks : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMainStacks>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMainStacksMethodInfo; }
};

class MCMatch : public MCFunction
{
	MCParameter *params;
protected:
	Boolean chunk;
public:
	MCMatch()
	{
		params = NULL;
	}
	virtual ~MCMatch();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCMatchChunk : public MCMatch
{
public:
	MCMatchChunk()
	{
		chunk = True;
	}
};

class MCMatchText : public MCMatch
{
public:
	MCMatchText()
	{
		chunk = False;
	}
};


class MCMe : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalMe>
{
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalMeMethodInfo; }
};

class MCMenuObject : public MCConstantFunctionCtxt<MCStringRef, MCLegacyEvalMenuObject>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalMenuObjectMethodInfo; }
};

class MCMenus : public MCConstantFunctionCtxt<MCStringRef, MCLegacyEvalMenus>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalMenusMethodInfo; }
};

class MCMerge : public MCUnaryFunction
{
public:
	MCHandler *h;
	MCExpression *source;
public:
	MCMerge()
	{
		source = NULL;
	}
	virtual ~MCMerge();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	Exec_stat doscript(MCExecPoint &);
	void deletestatements(MCStatement *statements);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCStringsEvalMergeMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCMillisecs : public MCConstantFunctionCtxt<double, MCDateTimeEvalMilliseconds>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalMillisecondsMethodInfo; }
};

class MCMonthNames : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalMonthNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalMonthNamesMethodInfo; }
};

class MCMouse : public MCFunction
{
	MCExpression *which;
public:
	MCMouse()
	{
		which = NULL;
	}
	virtual ~MCMouse();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCMouseChar : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseChar>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseCharMethodInfo; }
};

class MCMouseCharChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseCharChunk>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseCharChunkMethodInfo; }
};

class MCMouseChunk : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseChunk>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseChunkMethodInfo; }
};

class MCMouseClick : public MCConstantFunctionCtxt<bool, MCInterfaceEvalMouseClick>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseClickMethodInfo; }
};

class MCMouseColor : public MCConstantFunctionCtxt<MCColor, MCInterfaceEvalMouseColor>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseColorMethodInfo; }
};

class MCMouseControl : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseControl>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseControlMethodInfo; }
};

class MCMouseH : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalMouseH>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseHMethodInfo; }
};

class MCMouseLine : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseLine>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseLineMethodInfo; }
};

class MCMouseLoc : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseLoc>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseLocMethodInfo; }
};

class MCMouseStack : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseStack>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseStackMethodInfo; }
};

class MCMouseText : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMouseText>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseTextMethodInfo; }
};

class MCMouseV : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalMouseV>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMouseVMethodInfo; }
};

class MCMovie : public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalMovie>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalMovieMethodInfo; }
};

class MCMovingControls : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalMovingControls>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalMovingControlsMethodInfo; }
};

class MCNumToChar: public MCFunction
{
	MCExpression *source;
public:
	MCNumToChar()
	{
		source = NULL;
	}
	virtual ~MCNumToChar();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCStringsEvalNumToCharMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCNumToByte: public MCFunction
{
	MCExpression *source;
public:
	MCNumToByte(void)
	{
		source = NULL;
	}
	virtual ~MCNumToByte(void);
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCStringsEvalNumToByteMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCOpenFiles : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalOpenFiles>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalOpenFilesMethodInfo; }
};

class MCOpenProcesses : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalOpenProcesses>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalOpenProcessesMethodInfo; }
};

class MCOpenProcessIds : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalOpenProcessesIds>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalOpenProcessesIdsMethodInfo; }
};

class MCOpenSockets : public MCConstantFunctionCtxt<MCStringRef, MCNetworkEvalOpenSockets>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalOpenSocketsMethodInfo; }
};

class MCOpenStacks : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalOpenStacks>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalOpenStacksMethodInfo; }
};

class MCOptionKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalOptionKey>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalOptionKeyMethodInfo; }
};

class MCParam : public MCUnaryFunction
{
	MCHandler *h;
	MCExpression *source;
public:
	MCParam()
	{
		source = NULL;
	}
	virtual ~MCParam();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalParamMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCParamCount : public MCConstantFunctionCtxt<integer_t, MCEngineEvalParamCount>
{
	MCHandler *h;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalParamCountMethodInfo; }
};

class MCParams : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalParams>
{
	MCHandler *h;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalParamsMethodInfo; }
};

class MCPeerAddress : public MCUnaryFunction
{
	MCExpression *socket;
public:
	MCPeerAddress()
	{
		socket = NULL;
	}
	virtual ~MCPeerAddress();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalPeerAddressMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return socket; }
};

class MCPendingMessages : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalPendingMessages>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalPendingMessagesMethodInfo; }
};

class MCPid : public MCConstantFunctionCtxt<integer_t, MCFilesEvalProcessId>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalProcessIdMethodInfo; }
};

class MCPlatform : public MCConstantFunctionCtxt<MCNameRef, MCEngineEvalPlatform>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalPlatformMethodInfo; }
};


// JS-2013-06-19: [[ StatsFunctions ]] Definition of populationStdDev
class MCPopulationStdDev : public MCFunction
{
	MCParameter *params;
public:
	MCPopulationStdDev()
	{
		params = NULL;
	}
	virtual ~MCPopulationStdDev();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of populationVariance
class MCPopulationVariance : public MCFunction
{
	MCParameter *params;
public:
	MCPopulationVariance()
	{
		params = NULL;
	}
	virtual ~MCPopulationVariance();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

class MCProcessor : public MCConstantFunction
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalProcessorMethodInfo; }
};

class MCPropertyNames : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalPropertyNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalPropertyNamesMethodInfo; }
};

class MCQTVersion : public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalQTVersion>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalQTVersionMethodInfo; }
};

class MCReplaceText : public MCFunction
{
	MCExpression *source;
	MCExpression *pattern;
	MCExpression *replacement;
public:
	MCReplaceText()
	{
		source = pattern = replacement = NULL;
	}
	virtual ~MCReplaceText();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCTheResult : public MCConstantFunctionCtxt<MCValueRef, MCEngineEvalResult>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalResultMethodInfo; }
};

class MCScreenColors : public MCConstantFunctionCtxt<double, MCInterfaceEvalScreenColors>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalScreenColorsMethodInfo; }
};

class MCScreenDepth : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalScreenDepth>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalScreenDepthMethodInfo; }
};

class MCScreenLoc : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalScreenLoc>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalScreenLocMethodInfo; }
};

class MCScreenName : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalScreenName>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalScreenNameMethodInfo; }
};

class MCScreenRect : public MCFunction
{
	bool f_plural;
public:
	MCScreenRect(bool p_plural)
		: f_plural(p_plural)
	{
	}

	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);

	static void evaluate(MCExecPoint&, bool working, bool plural, bool effective);
};

class MCScreenType : public MCConstantFunctionCtxt<MCNameRef, MCLegacyEvalScreenType>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalScreenTypeMethodInfo; }
};

class MCScreenVendor : public MCConstantFunctionCtxt<MCNameRef, MCLegacyEvalScreenVendor>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalScreenVendorMethodInfo; }
};

class MCScriptLimits : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalScriptLimits>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalScriptLimitsMethodInfo; }
};

class MCSeconds : public MCConstantFunctionCtxt<double, MCDateTimeEvalSeconds>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalSecondsMethodInfo; }
};

class MCSelectedButton : public MCFunction
{
	MCExpression *family;
	MCChunk *object;
	Boolean bg;
public:
	MCSelectedButton()
	{
		family = NULL;
		object = NULL;
		bg = False;
	}
	virtual ~MCSelectedButton();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSelectedChunk : public MCFunction
{
	MCChunk *object;
public:
	MCSelectedChunk()
	{
		object = NULL;
	}
	virtual ~MCSelectedChunk();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSelectedField : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalSelectedField>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalSelectedFieldMethodInfo; }
};

class MCSelectedImage : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalSelectedImage>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalSelectedImageMethodInfo; }
};

class MCSelectedLine : public MCFunction
{
	MCChunk *object;
public:
	MCSelectedLine()
	{
		object = NULL;
	}
	virtual ~MCSelectedLine();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSelectedLoc : public MCFunction
{
	MCChunk *object;
public:
	MCSelectedLoc()
	{
		object = NULL;
	}
	virtual ~MCSelectedLoc();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSelectedObject : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalSelectedObject>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalSelectedObjectMethodInfo; }
};

class MCSelectedText : public MCFunction
{
	MCChunk *object;
public:
	MCSelectedText()
	{
		object = NULL;
	}
	virtual ~MCSelectedText();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCShell : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCShell()
	{
		source = NULL;
	}
	virtual ~MCShell();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalShellMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCShiftKey : public MCConstantFunctionCtxt<MCNameRef, MCInterfaceEvalShiftKey>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalShiftKeyMethodInfo; }
};

class MCSound : public MCConstantFunctionCtxt<MCStringRef, MCMultimediaEvalSound>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalSoundMethodInfo; }
};

class MCStacks : public MCConstantFunctionCtxt<MCStringRef, MCInterfaceEvalStacks>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalStacksMethodInfo; }
};

class MCStackSpace : public MCConstantFunctionCtxt<integer_t, MCLegacyEvalStackSpace>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCLegacyEvalStackSpaceMethodInfo; }
};

class MCSysError : public MCConstantFunctionCtxt<uinteger_t, MCEngineEvalSysError>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalSysErrorMethodInfo; }
};

class MCSystemVersion : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalSystemVersion>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalSystemVersionMethodInfo; }
};

class MCTarget : public MCConstantFunction
{
	Boolean contents;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return contents == True ? kMCEngineEvalTargetContentsMethodInfo : kMCEngineEvalTargetMethodInfo; }
};

// MW-2008-11-05: [[ Owner Reference ]] The owner function syntax class.
class MCOwner : public MCFunction
{
	MCChunk *object;
public:
	MCOwner(void)
	{
		object = NULL;
	}
	~MCOwner(void);
	virtual Parse_stat parse(MCScriptPoint&, Boolean the);
	virtual Exec_stat eval(MCExecPoint&);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCTempName : public MCConstantFunctionCtxt<MCStringRef, MCFilesEvalTempName>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalTempNameMethodInfo; }
};

class MCTicks : public MCConstantFunctionCtxt<double, MCDateTimeEvalTicks>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalTicksMethodInfo; }
};

class MCTheTime : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalTime>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalTimeMethodInfo; }
};

class MCToLower : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCToLower()
	{
		source = NULL;
	}
	virtual ~MCToLower();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCStringsEvalToLowerMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCToUpper : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCToUpper()
	{
		source = NULL;
	}
	virtual ~MCToUpper();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCStringsEvalToUpperMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCTopStack : public MCFunction
{
	MCExpression *which;
public:
	MCTopStack()
	{
		which = NULL;
	}
	virtual ~MCTopStack();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCUniDecode : public MCFunction
{
	MCExpression *source;
	MCExpression *language;
public:
	MCUniDecode()
	{
		source = NULL;
		language = NULL;
	}
	virtual ~MCUniDecode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCUniEncode : public MCFunction
{
	MCExpression *source;
	MCExpression *language;
public:
	MCUniEncode()
	{
		source = NULL;
		language = NULL;
	}
	virtual ~MCUniEncode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCUrlDecode : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCUrlDecode()
	{
		source = NULL;
	}
	virtual ~MCUrlDecode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalUrlDecodeMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCUrlEncode : public MCUnaryFunction
{
	MCExpression *source;
public:
	MCUrlEncode()
	{
		source = NULL;
	}
	virtual ~MCUrlEncode();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFiltersEvalUrlEncodeMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return source; }
};

class MCUrlStatus : public MCUnaryFunction
{
	MCExpression *url;
public:
	MCUrlStatus()
	{
		url = NULL;
	}
	virtual ~MCUrlStatus();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCNetworkEvalUrlStatusMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return url; }
};

class MCValue : public MCFunction
{
	MCHandler *h;
	MCExpression *source;
	MCChunk *object;
public:
	MCValue()
	{
		source = NULL;
		object = NULL;
	}
	virtual ~MCValue();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCVariables : public MCConstantFunctionCtxt<MCStringRef, MCEngineEvalVariableNames>
{
	MCHandler *h;
public:
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalVariableNamesMethodInfo; }
};

class MCVersion : public MCConstantFunctionCtxt<MCNameRef, MCEngineEvalVersion>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCEngineEvalVersionMethodInfo; }
};

class MCWaitDepth : public MCConstantFunctionCtxt<integer_t, MCInterfaceEvalWaitDepth>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCInterfaceEvalWaitDepthMethodInfo; }
};

class MCWeekDayNames : public MCConstantFunctionCtxt<MCStringRef, MCDateTimeEvalWeekDayNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCDateTimeEvalWeekDayNamesMethodInfo; }
};

class MCWithin : public MCFunction
{
	MCChunk *object;
	MCExpression *point;
public:
	MCWithin()
	{
		object = NULL;
		point = NULL;
	}
	virtual ~MCWithin();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

// platform specific functions in funcs.cpp

class MCMCISendString : public MCUnaryFunction
{
	MCExpression *string;
public:
	MCMCISendString()
	{
		string = NULL;
	}
	virtual ~MCMCISendString();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMultimediaEvalMCISendStringMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return string; }
};

class MCDeleteRegistry : public MCUnaryFunction
{
	MCExpression *key;
public:
	MCDeleteRegistry()
	{
		key = NULL;
	}
	virtual ~MCDeleteRegistry();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalDeleteRegistryMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return key; }
};

class MCListRegistry : public MCUnaryFunction
{
	MCExpression *key;
public:
	MCListRegistry()
	{
		key = NULL;
	}
	virtual ~MCListRegistry();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalListRegistryMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return key; }
};

class MCQueryRegistry : public MCFunction
{
	MCExpression *key;
	MCExpression *type;
public:
	MCQueryRegistry()
	{
		key = type = NULL;
	}
	virtual ~MCQueryRegistry();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSetRegistry : public MCFunction
{
	MCExpression *key;
	MCExpression *value;
	MCExpression *type;
public:
	MCSetRegistry()
	{
		key = value = type = NULL;
	}
	virtual ~MCSetRegistry();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCCopyResource : public MCFunction
{
	MCExpression *source;
	MCExpression *dest;
	MCExpression *type;
	MCExpression *name;
	MCExpression *newid;
public:
	MCCopyResource()
	{
		source = dest = type = name = newid = NULL;
	}
	virtual ~MCCopyResource();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCDeleteResource : public MCFunction
{
	MCExpression *source;
	MCExpression *type;
	MCExpression *name;
public:
	MCDeleteResource()
	{
		source = type = name = NULL;
	}
	virtual ~MCDeleteResource();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCGetResource : public MCFunction
{
	MCExpression *source;
	MCExpression *type;
	MCExpression *name;
public:
	MCGetResource()
	{
		source = type = name = NULL;
	}
	virtual ~MCGetResource();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCGetResources : public MCFunction
{
	MCExpression *source;
	MCExpression *type;
public:
	MCGetResources()
	{
		source = type = NULL;
	}
	virtual ~MCGetResources();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSetResource : public MCFunction
{
	MCExpression *source;
	MCExpression *type;
	MCExpression *id;
	MCExpression *name;
	MCExpression *flags;
	MCExpression *value;
public:
	MCSetResource()
	{
		source = type = id = name = flags = value = NULL;
	}
	virtual ~MCSetResource();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSpecialFolderPath : public MCUnaryFunction
{
	MCExpression *type;
public:
	MCSpecialFolderPath()
	{
		type = NULL;
	}
	virtual ~MCSpecialFolderPath();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalSpecialFolderPathMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return type; }
};

class MCShortFilePath : public MCUnaryFunction
{
	MCExpression *type;
public:
	MCShortFilePath()
	{
		type = NULL;
	}
	virtual ~MCShortFilePath();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalShortFilePathMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return type; }
};

class MCLongFilePath : public MCUnaryFunction
{
	MCExpression *type;
public:
	MCLongFilePath()
	{
		type = NULL;
	}
	virtual ~MCLongFilePath();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalLongFilePathMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return type; }
};

class MCAlternateLanguages : public MCConstantFunctionCtxt<MCStringRef, MCScriptingEvalAlternateLanguages>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCScriptingEvalAlternateLanguagesMethodInfo; }
};

class MCAliasReference: public MCUnaryFunction
{
	MCExpression *type;
public:
	MCAliasReference()
	{
		type = NULL;
	}
	virtual ~MCAliasReference();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCFilesEvalAliasReferenceMethodInfo; }
	virtual MCExpression *getmethodarg(void) const { return type; }
};

class MCCipherNames : public MCConstantFunctionCtxt<MCStringRef, MCSecurityEvalCipherNames>
{
public:
	// virtual Exec_stat eval(MCExecPoint &);
	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCSecurityEvalCipherNamesMethodInfo; }
};

// Math functions in funcsm.cpp

class MCAbsFunction : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalAbs, EE_ABS_BADSOURCE, PE_ABS_BADPARAM, kMCMathEvalAbsMethodInfo>
{
public:
	MCAbsFunction(){}
    virtual ~MCAbsFunction(){}
};

class MCAcos : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalAcos, EE_ACOS_BADSOURCE, PE_ACOS_BADPARAM, kMCMathEvalAcosMethodInfo>
{
	public:
	MCAcos(){}	
	virtual ~MCAcos(){}
};

class MCAnnuity : public MCFunction
{
	MCExpression *rate;
	MCExpression *periods;
public:
	MCAnnuity()
	{
		rate = periods = NULL;
	}
	virtual ~MCAnnuity();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of arithmeticMean
class MCArithmeticMean : public MCFunction
{
	MCParameter *params;
public:
	MCArithmeticMean()
	{
		params = NULL;
	}
	virtual ~MCArithmeticMean();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

class MCAsin : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalAsin, EE_ASIN_BADSOURCE, PE_ASIN_BADPARAM, kMCMathEvalAsinMethodInfo>
{
public:
	MCAsin(){}
	virtual ~MCAsin(){}
};

class MCAtan : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalAtan, EE_ATAN_BADSOURCE, PE_ATAN_BADPARAM, kMCMathEvalAtanMethodInfo>
{
public:
	MCAtan(){}
	virtual ~MCAtan(){}
};

class MCAtan2 : public MCFunction
{
	MCExpression *s1;
	MCExpression *s2;
public:
	MCAtan2()
	{
		s1 = s2 = NULL;
	}
	virtual ~MCAtan2();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of averageDev (was average)
class MCAvgDev : public MCFunction
{
	MCParameter *params;
public:
	MCAvgDev()
	{
		params = NULL;
	}
	virtual ~MCAvgDev();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalAverageMethodInfo; }
	virtual MCParameter *getmethodarg(void) const { return params; }
};

class MCCompound : public MCFunction
{
	MCExpression *rate;
	MCExpression *periods;
public:
	MCCompound()
	{
		rate = periods = NULL;
	}
	virtual ~MCCompound();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCCos : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalCos, EE_COS_BADSOURCE, PE_COS_BADPARAM, kMCMathEvalCosMethodInfo>
{
public:
	MCCos(){}
	virtual ~MCCos(){}
};

class MCExp : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalExp, EE_EXP_BADSOURCE, PE_EXP_BADPARAM, kMCMathEvalExpMethodInfo>
{
public:
	MCExp(){}
	virtual ~MCExp(){}
};

class MCExp1 : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalExp1, EE_EXP1_BADSOURCE, PE_EXP1_BADPARAM, kMCMathEvalExp1MethodInfo>
{
public:
	MCExp1(){}
	virtual ~MCExp1(){}
};

class MCExp2 : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalExp2, EE_EXP2_BADSOURCE, PE_EXP2_BADPARAM, kMCMathEvalExp2MethodInfo>
{
public:
	MCExp2(){}
	virtual ~MCExp2(){}
};

class MCExp10 : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalExp10, EE_EXP10_BADSOURCE, PE_EXP10_BADPARAM, kMCMathEvalExp10MethodInfo>
{
public:
	MCExp10(){}
	virtual ~MCExp10(){}
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of geometricMean
class MCGeometricMean : public MCFunction
{
	MCParameter *params;
public:
	MCGeometricMean()
	{
		params = NULL;
	}
	virtual ~MCGeometricMean();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of harmonicMean
class MCHarmonicMean : public MCFunction
{
	MCParameter *params;
public:
	MCHarmonicMean()
	{
		params = NULL;
	}
	virtual ~MCHarmonicMean();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

class MCLn : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalLn, EE_LN_BADSOURCE, PE_LN_BADPARAM, kMCMathEvalLnMethodInfo>
{
public:
	MCLn(){}
	virtual ~MCLn(){}
};

class MCLn1 : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalLn1, EE_LN1_BADSOURCE, PE_LN1_BADPARAM, kMCMathEvalLn1MethodInfo>
{
public:
	MCLn1(){}
	virtual ~MCLn1(){}
};

class MCLog2 : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalLog2, EE_LOG2_BADSOURCE, PE_LOG2_BADPARAM, kMCMathEvalLog2MethodInfo>
{
public:
	MCLog2(){}
	virtual ~MCLog2(){}
};

class MCLog10 : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalLog10, EE_LOG10_BADSOURCE, PE_LOG10_BADPARAM, kMCMathEvalLog10MethodInfo>
{
public:
	MCLog10(){}
	virtual ~MCLog10(){}
};

class MCMatrixMultiply : public MCFunction
{
	MCExpression *dest;
	MCExpression *source;
public:
	MCMatrixMultiply()
	{
		source = NULL;
		dest = NULL;
	}
	virtual ~MCMatrixMultiply();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCMaxFunction : public MCParamFunction
{
	MCParameter *params;
public:
	MCMaxFunction()
	{
		params = NULL;
	}
	virtual ~MCMaxFunction();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalMaxMethodInfo; }
	virtual MCParameter *getmethodarg(void) const { return params; }
};

class MCMedian : public MCParamFunction
{
	MCParameter *params;
public:
	MCMedian()
	{
		params = NULL;
	}
	virtual ~MCMedian();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalMedianMethodInfo; }
	virtual MCParameter *getmethodarg(void) const { return params; }
};

class MCMD5Digest : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, &MCExecContext::EvalExprAsDataRef, MCFiltersEvalMD5Digest, EE_MD5DIGEST_BADSOURCE, PE_MD5DIGEST_BADPARAM, kMCFiltersEvalMD5DigestMethodInfo>
{
public:
	MCMD5Digest(){}
	virtual ~MCMD5Digest(){}
};

class MCSHA1Digest : public MCUnaryFunctionCtxt<MCDataRef, MCDataRef, &MCExecContext::EvalExprAsDataRef, MCFiltersEvalSHA1Digest, EE_SHA1DIGEST_BADSOURCE, PE_SHA1DIGEST_BADPARAM, kMCFiltersEvalSHA1DigestMethodInfo>
{
public:
	MCSHA1Digest(){}
	virtual ~MCSHA1Digest(){}
};

class MCMinFunction : public MCParamFunction
{
	MCParameter *params;
public:
	MCMinFunction()
	{
		params = NULL;
	}
	virtual ~MCMinFunction();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalMinMethodInfo; }
	virtual MCParameter *getmethodarg(void) const { return params; }
};

class MCRandom : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalRandom, EE_RANDOM_BADSOURCE, PE_RANDOM_BADPARAM, kMCMathEvalRandomMethodInfo>
{
public:
	MCRandom(){}
	virtual ~MCRandom(){}
};

class MCRound : public MCFunction
{
	MCExpression *source;
	MCExpression *digit;
public:
	MCRound()
	{
		source = digit = NULL;
	}
	virtual ~MCRound();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCSin : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalSin, EE_SIN_BADSOURCE, PE_SIN_BADPARAM, kMCMathEvalSinMethodInfo>
{
public:
	MCSin(){}
	virtual ~MCSin(){}
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of sampleStdDev (was stdDev)
class MCSampleStdDev : public MCFunction
{
	MCParameter *params;
public:
	MCSampleStdDev()
	{
		params = NULL;
	}
	virtual ~MCSampleStdDev();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

// JS-2013-06-19: [[ StatsFunctions ]] Definition of sampleVariance
class MCSampleVariance : public MCFunction
{
	MCParameter *params;
public:
	MCSampleVariance()
	{
		params = NULL;
	}
	virtual ~MCSampleVariance();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
};

class MCSqrt : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalSqrt, EE_SQRT_BADSOURCE, PE_SQRT_BADPARAM, kMCMathEvalSqrtMethodInfo>
{
public:
	MCSqrt(){}
	virtual ~MCSqrt(){}
};

class MCStatRound : public MCFunction
{
	MCExpression *source;
	MCExpression *digit;
public:
	MCStatRound()
	{
		source = digit = NULL;
	}
	virtual ~MCStatRound();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCStdDev : public MCParamFunction
{
	MCParameter *params;
public:
	MCStdDev()
	{
		params = NULL;
	}
	virtual ~MCStdDev();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalMinMethodInfo; }
	virtual MCParameter *getmethodarg(void) const { return params; }
};

class MCSum : public MCParamFunction
{
	MCParameter *params;
public:
	MCSum()
	{
		params = NULL;
	}
	virtual ~MCSum();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);

	virtual MCExecMethodInfo *getmethodinfo(void) const { return kMCMathEvalSumMethodInfo; }
	virtual MCParameter *getmethodarg(void) const { return params; }
};

class MCTan : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalTan, EE_TAN_BADSOURCE, PE_TAN_BADPARAM, kMCMathEvalTanMethodInfo>
{
public:
	MCTan(){}
	virtual ~MCTan(){}
};

class MCTextHeightSum : public MCFunction
{
	MCChunk *object;
public:
	MCTextHeightSum()
	{
		object = NULL;
	}
	virtual ~MCTextHeightSum();
	virtual Parse_stat parse(MCScriptPoint &, Boolean the);
	virtual Exec_stat eval(MCExecPoint &);
	virtual void compile(MCSyntaxFactoryRef);
};

class MCTranspose : public MCUnaryFunctionCtxt<MCArrayRef, MCArrayRef, &MCExecContext::EvalExprAsArrayRef, MCArraysEvalTransposeMatrix, EE_TRANSPOSE_BADSOURCE, PE_TRANSPOSE_BADPARAM, kMCArraysEvalTransposeMatrixMethodInfo>
{
public:
	MCTranspose(){}
	virtual ~MCTranspose(){}
};

class MCTrunc : public MCUnaryFunctionCtxt<double, double, &MCExecContext::EvalExprAsDouble, MCMathEvalTrunc, EE_TRUNC_BADSOURCE, PE_TRUNC_BADPARAM, kMCMathEvalTruncMethodInfo>
{
public:
    MCTrunc(){}
    virtual ~MCTrunc(){}
};

class MCHTTPProxyForURL: public MCFunction
{
	MCExpression *url;
	MCExpression *host;
	MCExpression *pac;

	static MCScriptEnvironment *pac_engine;

public:
	MCHTTPProxyForURL(void)
	{
		url = NULL;
		host = NULL;
		pac = NULL;
	}

	virtual ~MCHTTPProxyForURL(void);

	virtual Parse_stat parse(MCScriptPoint& sp, Boolean the);
	virtual Exec_stat eval(MCExecPoint& ep);
	virtual void compile(MCSyntaxFactoryRef);

private:
	static char *PACdnsResolve(const char* const* p_arguments, unsigned int p_argument_count);
	static char *PACmyIpAddress(const char* const* p_arguments, unsigned int p_argument_count);
};

class MCRandomBytes: public MCUnaryFunctionCtxt<uinteger_t, MCDataRef, &MCExecContext::EvalExprAsUInt, MCSecurityEvalRandomBytes, EE_RANDOMBYTES_BADCOUNT, PE_RANDOMBYTES_BADPARAM, kMCSecurityEvalRandomBytesMethodInfo>
{
public:
    MCRandomBytes(void){}
    virtual ~MCRandomBytes(void){}
};

// MW-2012-10-08: [[ HitTest ]] controlAtLoc and controlAtScreenLoc function.
class MCControlAtLoc: public MCUnaryFunction
{
    MCExpression *location;
	bool is_screen : 1;
	
public:
    MCControlAtLoc(bool p_is_screen)
    {
        location = NULL;
		is_screen = p_is_screen;
	}
	
    virtual ~MCControlAtLoc();
    Parse_stat parse(MCScriptPoint &sp, Boolean the);
    virtual void eval_ctxt(MCExecContext &ctxt);
	
    virtual MCExecMethodInfo *getmethodinfo(void) const { return is_screen ? kMCInterfaceEvalControlAtScreenLocMethodInfo : kMCInterfaceEvalControlAtLocMethodInfo; }
    virtual MCExpression *getmethodarg(void) const { return location; }
};

// MW-20113-05-08: [[ Uuid ]] The uuid generation function.
class MCUuidFunc: public MCFunction
{
	MCExpression *type;
	MCExpression *namespace_id;
	MCExpression *name;
	
public:
	MCUuidFunc(void)
	{
		type = nil;
		namespace_id = nil;
		name = nil;
	}
	
	virtual ~MCUuidFunc(void);
	virtual Parse_stat parse(MCScriptPoint &sp, Boolean the);
	virtual Exec_stat eval(MCExecPoint &ep);
};

// MERG-2013-08-14: [[ MeasureText ]] Measure text relative to the effective font on an object
class MCMeasureText: public MCFunction
{
    MCChunk *m_object;
    MCExpression *m_text;
    MCExpression *m_mode;
    bool m_is_unicode;
    
public:
    MCMeasureText(bool p_is_unicode)
    {
        m_object = nil;
        m_text = nil;
        m_mode = nil;
        m_is_unicode = p_is_unicode;
    }
    
    virtual ~MCMeasureText(void);
	virtual Parse_stat parse(MCScriptPoint &sp, Boolean the);
	virtual Exec_stat eval(MCExecPoint &ep);
};

#endif



