//=============================================================================
//
// Render/HLSLParser.cpp
//
// Created by Max McGuire (max@unknownworlds.com)
// Copyright (c) 2013, Unknown Worlds Entertainment, Inc.
//
//=============================================================================

#include "Engine/String.h"
#include "Engine/Assert.h"

#include "HLSLParser.h"
#include "HLSLTree.h"

#include <algorithm>

namespace M4
{

enum CompareFunctionsResult
{
    FunctionsEqual,
    Function1Better,
    Function2Better
};

/** This structure stores a HLSLFunction-like declaration for an intrinsic function */
struct Intrinsic
{
    explicit Intrinsic(const char* name, HLSLBaseType returnType)
    {
        function.name                   = name;
        function.returnType.baseType    = returnType;
        function.numArguments           = 0;
    }
    explicit Intrinsic(const char* name, HLSLBaseType returnType, HLSLBaseType arg1)
    {
        function.name                   = name;
        function.returnType.baseType    = returnType;
        function.numArguments           = 1;
        function.argument               = argument + 0;
        argument[0].type.baseType       = arg1;
        argument[0].type.constant       = true;
    }
    explicit Intrinsic(const char* name, HLSLBaseType returnType, HLSLBaseType arg1, HLSLBaseType arg2)
    {
        function.name                   = name;
        function.returnType.baseType    = returnType;
        function.argument               = argument + 0;
        function.numArguments           = 2;
        argument[0].type.baseType       = arg1;
        argument[0].type.constant       = true;
        argument[0].nextArgument        = argument + 1;
        argument[1].type.baseType       = arg2;
        argument[1].type.constant       = true;
    }
    explicit Intrinsic(const char* name, HLSLBaseType returnType, HLSLBaseType arg1, HLSLBaseType arg2, HLSLBaseType arg3)
    {
        function.name                   = name;
        function.returnType.baseType    = returnType;
        function.argument               = argument + 0;
        function.numArguments           = 3;
        argument[0].type.baseType       = arg1;
        argument[0].type.constant       = true;
        argument[0].nextArgument        = argument + 1;
        argument[1].type.baseType       = arg2;
        argument[1].type.constant       = true;
        argument[1].nextArgument        = argument + 2;
        argument[2].type.baseType       = arg3;
        argument[2].type.constant       = true;
    }
    explicit Intrinsic(const char* name, HLSLBaseType returnType, HLSLBaseType arg1, HLSLBaseType arg2, HLSLBaseType arg3, HLSLBaseType arg4)
    {
        function.name                   = name;
        function.returnType.baseType    = returnType;
        function.argument               = argument + 0;
        function.numArguments           = 3;
        argument[0].type.baseType       = arg1;
        argument[0].type.constant       = true;
        argument[0].nextArgument        = argument + 1;
        argument[1].type.baseType       = arg2;
        argument[1].type.constant       = true;
        argument[1].nextArgument        = argument + 2;
        argument[2].type.baseType       = arg3;
        argument[2].type.constant       = true;
        argument[2].nextArgument        = argument + 3;
        argument[3].type.baseType       = arg4;
        argument[3].type.constant       = true;
    }
    HLSLFunction    function;
    HLSLArgument    argument[4];
};

enum NumericType
{
    NumericType_Float,
    NumericType_Half,
    NumericType_Bool,
    NumericType_Int,
    NumericType_Uint,
    NumericType_Count,
    NumericType_NaN,
};

static const int _numberTypeRank[NumericType_Count][NumericType_Count] = 
{
    //F  H  B  I  U    
    { 0, 4, 4, 4, 4 },  // NumericType_Float
    { 1, 0, 4, 4, 4 },  // NumericType_Half
    { 5, 5, 0, 5, 5 },  // NumericType_Bool
    { 5, 5, 4, 0, 3 },  // NumericType_Int
    { 5, 5, 4, 2, 0 }   // NumericType_Uint
};


struct BaseTypeDescription
{
    const char*     typeName;
    NumericType     numericType;
    int             numComponents;
    int             numDimensions;
    int             height;
    int             binaryOpRank;        
};


#define INTRINSIC_FLOAT1_FUNCTION(name) \
        Intrinsic( name, HLSLBaseType_Float,   HLSLBaseType_Float  ),   \
        Intrinsic( name, HLSLBaseType_Float2,  HLSLBaseType_Float2 ),   \
        Intrinsic( name, HLSLBaseType_Float3,  HLSLBaseType_Float3 ),   \
        Intrinsic( name, HLSLBaseType_Float4,  HLSLBaseType_Float4 ),   \
        Intrinsic( name, HLSLBaseType_Half,    HLSLBaseType_Half   ),   \
        Intrinsic( name, HLSLBaseType_Half2,   HLSLBaseType_Half2  ),   \
        Intrinsic( name, HLSLBaseType_Half3,   HLSLBaseType_Half3  ),   \
        Intrinsic( name, HLSLBaseType_Half4,   HLSLBaseType_Half4  )

#define INTRINSIC_FLOAT2_FUNCTION(name) \
        Intrinsic( name, HLSLBaseType_Float,   HLSLBaseType_Float,   HLSLBaseType_Float  ),   \
        Intrinsic( name, HLSLBaseType_Float2,  HLSLBaseType_Float2,  HLSLBaseType_Float2 ),   \
        Intrinsic( name, HLSLBaseType_Float3,  HLSLBaseType_Float3,  HLSLBaseType_Float3 ),   \
        Intrinsic( name, HLSLBaseType_Float4,  HLSLBaseType_Float4,  HLSLBaseType_Float4 ),   \
        Intrinsic( name, HLSLBaseType_Half,    HLSLBaseType_Half,    HLSLBaseType_Half   ),   \
        Intrinsic( name, HLSLBaseType_Half2,   HLSLBaseType_Half2,   HLSLBaseType_Half2  ),   \
        Intrinsic( name, HLSLBaseType_Half3,   HLSLBaseType_Half3,   HLSLBaseType_Half3  ),   \
        Intrinsic( name, HLSLBaseType_Half4,   HLSLBaseType_Half4,   HLSLBaseType_Half4  )

#define INTRINSIC_FLOAT3_FUNCTION(name) \
        Intrinsic( name, HLSLBaseType_Float,   HLSLBaseType_Float,   HLSLBaseType_Float,  HLSLBaseType_Float ),   \
        Intrinsic( name, HLSLBaseType_Float2,  HLSLBaseType_Float2,  HLSLBaseType_Float,  HLSLBaseType_Float2 ),   \
        Intrinsic( name, HLSLBaseType_Float3,  HLSLBaseType_Float3,  HLSLBaseType_Float,  HLSLBaseType_Float3 ),   \
        Intrinsic( name, HLSLBaseType_Float4,  HLSLBaseType_Float4,  HLSLBaseType_Float,  HLSLBaseType_Float4 ),   \
        Intrinsic( name, HLSLBaseType_Half,    HLSLBaseType_Half,    HLSLBaseType_Half,   HLSLBaseType_Half ),   \
        Intrinsic( name, HLSLBaseType_Half2,   HLSLBaseType_Half2,   HLSLBaseType_Half2,  HLSLBaseType_Half2 ),   \
        Intrinsic( name, HLSLBaseType_Half3,   HLSLBaseType_Half3,   HLSLBaseType_Half3,  HLSLBaseType_Half3 ),   \
        Intrinsic( name, HLSLBaseType_Half4,   HLSLBaseType_Half4,   HLSLBaseType_Half4,  HLSLBaseType_Half4 )

const Intrinsic _intrinsic[] = 
    {
        INTRINSIC_FLOAT1_FUNCTION( "abs" ),
        INTRINSIC_FLOAT2_FUNCTION( "atan2" ),
        INTRINSIC_FLOAT3_FUNCTION( "clamp" ),
        INTRINSIC_FLOAT1_FUNCTION( "cos" ),

        INTRINSIC_FLOAT3_FUNCTION( "lerp" ),
        INTRINSIC_FLOAT3_FUNCTION( "smoothstep" ),

        INTRINSIC_FLOAT1_FUNCTION( "floor" ),
        INTRINSIC_FLOAT1_FUNCTION( "ceil" ),
        INTRINSIC_FLOAT1_FUNCTION( "frac" ),

        INTRINSIC_FLOAT2_FUNCTION( "fmod" ),

        Intrinsic( "clip", HLSLBaseType_Void,  HLSLBaseType_Float    ),
        Intrinsic( "clip", HLSLBaseType_Void,  HLSLBaseType_Float2   ),
        Intrinsic( "clip", HLSLBaseType_Void,  HLSLBaseType_Float3   ),
        Intrinsic( "clip", HLSLBaseType_Void,  HLSLBaseType_Float4   ),
        Intrinsic( "clip", HLSLBaseType_Void,  HLSLBaseType_Half     ),
        Intrinsic( "clip", HLSLBaseType_Void,  HLSLBaseType_Half2    ),
        Intrinsic( "clip", HLSLBaseType_Void,  HLSLBaseType_Half3    ),
        Intrinsic( "clip", HLSLBaseType_Void,  HLSLBaseType_Half4    ),

        Intrinsic( "dot", HLSLBaseType_Float,  HLSLBaseType_Float,   HLSLBaseType_Float  ),
        Intrinsic( "dot", HLSLBaseType_Float,  HLSLBaseType_Float2,  HLSLBaseType_Float2 ),
        Intrinsic( "dot", HLSLBaseType_Float,  HLSLBaseType_Float3,  HLSLBaseType_Float3 ),
        Intrinsic( "dot", HLSLBaseType_Float,  HLSLBaseType_Float4,  HLSLBaseType_Float4 ),
        Intrinsic( "dot", HLSLBaseType_Half,   HLSLBaseType_Half,    HLSLBaseType_Half   ),
        Intrinsic( "dot", HLSLBaseType_Half,   HLSLBaseType_Half2,   HLSLBaseType_Half2  ),
        Intrinsic( "dot", HLSLBaseType_Half,   HLSLBaseType_Half3,   HLSLBaseType_Half3  ),
        Intrinsic( "dot", HLSLBaseType_Half,   HLSLBaseType_Half4,   HLSLBaseType_Half4  ),

        Intrinsic( "cross", HLSLBaseType_Float3,  HLSLBaseType_Float3,  HLSLBaseType_Float3 ),

        Intrinsic( "length", HLSLBaseType_Float,  HLSLBaseType_Float  ),
        Intrinsic( "length", HLSLBaseType_Float,  HLSLBaseType_Float2 ),
        Intrinsic( "length", HLSLBaseType_Float,  HLSLBaseType_Float3 ),
        Intrinsic( "length", HLSLBaseType_Float,  HLSLBaseType_Float4 ),
        Intrinsic( "length", HLSLBaseType_Half,   HLSLBaseType_Half   ),
        Intrinsic( "length", HLSLBaseType_Half,   HLSLBaseType_Half2  ),
        Intrinsic( "length", HLSLBaseType_Half,   HLSLBaseType_Half3  ),
        Intrinsic( "length", HLSLBaseType_Half,   HLSLBaseType_Half4  ),

        INTRINSIC_FLOAT2_FUNCTION( "max" ),
        INTRINSIC_FLOAT2_FUNCTION( "min" ),

        INTRINSIC_FLOAT2_FUNCTION( "mul" ),
        Intrinsic( "mul", HLSLBaseType_Float3, HLSLBaseType_Float3, HLSLBaseType_Float3x3 ),
        Intrinsic( "mul", HLSLBaseType_Float4, HLSLBaseType_Float4, HLSLBaseType_Float4x4 ),

        Intrinsic( "transpose", HLSLBaseType_Float3x3, HLSLBaseType_Float3x3 ),
        Intrinsic( "transpose", HLSLBaseType_Float4x4, HLSLBaseType_Float4x4 ),

        INTRINSIC_FLOAT1_FUNCTION( "normalize" ),
        INTRINSIC_FLOAT2_FUNCTION( "pow" ),
        INTRINSIC_FLOAT1_FUNCTION( "saturate" ),
        INTRINSIC_FLOAT1_FUNCTION( "sin" ),
        INTRINSIC_FLOAT1_FUNCTION( "sqrt" ),
        INTRINSIC_FLOAT1_FUNCTION( "rsqrt" ),
        INTRINSIC_FLOAT1_FUNCTION( "rcp" ),
        
        INTRINSIC_FLOAT1_FUNCTION( "ddx" ),
        INTRINSIC_FLOAT1_FUNCTION( "ddy" ),
        
        INTRINSIC_FLOAT1_FUNCTION( "sign" ),
        INTRINSIC_FLOAT2_FUNCTION( "step" ),
        INTRINSIC_FLOAT2_FUNCTION( "reflect" ),

        Intrinsic( "tex2D",     HLSLBaseType_Float4, HLSLBaseType_Sampler2D, HLSLBaseType_Float2 ),
        Intrinsic( "tex2Dproj", HLSLBaseType_Float4, HLSLBaseType_Sampler2D, HLSLBaseType_Float4 ),
        Intrinsic( "tex2Dlod",  HLSLBaseType_Float4, HLSLBaseType_Sampler2D, HLSLBaseType_Float4 ),

        Intrinsic( "texCUBE",       HLSLBaseType_Float4, HLSLBaseType_SamplerCube, HLSLBaseType_Float3 ),
        Intrinsic( "texCUBEbias",   HLSLBaseType_Float4, HLSLBaseType_SamplerCube, HLSLBaseType_Float4 ),

        Intrinsic( "sincos", HLSLBaseType_Void,  HLSLBaseType_Float,   HLSLBaseType_Float,  HLSLBaseType_Float ),
        Intrinsic( "sincos", HLSLBaseType_Void,  HLSLBaseType_Float2,  HLSLBaseType_Float,  HLSLBaseType_Float2 ),
        Intrinsic( "sincos", HLSLBaseType_Void,  HLSLBaseType_Float3,  HLSLBaseType_Float,  HLSLBaseType_Float3 ),
        Intrinsic( "sincos", HLSLBaseType_Void,  HLSLBaseType_Float4,  HLSLBaseType_Float,  HLSLBaseType_Float4 ),
        Intrinsic( "sincos", HLSLBaseType_Void,  HLSLBaseType_Half,    HLSLBaseType_Half,   HLSLBaseType_Half ),
        Intrinsic( "sincos", HLSLBaseType_Void,  HLSLBaseType_Half2,   HLSLBaseType_Half2,  HLSLBaseType_Half2 ),
        Intrinsic( "sincos", HLSLBaseType_Void,  HLSLBaseType_Half3,   HLSLBaseType_Half3,  HLSLBaseType_Half3 ),
        Intrinsic( "sincos", HLSLBaseType_Void,  HLSLBaseType_Half4,   HLSLBaseType_Half4,  HLSLBaseType_Half4 )

    };

const int _numIntrinsics = sizeof(_intrinsic) / sizeof(Intrinsic);

// The order in this array must match up with HLSLBinaryOp
const int _binaryOpPriority[] =
    {
        2, 1, //  &&, ||
        5, 5, //  +,  -
        6, 6, //  *,  /
        4, 4, //  <,  >,
        4, 4, //  <=, >=,
        3, 3, //  ==, !=
    };

const BaseTypeDescription _baseTypeDescriptions[HLSLBaseType_Count] = 
    {
        { "unknown type",   NumericType_NaN,         0, 0, 0, -1 },     // HLSLBaseType_Unknown
        { "void",           NumericType_NaN,         0, 0, 0, -1 },     // HLSLBaseType_Void
        { "float",          NumericType_Float,       1, 0, 1,  0 },     // HLSLBaseType_Float
        { "float2",         NumericType_Float,       2, 1, 1,  0 },     // HLSLBaseType_Float2
        { "float3",         NumericType_Float,       3, 1, 3,  0 },     // HLSLBaseType_Float3
        { "float4",         NumericType_Float,       4, 1, 4,  0 },     // HLSLBaseType_Float4
        { "float3x3",       NumericType_Float,       3, 2, 3,  0 },     // HLSLBaseType_Float3x3
        { "float4x4",       NumericType_Float,       4, 2, 4,  0 },     // HLSLBaseType_Float4x4
                                                            
        { "half",           NumericType_Half,        1, 0, 1,  1 },     // HLSLBaseType_Half
        { "half2",          NumericType_Half,        2, 1, 1,  1 },     // HLSLBaseType_Half2
        { "half3",          NumericType_Half,        3, 1, 1,  1 },     // HLSLBaseType_Half3
        { "half4",          NumericType_Half,        4, 1, 1,  1 },     // HLSLBaseType_Half4
        { "half3x3",        NumericType_Half,        3, 2, 3,  1 },     // HLSLBaseType_Half3x3
        { "half4x4",        NumericType_Half,        4, 2, 4,  1 },     // HLSLBaseType_Half4x4
                                                            
        { "bool",           NumericType_Bool,        1, 0, 1,  4 },     // HLSLBaseType_Bool
                                                            
        { "int",            NumericType_Int,         1, 0, 1,  3 },     // HLSLBaseType_Int
        { "int2",           NumericType_Int,         2, 1, 1,  3 },     // HLSLBaseType_Int2
        { "int3",           NumericType_Int,         3, 1, 1,  3 },     // HLSLBaseType_Int3
        { "int4",           NumericType_Int,         4, 1, 1,  3 },     // HLSLBaseType_Int4
                                                            
        { "uint",           NumericType_Uint,        1, 0, 1,  2 },     // HLSLBaseType_Uint
        { "uint2",          NumericType_Uint,        2, 1, 1,  2 },     // HLSLBaseType_Uint2
        { "uint3",          NumericType_Uint,        3, 1, 1,  2 },     // HLSLBaseType_Uint3
        { "uint4",          NumericType_Uint,        4, 1, 1,  2 },     // HLSLBaseType_Uint4

        { "texture",        NumericType_NaN,         1, 0, 0, -1 },     // HLSLBaseType_Texture
        { "sampler2D",      NumericType_NaN,         1, 0, 0, -1 },     // HLSLBaseType_Sampler2D
        { "samplerCUBE",    NumericType_NaN,         1, 0, 0, -1 },     // HLSLBaseType_SamplerCube
        { "user defined",   NumericType_NaN,         1, 0, 0, -1 }      // HLSLBaseType_UserDefined
    };

HLSLBaseType _binaryOpTypeLookup[HLSLBaseType_NumericCount][HLSLBaseType_NumericCount] = 
    {
        {
            HLSLBaseType_Float, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float4,
            HLSLBaseType_Float3x3, HLSLBaseType_Float4x4, HLSLBaseType_Float, HLSLBaseType_Float2,
            HLSLBaseType_Float3, HLSLBaseType_Float4, HLSLBaseType_Float3x3, HLSLBaseType_Float4x4,
            HLSLBaseType_Float, HLSLBaseType_Float, HLSLBaseType_Float2, HLSLBaseType_Float3,
            HLSLBaseType_Float4, HLSLBaseType_Float, HLSLBaseType_Float2, HLSLBaseType_Float3,
            HLSLBaseType_Float4
        },
        {
            HLSLBaseType_Float2, HLSLBaseType_Float2, HLSLBaseType_Float2, HLSLBaseType_Float2,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Float2, HLSLBaseType_Float2,
            HLSLBaseType_Float2, HLSLBaseType_Float2, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Float2, HLSLBaseType_Float2, HLSLBaseType_Float2, HLSLBaseType_Float2,
            HLSLBaseType_Float2, HLSLBaseType_Float2, HLSLBaseType_Float2, HLSLBaseType_Float2,
            HLSLBaseType_Float2
        },
        {
            HLSLBaseType_Float3, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float3,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Float3, HLSLBaseType_Float2,
            HLSLBaseType_Float3, HLSLBaseType_Float3, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Float3, HLSLBaseType_Float3, HLSLBaseType_Float2, HLSLBaseType_Float3,
            HLSLBaseType_Float3, HLSLBaseType_Float3, HLSLBaseType_Float2, HLSLBaseType_Float3,
            HLSLBaseType_Float3
        },
        {
            HLSLBaseType_Float4, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float4,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Float4, HLSLBaseType_Float2,
            HLSLBaseType_Float3, HLSLBaseType_Float4, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Float4, HLSLBaseType_Float4, HLSLBaseType_Float2, HLSLBaseType_Float3,
            HLSLBaseType_Float4, HLSLBaseType_Float4, HLSLBaseType_Float2, HLSLBaseType_Float3,
            HLSLBaseType_Float4
        },
        {
            HLSLBaseType_Float3x3, HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Float3x3, HLSLBaseType_Float3x3, HLSLBaseType_Float3x3, HLSLBaseType_Unknown,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Float3x3, HLSLBaseType_Float3x3,
            HLSLBaseType_Float3x3, HLSLBaseType_Float3x3, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Unknown, HLSLBaseType_Float3x3, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Unknown
        },
        {
            HLSLBaseType_Float4x4, HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Float3x3, HLSLBaseType_Float4x4, HLSLBaseType_Float4x4, HLSLBaseType_Unknown,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Float3x3, HLSLBaseType_Float4x4,
            HLSLBaseType_Float4x4, HLSLBaseType_Float4x4, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Unknown, HLSLBaseType_Float4x4, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Unknown
        },
        {
            HLSLBaseType_Float, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float4,
            HLSLBaseType_Float3x3, HLSLBaseType_Float4x4, HLSLBaseType_Half, HLSLBaseType_Half2,
            HLSLBaseType_Half3, HLSLBaseType_Half4, HLSLBaseType_Half3x3, HLSLBaseType_Half4x4,
            HLSLBaseType_Half, HLSLBaseType_Half, HLSLBaseType_Half2, HLSLBaseType_Half3,
            HLSLBaseType_Half4, HLSLBaseType_Half, HLSLBaseType_Half2, HLSLBaseType_Half3,
            HLSLBaseType_Half4
        },
        {
            HLSLBaseType_Float2, HLSLBaseType_Float2, HLSLBaseType_Float2, HLSLBaseType_Float2,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Half2, HLSLBaseType_Half2,
            HLSLBaseType_Half2, HLSLBaseType_Half2, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Half2, HLSLBaseType_Half2, HLSLBaseType_Half2, HLSLBaseType_Half2,
            HLSLBaseType_Half2, HLSLBaseType_Half2, HLSLBaseType_Half2, HLSLBaseType_Half2,
            HLSLBaseType_Half2
        },
        {
            HLSLBaseType_Float3, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float3,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Half3, HLSLBaseType_Half2,
            HLSLBaseType_Half3, HLSLBaseType_Half3, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Half3, HLSLBaseType_Half3, HLSLBaseType_Half2, HLSLBaseType_Half3,
            HLSLBaseType_Half3, HLSLBaseType_Half3, HLSLBaseType_Half2, HLSLBaseType_Half3,
            HLSLBaseType_Half3
        },
        {
            HLSLBaseType_Float4, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float4,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Half4, HLSLBaseType_Half2,
            HLSLBaseType_Half3, HLSLBaseType_Half4, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Half4, HLSLBaseType_Half4, HLSLBaseType_Half2, HLSLBaseType_Half3,
            HLSLBaseType_Half4, HLSLBaseType_Half4, HLSLBaseType_Half2, HLSLBaseType_Half3,
            HLSLBaseType_Half4
        },
        {
            HLSLBaseType_Float3x3, HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Float3x3, HLSLBaseType_Float3x3, HLSLBaseType_Half3x3, HLSLBaseType_Unknown,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Half3x3, HLSLBaseType_Half3x3,
            HLSLBaseType_Half3x3, HLSLBaseType_Half3x3, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Unknown, HLSLBaseType_Half3x3, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Unknown
        },
        {
            HLSLBaseType_Float4x4, HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Float3x3, HLSLBaseType_Float4x4, HLSLBaseType_Half4x4, HLSLBaseType_Unknown,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Half3x3, HLSLBaseType_Half4x4,
            HLSLBaseType_Half4x4, HLSLBaseType_Half4x4, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Unknown, HLSLBaseType_Half4x4, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Unknown
        },
        {
            HLSLBaseType_Float, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float4,
            HLSLBaseType_Float3x3, HLSLBaseType_Float4x4, HLSLBaseType_Half, HLSLBaseType_Half2,
            HLSLBaseType_Half3, HLSLBaseType_Half4, HLSLBaseType_Half3x3, HLSLBaseType_Half4x4,
            HLSLBaseType_Int, HLSLBaseType_Int, HLSLBaseType_Int2, HLSLBaseType_Int3,
            HLSLBaseType_Int4, HLSLBaseType_Uint, HLSLBaseType_Uint2, HLSLBaseType_Uint3,
            HLSLBaseType_Uint4
        },
        {
            HLSLBaseType_Float, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float4,
            HLSLBaseType_Float3x3, HLSLBaseType_Float4x4, HLSLBaseType_Half, HLSLBaseType_Half2,
            HLSLBaseType_Half3, HLSLBaseType_Half4, HLSLBaseType_Half3x3, HLSLBaseType_Half4x4,
            HLSLBaseType_Int, HLSLBaseType_Int, HLSLBaseType_Int2, HLSLBaseType_Int3,
            HLSLBaseType_Int4, HLSLBaseType_Uint, HLSLBaseType_Uint2, HLSLBaseType_Uint3,
            HLSLBaseType_Uint4
        },
        {
            HLSLBaseType_Float2, HLSLBaseType_Float2, HLSLBaseType_Float2, HLSLBaseType_Float2,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Half2, HLSLBaseType_Half2,
            HLSLBaseType_Half2, HLSLBaseType_Half2, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Int2, HLSLBaseType_Int2, HLSLBaseType_Int2, HLSLBaseType_Int2,
            HLSLBaseType_Int2, HLSLBaseType_Uint2, HLSLBaseType_Uint2, HLSLBaseType_Uint2,
            HLSLBaseType_Uint2
        },
        {
            HLSLBaseType_Float3, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float3,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Half3, HLSLBaseType_Half2,
            HLSLBaseType_Half3, HLSLBaseType_Half3, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Int3, HLSLBaseType_Int3, HLSLBaseType_Int2, HLSLBaseType_Int3,
            HLSLBaseType_Int3, HLSLBaseType_Uint3, HLSLBaseType_Uint2, HLSLBaseType_Uint3,
            HLSLBaseType_Uint3
        },
        {
            HLSLBaseType_Float4, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float4,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Half4, HLSLBaseType_Half2,
            HLSLBaseType_Half3, HLSLBaseType_Half4, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Int4, HLSLBaseType_Int4, HLSLBaseType_Int2, HLSLBaseType_Int3,
            HLSLBaseType_Int4, HLSLBaseType_Uint4, HLSLBaseType_Uint2, HLSLBaseType_Uint3,
            HLSLBaseType_Uint4
        },
        {
            HLSLBaseType_Float, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float4,
            HLSLBaseType_Float3x3, HLSLBaseType_Float4x4, HLSLBaseType_Half, HLSLBaseType_Half2,
            HLSLBaseType_Half3, HLSLBaseType_Half4, HLSLBaseType_Half3x3, HLSLBaseType_Half4x4,
            HLSLBaseType_Uint, HLSLBaseType_Uint, HLSLBaseType_Uint2, HLSLBaseType_Uint3,
            HLSLBaseType_Uint4, HLSLBaseType_Uint, HLSLBaseType_Uint2, HLSLBaseType_Uint3,
            HLSLBaseType_Uint4
        },
        {
            HLSLBaseType_Float2, HLSLBaseType_Float2, HLSLBaseType_Float2, HLSLBaseType_Float2,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Half2, HLSLBaseType_Half2,
            HLSLBaseType_Half2, HLSLBaseType_Half2, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Uint2, HLSLBaseType_Uint2, HLSLBaseType_Uint2, HLSLBaseType_Uint2,
            HLSLBaseType_Uint2, HLSLBaseType_Uint2, HLSLBaseType_Uint2, HLSLBaseType_Uint2,
            HLSLBaseType_Uint2
        },
        {
            HLSLBaseType_Float3, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float3,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Half3, HLSLBaseType_Half2,
            HLSLBaseType_Half3, HLSLBaseType_Half3, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Uint3, HLSLBaseType_Uint3, HLSLBaseType_Uint2, HLSLBaseType_Uint3,
            HLSLBaseType_Uint3, HLSLBaseType_Uint3, HLSLBaseType_Uint2, HLSLBaseType_Uint3,
            HLSLBaseType_Uint3
        },
        {
            HLSLBaseType_Float4, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float4,
            HLSLBaseType_Unknown, HLSLBaseType_Unknown, HLSLBaseType_Half4, HLSLBaseType_Half2,
            HLSLBaseType_Half3, HLSLBaseType_Half4, HLSLBaseType_Unknown, HLSLBaseType_Unknown,
            HLSLBaseType_Uint4, HLSLBaseType_Uint4, HLSLBaseType_Uint2, HLSLBaseType_Uint3,
            HLSLBaseType_Uint4, HLSLBaseType_Uint4, HLSLBaseType_Uint2, HLSLBaseType_Uint3,
            HLSLBaseType_Uint4
        },
    };

// Priority of the ? : operator.
const int _conditionalOpPriority = 1;

static const char* GetTypeName(const HLSLType& type)
{
    if (type.baseType == HLSLBaseType_UserDefined)
    {
        return type.typeName;
    }
    else
    {
        return _baseTypeDescriptions[type.baseType].typeName;
    }
}

static const char* GetBinaryOpName(HLSLBinaryOp binaryOp)
{
    switch (binaryOp)
    {
    case HLSLBinaryOp_Add:          return "+";
    case HLSLBinaryOp_Sub:          return "-";
    case HLSLBinaryOp_Mul:          return "*";
    case HLSLBinaryOp_Div:          return "/";
    case HLSLBinaryOp_Less:         return "<";
    case HLSLBinaryOp_Greater:      return ">";
    case HLSLBinaryOp_LessEqual:    return "<=";
    case HLSLBinaryOp_GreaterEqual: return ">=";
    case HLSLBinaryOp_Equal:        return "==";
    case HLSLBinaryOp_NotEqual:     return "!=";
    case HLSLBinaryOp_Assign:       return "=";
    case HLSLBinaryOp_AddAssign:    return "+=";
    case HLSLBinaryOp_SubAssign:    return "-=";
    case HLSLBinaryOp_MulAssign:    return "*=";
    case HLSLBinaryOp_DivAssign:    return "/=";
    case HLSLBinaryOp_And:          return "&&";
    case HLSLBinaryOp_Or:           return "||";
    default:
        ASSERT(0);
        return "???";
    }
}

/*
 * 1.) Match
 * 2.) Scalar dimension promotion (scalar -> vector/matrix)
 * 3.) Conversion
 * 4.) Conversion + scalar dimension promotion
 * 5.) Truncation (vector -> scalar or lower component vector, matrix -> scalar or lower component matrix)
 * 6.) Conversion + truncation
 */    
static int GetTypeCastRank(const HLSLType& srcType, const HLSLType& dstType)
{

    if (srcType.array != dstType.array || srcType.arraySize != dstType.arraySize)
    {
        return -1;
    }

    if (srcType.baseType == HLSLBaseType_UserDefined && dstType.baseType == HLSLBaseType_UserDefined)
    {
        return strcmp(srcType.typeName, dstType.typeName) == 0 ? 0 : -1;
    }

    if (srcType.baseType == dstType.baseType)
    {
        return 0;
    }

    const BaseTypeDescription& srcDesc = _baseTypeDescriptions[srcType.baseType];
    const BaseTypeDescription& dstDesc = _baseTypeDescriptions[dstType.baseType];
    if (srcDesc.numericType == NumericType_NaN || dstDesc.numericType == NumericType_NaN)
    {
        return -1;
    }

    // Result bits: T R R R P (T = truncation, R = conversion rank, P = dimension promotion)
    int result = _numberTypeRank[srcDesc.numericType][dstDesc.numericType] << 1;

    if (srcDesc.numDimensions == 0 && dstDesc.numDimensions > 0)
    {
        // Scalar dimension promotion
        result |= (1 << 0);
    }
    else if ((srcDesc.numDimensions == dstDesc.numDimensions && srcDesc.numComponents > dstDesc.numComponents) ||
             (srcDesc.numDimensions > 0 && dstDesc.numDimensions == 0))
    {
        // Truncation
        result |= (1 << 4);
    }
    else if (srcDesc.numDimensions != dstDesc.numDimensions ||
             srcDesc.numComponents != dstDesc.numComponents)
    {
        // Can't convert
        return -1;
    }
    
    return result;
    
}

static bool GetFunctionCallCastRanks(const HLSLFunctionCall* call, const HLSLFunction* function, int* rankBuffer)
{

    if (function == NULL || function->numArguments != call->numArguments)
    {
        // Function not viable
        return false;
    }

    const HLSLExpression* expression = call->argument;
    const HLSLArgument* argument = function->argument;
   
    for (int i = 0; i < call->numArguments; ++i)
    {
        int rank = GetTypeCastRank(expression->expressionType, argument->type);
        if (rank == -1)
        {
            return false;
        }

        rankBuffer[i] = rank;
        expression = expression->nextExpression;
        argument = argument->nextArgument;
    }

    return true;

}

struct CompareRanks
{
    bool operator() (const int& rank1, const int& rank2) { return rank1 > rank2; }
};

static CompareFunctionsResult CompareFunctions(const HLSLFunctionCall* call, const HLSLFunction* function1, const HLSLFunction* function2)
{ 

    int* function1Ranks = static_cast<int*>(alloca(sizeof(int) * call->numArguments));
    int* function2Ranks = static_cast<int*>(alloca(sizeof(int) * call->numArguments));

    const bool function1Viable = GetFunctionCallCastRanks(call, function1, function1Ranks);
    const bool function2Viable = GetFunctionCallCastRanks(call, function2, function2Ranks);

    // Both functions have to be viable to be able to compare them
    if (!(function1Viable && function2Viable))
    {
        if (function1Viable)
        {
            return Function1Better;
        }
        else if (function2Viable)
        {
            return Function2Better;
        }
        else
        {
            return FunctionsEqual;
        }
    }

    std::sort(function1Ranks, function1Ranks + call->numArguments, CompareRanks());
    std::sort(function2Ranks, function2Ranks + call->numArguments, CompareRanks());
    
    for (int i = 0; i < call->numArguments; ++i)
    {
        if (function1Ranks[i] < function2Ranks[i])
        {
            return Function1Better;
        }
        else if (function2Ranks[i] < function1Ranks[i])
        {
            return Function2Better;
        }
    }

    return FunctionsEqual;

}

static bool GetBinaryOpResultType(HLSLBinaryOp binaryOp, const HLSLType& type1, const HLSLType& type2, HLSLType& result)
{

    if (type1.baseType < HLSLBaseType_FirstNumeric || type1.baseType > HLSLBaseType_LastNumeric || type1.array ||
        type2.baseType < HLSLBaseType_FirstNumeric || type2.baseType > HLSLBaseType_LastNumeric || type2.array)
    {
         return false;
    }

    switch (binaryOp)
    {
    case HLSLBinaryOp_And:
    case HLSLBinaryOp_Or:
    case HLSLBinaryOp_Less:
    case HLSLBinaryOp_Greater:
    case HLSLBinaryOp_LessEqual:
    case HLSLBinaryOp_GreaterEqual:
    case HLSLBinaryOp_Equal:
    case HLSLBinaryOp_NotEqual:
        result.baseType = HLSLBaseType_Bool;
        break;
    default:
        result.baseType = _binaryOpTypeLookup[type1.baseType - HLSLBaseType_FirstNumeric][type2.baseType - HLSLBaseType_FirstNumeric];
        break;
    }

    result.typeName     = NULL;
    result.array        = false;
    result.arraySize    = NULL;
    result.constant     = false;
    
    return result.baseType != HLSLBaseType_Unknown;

}

HLSLParser::HLSLParser(Allocator* allocator, const char* fileName, const char* buffer, size_t length) : 
    m_tokenizer(fileName, buffer, length),
    m_userTypes(allocator),
    m_variables(allocator),
    m_functions(allocator)
{
    m_numGlobals = 0;
}

bool HLSLParser::Accept(int token)
{
    if (m_tokenizer.GetToken() == token)
    {
       m_tokenizer.Next();
       return true;
    }
    return false;
}

bool HLSLParser::Accept(const char* token)
{
    if (m_tokenizer.GetToken() == HLSLToken_Identifier && String_Equal( token, m_tokenizer.GetIdentifier() ) )
    {
        m_tokenizer.Next();
        return true;
    }
    return false;
}

bool HLSLParser::Expect(int token)
{
    if (!Accept(token))
    {
        char want[HLSLTokenizer::s_maxIdentifier];
        m_tokenizer.GetTokenName(token, want);
        char near[HLSLTokenizer::s_maxIdentifier];
        m_tokenizer.GetTokenName(near);
        m_tokenizer.Error("Syntax error: expected '%s' near '%s'", want, near);
        return false;
    }
    return true;
}

bool HLSLParser::AcceptIdentifier(const char*& identifier)
{
    if (m_tokenizer.GetToken() == HLSLToken_Identifier)
    {
        identifier = m_tree->AddString( m_tokenizer.GetIdentifier() );
        m_tokenizer.Next();
        return true;
    }
    return false;
}

bool HLSLParser::ExpectIdentifier(const char*& identifier)
{
    if (!AcceptIdentifier(identifier))
    {
        char near[HLSLTokenizer::s_maxIdentifier];
        m_tokenizer.GetTokenName(near);
        m_tokenizer.Error("Syntax error: expected identifier near '%s'", near);
        identifier = "";
        return false;
    }
    return true;
}

bool HLSLParser::AcceptFloat(float& value)
{
    if (m_tokenizer.GetToken() == HLSLToken_FloatLiteral)
    {
        value = m_tokenizer.GetFloat();
        m_tokenizer.Next();
        return true;
    }
    return false;
}

bool HLSLParser::AcceptInt(int& value)
{
    if (m_tokenizer.GetToken() == HLSLToken_IntLiteral)
    {
        value = m_tokenizer.GetInt();
        m_tokenizer.Next();
        return true;
    }
    return false;
}

bool HLSLParser::ParseTopLevel(HLSLStatement*& statement)
{

    int line             = GetLineNumber();
    const char* fileName = GetFileName();
    
    HLSLBaseType type;
    const char*  typeName = NULL;
    bool         constant = false;

    if (Accept(HLSLToken_Struct))
    {
        // Struct declaration.

        const char* structName = NULL;
        if (!ExpectIdentifier(structName))
        {
            return false;
        }
        if (FindUserDefinedType(structName) != NULL)
        {
            m_tokenizer.Error("struct %s already defined", structName);
            return false;
        }

        if (!Expect('{'))
        {
            return false;
        }

        HLSLStruct* structure = m_tree->AddNode<HLSLStruct>(fileName, line);
        structure->name = structName;

        m_userTypes.PushBack(structure);
 
        HLSLStructField* lastField = NULL;

        // Add the struct to our list of user defined types.
        while (!Accept('}'))
        {
            if (CheckForUnexpectedEndOfStream('}'))
            {
                return false;
            }
            HLSLStructField* field = NULL;
            if (!ParseFieldDeclaration(field))
            {
                return false;
            }
            ASSERT(field != NULL);
            if (lastField == NULL)
            {
                structure->field = field;
            }
            else
            {
                lastField->nextField = field;
            }
            lastField = field;
        }

        statement = structure;

    }
    else if (Accept(HLSLToken_CBuffer) || Accept(HLSLToken_TBuffer))
    {
        // cbuffer/tbuffer declaration.

        HLSLBuffer* buffer = m_tree->AddNode<HLSLBuffer>(fileName, line);
        AcceptIdentifier(buffer->name);

        // Optional register assignment.
        if (Accept(':'))
        {
            if (!Expect(HLSLToken_Register) || !Expect('(') || !ExpectIdentifier(buffer->registerName) || !Expect(')'))
            {
                return false;
            }
            // TODO: Check that we aren't re-using a register.
        }

        // Fields.
        if (!Expect('{'))
        {
            return false;
        }
        HLSLBufferField* lastField = NULL;
        while (!Accept('}'))
        {
            if (CheckForUnexpectedEndOfStream('}'))
            {
                return false;
            }
            HLSLBufferField* field = NULL;
            if (!ParseBufferFieldDeclaration(field))
            {
                m_tokenizer.Error("Expected variable declaration");
                return false;
            }
            DeclareVariable( field->name, field->type );
            if (buffer->field == NULL)
            {
                buffer->field = field;
            }
            else
            {
                lastField->nextField = field;
            }
            lastField = field;
        }

        statement = buffer;
    }
    else if (AcceptType(true, type, typeName, &constant))
    {
        // Global declaration (uniform or function).
        const char* globalName = NULL;
        if (!ExpectIdentifier(globalName))
        {
            return false;
        }

        if (Accept('('))
        {
            // Function declaration.

            HLSLFunction* function = m_tree->AddNode<HLSLFunction>(fileName, line);
            function->name                  = globalName;
            function->returnType.baseType   = type;
            function->returnType.typeName   = typeName;

            BeginScope();

            if (!ParseArgumentList(')', function->argument, function->numArguments))
            {
                return false;
            }

            // Optional semantic.
            if (Accept(':') && !ExpectIdentifier(function->semantic))
            {
                return false;
            }
            
            m_functions.PushBack( function );

            if (!Expect('{') || !ParseBlock(function->statement, function->returnType))
            {
                return false;
            }

            EndScope();

            // Note, no semi-colon at the end of a function declaration.
            statement = function;
            return true;
        }
        else
        {
            // Uniform declaration.
            HLSLDeclaration* declaration = m_tree->AddNode<HLSLDeclaration>(fileName, line);
            declaration->name           = globalName;
            declaration->type.baseType  = type;
            declaration->type.constant  = constant;

            // Handle array syntax.
            if (Accept('['))
            {
                if (!Accept(']'))
                {
                    if (!ParseExpression(declaration->type.arraySize) || !Expect(']'))
                    {
                        return false;
                    }
                }
                declaration->type.array = true;
            }

            // Handle optional register.
            if (Accept(':'))
            {
                if (!Expect(HLSLToken_Register) || !Expect('(') || !ExpectIdentifier(declaration->registerName) || !Expect(')'))
                {
                    return false;
                }
            }

            DeclareVariable( globalName, declaration->type );

            if (!ParseDeclarationAssignment(declaration))
            {
                return false;
            }

            // TODO: Multiple variables declared on one line.
            
            statement = declaration;
        }
    }

    return Expect(';');

}

bool HLSLParser::ParseStatementOrBlock(HLSLStatement*& firstStatement, const HLSLType& returnType)
{
    if (Accept('{'))
    {
        BeginScope();
        if (!ParseBlock(firstStatement, returnType))
        {
            return false;
        }
        EndScope();
        return true;
    }
    else
    {
        return ParseStatement(firstStatement, returnType);
    }
}

bool HLSLParser::ParseBlock(HLSLStatement*& firstStatement, const HLSLType& returnType)
{
    HLSLStatement* lastStatement = NULL;
    while (!Accept('}'))
    {
        if (CheckForUnexpectedEndOfStream('}'))
        {
            return false;
        }
        HLSLStatement* statement = NULL;
        if (!ParseStatement(statement, returnType))
        {
            return false;
        }
        if (statement != NULL)
        {
            if (firstStatement == NULL)
            {
                firstStatement = statement;
            }
            else
            {
                lastStatement->nextStatement = statement;
            }
            lastStatement = statement;
        }
    }
    return true;
}

bool HLSLParser::ParseStatement(HLSLStatement*& statement, const HLSLType& returnType)
{
    const char* fileName = GetFileName();
    int         line     = GetLineNumber();

    // Empty statements.
    if (Accept(';'))
    {
        return true;
    }

    // If statement.
    if (Accept(HLSLToken_If))
    {
        HLSLIfStatement* ifStatement = m_tree->AddNode<HLSLIfStatement>(fileName, line);
        if (!Expect('(') || !ParseExpression(ifStatement->condition) || !Expect(')'))
        {
            return false;
        }
        statement = ifStatement;
        if (!ParseStatementOrBlock(ifStatement->statement, returnType))
        {
            return false;
        }
        if (Accept(HLSLToken_Else))
        {
            return ParseStatementOrBlock(ifStatement->elseStatement, returnType);
        }
        return true;
    }

    // For statement.
    if (Accept(HLSLToken_For))
    {
        HLSLForStatement* forStatement = m_tree->AddNode<HLSLForStatement>(fileName, line);
        if (!Expect('('))
        {
            return false;
        }
        BeginScope();
        if (!ParseDeclaration(forStatement->initialization))
        {
            return false;
        }
        if (!Expect(';'))
        {
            return false;
        }
        ParseExpression(forStatement->condition);
        if (!Expect(';'))
        {
            return false;
        }
        ParseExpression(forStatement->increment);
        if (!Expect(')'))
        {
            return false;
        }
        statement = forStatement;
        if (!ParseStatementOrBlock(forStatement->statement, returnType))
        {
            return false;
        }
        EndScope();
        return true;
    }

    // Discard statement.
    if (Accept(HLSLToken_Discard))
    {
        HLSLDiscardStatement* discardStatement = m_tree->AddNode<HLSLDiscardStatement>(fileName, line);
        statement = discardStatement;
        return Expect(';');
    }

    // Break statement.
    if (Accept(HLSLToken_Break))
    {
        HLSLBreakStatement* breakStatement = m_tree->AddNode<HLSLBreakStatement>(fileName, line);
        statement = breakStatement;
        return Expect(';');
    }

    // Continue statement.
    if (Accept(HLSLToken_Continue))
    {
        HLSLContinueStatement* continueStatement = m_tree->AddNode<HLSLContinueStatement>(fileName, line);
        statement = continueStatement;
        return Expect(';');
    }

    // Return statement
    if (Accept(HLSLToken_Return))
    {
        HLSLReturnStatement* returnStatement = m_tree->AddNode<HLSLReturnStatement>(fileName, line);
        if (!Accept(';') && !ParseExpression(returnStatement->expression))
        {
            return false;
        }
        // Check that the return expression can be cast to the return type of the function.
        if (!CheckTypeCast(returnStatement->expression->expressionType, returnType))
        {
            return false;
        }

        statement = returnStatement;
        return Expect(';');
    }

    HLSLDeclaration* declaration = NULL;
    HLSLExpression*  expression  = NULL;

    if (ParseDeclaration(declaration))
    {
        statement = declaration;
    }
    else if (ParseExpression(expression))
    {
        HLSLExpressionStatement* expressionStatement;
        expressionStatement = m_tree->AddNode<HLSLExpressionStatement>(fileName, line);
        expressionStatement->expression = expression;
        statement = expressionStatement;
    }

    return Expect(';');
}

bool HLSLParser::ParseDeclaration(HLSLDeclaration*& declaration)
{
    const char* fileName    = GetFileName();
    int         line        = GetLineNumber();
    HLSLType    type;
    const char* name;

    if (AcceptDeclaration(true, type, name))
    {
        declaration        = m_tree->AddNode<HLSLDeclaration>(fileName, line);
        declaration->type  = type;
        declaration->name  = name;

        DeclareVariable( declaration->name, declaration->type );

        // Handle option assignment of the declared variables(s).
        return ParseDeclarationAssignment( declaration );
    }

    return false;
}

bool HLSLParser::ParseDeclarationAssignment(HLSLDeclaration* declaration)
{
    if (Accept('='))
    {
        // Handle array initialization syntax.
        if (declaration->type.array)
        {
            int numValues = 0;
            if (!Expect('{') || !ParseExpressionList('}', true, declaration->assignment, numValues))
            {
                return false;
            }
        }
        else if (!ParseExpression(declaration->assignment))
        {
            return false;
        }
    }
    return true;
}

bool HLSLParser::ParseFieldDeclaration(HLSLStructField*& field)
{
    field = m_tree->AddNode<HLSLStructField>( GetFileName(), GetLineNumber() );
    if (!ExpectDeclaration(false, field->type, field->name))
    {
        return false;
    }
    // Handle optional semantics.
    if (Accept(':'))
    {
        if (!ExpectIdentifier(field->semantic))
        {
            return false;
        }
    }
    return Expect(';');
}

bool HLSLParser::ParseBufferFieldDeclaration(HLSLBufferField*& field)
{
    field = m_tree->AddNode<HLSLBufferField>( GetFileName(), GetLineNumber() );
    if (AcceptDeclaration(false, field->type, field->name))
    {
        // Handle optional packoffset.
        if (Accept(':'))
        {
            if (!Expect(HLSLToken_PackOffset))
            {
                return false;
            }
            const char* constantName = NULL;
            const char* swizzleMask  = NULL;
            if (!Expect('(') || !ExpectIdentifier(constantName) || !Expect('.') || !ExpectIdentifier(swizzleMask) || !Expect(')'))
            {
                return false;
            }
        }
        return Expect(';');
    }
    return false;
}

bool HLSLParser::CheckTypeCast(const HLSLType& srcType, const HLSLType& dstType)
{
    if (GetTypeCastRank(srcType, dstType) == -1)
    {
        const char* srcTypeName = GetTypeName(srcType);
        const char* dstTypeName = GetTypeName(dstType);
        m_tokenizer.Error("Cannot implicitly convert from '%s' to '%s'", srcTypeName, dstTypeName);
        return false;
    }
    return true;
}

bool HLSLParser::ParseExpression(HLSLExpression*& expression)
{
    if (!ParseBinaryExpression(0, expression))
    {
        return false;
    }
    HLSLBinaryOp assignOp;
    while (AcceptAssign(assignOp))
    {
        HLSLExpression* expression2 = NULL;
        if (!ParseBinaryExpression(0, expression2))
        {
            return false;
        }
        HLSLBinaryExpression* binaryExpression = m_tree->AddNode<HLSLBinaryExpression>(expression->fileName, expression->line);
        binaryExpression->binaryOp    = assignOp;
        binaryExpression->expression1 = expression;
        binaryExpression->expression2 = expression2;
        // This type is not strictly correct, since the type should be a reference.
        // However, for our usage of the types it should be sufficient.
        binaryExpression->expressionType = expression->expressionType;

        if (!CheckTypeCast(expression2->expressionType, expression->expressionType))
        {
            return false;
        }

        expression = binaryExpression;
    }
    return true;
}

bool HLSLParser::AcceptBinaryOperator(int priority, HLSLBinaryOp& binaryOp)
{
    int token = m_tokenizer.GetToken();
    switch (token)
    {
    case '+':                       binaryOp = HLSLBinaryOp_Add;          break;
    case '-':                       binaryOp = HLSLBinaryOp_Sub;          break;
    case '*':                       binaryOp = HLSLBinaryOp_Mul;          break;
    case '/':                       binaryOp = HLSLBinaryOp_Div;          break;
    case '<':                       binaryOp = HLSLBinaryOp_Less;         break;
    case '>':                       binaryOp = HLSLBinaryOp_Greater;      break;
    case HLSLToken_LessEqual:       binaryOp = HLSLBinaryOp_LessEqual;    break;
    case HLSLToken_GreaterEqual:    binaryOp = HLSLBinaryOp_GreaterEqual; break;
    case HLSLToken_EqualEqual:      binaryOp = HLSLBinaryOp_Equal;        break;
    case HLSLToken_NotEqual:        binaryOp = HLSLBinaryOp_NotEqual;     break;
    case HLSLToken_AndAnd:          binaryOp = HLSLBinaryOp_And;          break;
    case HLSLToken_BarBar:          binaryOp = HLSLBinaryOp_Or;           break;
    default:
        return false;
    }
    if (_binaryOpPriority[binaryOp] > priority)
    {
        m_tokenizer.Next();
        return true;
    }
    return false;
}

bool HLSLParser::AcceptUnaryOperator(bool pre, HLSLUnaryOp& unaryOp)
{
    int token = m_tokenizer.GetToken();
    if (token == HLSLToken_PlusPlus)
    {
        unaryOp = pre ? HLSLUnaryOp_PreIncrement : HLSLUnaryOp_PostIncrement;
    }
    else if (token == HLSLToken_MinusMinus)
    {
        unaryOp = pre ? HLSLUnaryOp_PreDecrement : HLSLUnaryOp_PostDecrement;
    }
    else if (pre && token == '-')
    {
        unaryOp = HLSLUnaryOp_Negative;
    }
    else if (pre && token == '+')
    {
        unaryOp = HLSLUnaryOp_Positive;
    }
    else if (pre && token == '!')
    {
        unaryOp = HLSLUnaryOp_Not;
    }
    else
    {
        return false;
    }
    m_tokenizer.Next();
    return true;
}

bool HLSLParser::AcceptAssign(HLSLBinaryOp& binaryOp)
{
    if (Accept('='))
    {
        binaryOp = HLSLBinaryOp_Assign;
    }
    else if (Accept(HLSLToken_PlusEqual))
    {
        binaryOp = HLSLBinaryOp_AddAssign;
    }
    else if (Accept(HLSLToken_MinusEqual))
    {
        binaryOp = HLSLBinaryOp_SubAssign;
    }     
    else if (Accept(HLSLToken_TimesEqual))
    {
        binaryOp = HLSLBinaryOp_MulAssign;
    }     
    else if (Accept(HLSLToken_DivideEqual))
    {
        binaryOp = HLSLBinaryOp_DivAssign;
    }     
    else
    {
        return false;
    }
    return true;
}

bool HLSLParser::ParseBinaryExpression(int priority, HLSLExpression*& expression)
{
    const char* fileName = GetFileName();
    int         line     = GetLineNumber();

    bool needsEndParen;

    if (!ParseTerminalExpression(expression, needsEndParen))
    {
        return false;
    }

    while (1)
    {

        HLSLBinaryOp binaryOp;
        if (AcceptBinaryOperator(priority, binaryOp))
        {

            HLSLExpression* expression2 = NULL;
            ASSERT( binaryOp < sizeof(_binaryOpPriority) / sizeof(int) );
            if (!ParseBinaryExpression(_binaryOpPriority[binaryOp], expression2))
            {
                return false;
            }
            HLSLBinaryExpression* binaryExpression = m_tree->AddNode<HLSLBinaryExpression>(fileName, line);
            binaryExpression->binaryOp    = binaryOp;
            binaryExpression->expression1 = expression;
            binaryExpression->expression2 = expression2;
            if (!GetBinaryOpResultType( binaryOp, expression->expressionType, expression2->expressionType, binaryExpression->expressionType ))
            {
                const char* typeName1 = GetTypeName( binaryExpression->expression1->expressionType );
                const char* typeName2 = GetTypeName( binaryExpression->expression2->expressionType );
                m_tokenizer.Error("binary '%s' : no global operator found which takes types '%s' and '%s' (or there is no acceptable conversion)",
                    GetBinaryOpName(binaryOp), typeName1, typeName2);

                return false;
            }
            expression = binaryExpression;
        }
        else if (_conditionalOpPriority > priority && Accept('?'))
        {

            HLSLConditionalExpression* conditionalExpression = m_tree->AddNode<HLSLConditionalExpression>(fileName, line);
            conditionalExpression->condition = expression;
            
            HLSLExpression* expression1 = NULL;
            HLSLExpression* expression2 = NULL;
            if (!ParseBinaryExpression(_conditionalOpPriority, expression1) || !Expect(':') || !ParseBinaryExpression(_conditionalOpPriority, expression2))
            {
                return false;
            }

            // Make sure both cases have compatible types.
            if (GetTypeCastRank(expression1->expressionType, expression2->expressionType) == -1)
            {
                const char* srcTypeName = GetTypeName(expression2->expressionType);
                const char* dstTypeName = GetTypeName(expression1->expressionType);
                m_tokenizer.Error("':' no possible conversion from from '%s' to '%s'", srcTypeName, dstTypeName);
                return false;
            }

            conditionalExpression->trueExpression  = expression1;
            conditionalExpression->falseExpression = expression2;
            conditionalExpression->expressionType  = expression1->expressionType;

            expression = conditionalExpression;

        }
        else
        {
            break;
        }

    }

    return !needsEndParen || Expect(')');
}

bool HLSLParser::ParsePartialConstructor(HLSLExpression*& expression, HLSLBaseType type, const char* typeName)
{

    const char* fileName = GetFileName();
    int         line     = GetLineNumber();

    HLSLConstructorExpression* constructorExpression = m_tree->AddNode<HLSLConstructorExpression>(fileName, line);
    constructorExpression->type.baseType = type;
    constructorExpression->type.typeName = typeName;
    int numArguments = 0;
    if (!ParseExpressionList(')', false, constructorExpression->argument, numArguments))
    {
        return false;
    }    
    constructorExpression->expressionType = constructorExpression->type;
    constructorExpression->expressionType.constant = true;
    expression = constructorExpression;
    return true;
}

bool HLSLParser::ParseTerminalExpression(HLSLExpression*& expression, bool& needsEndParen)
{
    const char* fileName = GetFileName();
    int         line     = GetLineNumber();

    needsEndParen = false;

    HLSLUnaryOp unaryOp;
    if (AcceptUnaryOperator(true, unaryOp))
    {
        HLSLUnaryExpression* unaryExpression = m_tree->AddNode<HLSLUnaryExpression>(fileName, line);
        unaryExpression->unaryOp = unaryOp;
        if (!ParseTerminalExpression(unaryExpression->expression, needsEndParen))
        {
            return false;
        }
        if (unaryOp == HLSLUnaryOp_Not)
        {
            unaryExpression->expressionType = HLSLType(HLSLBaseType_Bool);
        }
        else
        {
            unaryExpression->expressionType = unaryExpression->expression->expressionType;
        }
        expression = unaryExpression;
        return true;
    }
    
    // Expressions inside parenthesis or casts.
    if (Accept('('))
    {
        // Check for a casting operator.
        HLSLType type;
        if (AcceptType(false, type.baseType, type.typeName, &type.constant))
        {
            // This is actually a type constructor like (float2(...
            if (Accept('('))
            {
                needsEndParen = true;
                return ParsePartialConstructor(expression, type.baseType, type.typeName);
            }
            HLSLCastingExpression* castingExpression = m_tree->AddNode<HLSLCastingExpression>(fileName, line);
            castingExpression->type = type;
            expression = castingExpression;
            castingExpression->expressionType = type;
            return Expect(')') && ParseExpression(castingExpression->expression);
        }
        return ParseExpression(expression) && Expect(')');
    }

    // Terminal values.

    float fValue = 0.0f;
    int   iValue = 0;
    
    if (AcceptFloat(fValue))
    {
        HLSLLiteralExpression* literalExpression = m_tree->AddNode<HLSLLiteralExpression>(fileName, line);
        literalExpression->type   = HLSLBaseType_Float;
        literalExpression->fValue = fValue;
        literalExpression->expressionType.baseType = literalExpression->type;
        literalExpression->expressionType.constant = true;
        expression = literalExpression;
        return true;
    }
    else if (AcceptInt(iValue))
    {
        HLSLLiteralExpression* literalExpression = m_tree->AddNode<HLSLLiteralExpression>(fileName, line);
        literalExpression->type   = HLSLBaseType_Int;
        literalExpression->iValue = iValue;
        literalExpression->expressionType.baseType = literalExpression->type;
        literalExpression->expressionType.constant = true;
        expression = literalExpression;
        return true;
    }
    else if (Accept(HLSLToken_True))
    {
        HLSLLiteralExpression* literalExpression = m_tree->AddNode<HLSLLiteralExpression>(fileName, line);
        literalExpression->type   = HLSLBaseType_Bool;
        literalExpression->bValue = true;
        literalExpression->expressionType.baseType = literalExpression->type;
        literalExpression->expressionType.constant = true;
        expression = literalExpression;
        return true;
    }
    else if (Accept(HLSLToken_False))
    {
        HLSLLiteralExpression* literalExpression = m_tree->AddNode<HLSLLiteralExpression>(fileName, line);
        literalExpression->type   = HLSLBaseType_Bool;
        literalExpression->bValue = false;
        literalExpression->expressionType.baseType = literalExpression->type;
        literalExpression->expressionType.constant = true;
        expression = literalExpression;
        return true;
    }

    // Type constructor.
    HLSLBaseType    type;
    const char*     typeName = NULL;
    if (AcceptType(false, type, typeName, NULL))
    {
        Expect('(');
        if (!ParsePartialConstructor(expression, type, typeName))
        {
            return false;
        }
    }
    else
    {

        HLSLIdentifierExpression* identifierExpression = m_tree->AddNode<HLSLIdentifierExpression>(fileName, line);
        if (!ExpectIdentifier(identifierExpression->name))
        {
            return false;
        }

        const HLSLType* identifierType = FindVariable(identifierExpression->name, identifierExpression->global);
        if (identifierType != NULL)
        {
            identifierExpression->expressionType = *identifierType;
        }
        else
        {
            if (!GetIsFunction(identifierExpression->name))
            {
                m_tokenizer.Error("Undeclared identifier '%s'", identifierExpression->name);
                return false;
            }
            // Functions are always global scope.
            identifierExpression->global = true;
        }

        expression = identifierExpression;

    }

    bool done = false;
    while (!done)
    {

        done = true;

        // Post fix unary operator
        HLSLUnaryOp unaryOp;
        while (AcceptUnaryOperator(false, unaryOp))
        {
            HLSLUnaryExpression* unaryExpression = m_tree->AddNode<HLSLUnaryExpression>(fileName, line);
            unaryExpression->unaryOp = unaryOp;
            unaryExpression->expression = expression;
            unaryExpression->expressionType = unaryExpression->expression->expressionType;
            expression = unaryExpression;
            done = false;
        }

        // Member access operator.
        while (Accept('.'))
        {
            HLSLMemberAccess* memberAccess = m_tree->AddNode<HLSLMemberAccess>(fileName, line);
            memberAccess->object = expression;
            if (!ExpectIdentifier(memberAccess->field))
            {
                return false;
            }
            if (!GetMemberType( expression->expressionType, memberAccess->field, memberAccess->expressionType ))
            {
                m_tokenizer.Error("Couldn't access '%s'", memberAccess->field);
                return false;
            }
            expression = memberAccess;
            done = false;
        }

        // Handle array access.
        while (Accept('['))
        {
            HLSLArrayAccess* arrayAccess = m_tree->AddNode<HLSLArrayAccess>(fileName, line);
            arrayAccess->array = expression;
            if (!ParseExpression(arrayAccess->index) || !Expect(']'))
            {
                return false;
            }

            if (expression->expressionType.array)
            {
                arrayAccess->expressionType = expression->expressionType;
                arrayAccess->expressionType.array     = false;
                arrayAccess->expressionType.arraySize = NULL;
            }
            else
            {
                switch (expression->expressionType.baseType)
                {
                case HLSLBaseType_Float2:
                case HLSLBaseType_Float3:
                case HLSLBaseType_Float4:
                    arrayAccess->expressionType.baseType = HLSLBaseType_Float;
                    break;
                case HLSLBaseType_Float3x3:
                    arrayAccess->expressionType.baseType = HLSLBaseType_Float3;
                    break;
                case HLSLBaseType_Float4x4:
                    arrayAccess->expressionType.baseType = HLSLBaseType_Float4;
                    break;
                case HLSLBaseType_Half2:
                case HLSLBaseType_Half3:
                case HLSLBaseType_Half4:
                    arrayAccess->expressionType.baseType = HLSLBaseType_Half;
                    break;
                case HLSLBaseType_Half3x3:
                    arrayAccess->expressionType.baseType = HLSLBaseType_Half3;
                    break;
                case HLSLBaseType_Half4x4:
                    arrayAccess->expressionType.baseType = HLSLBaseType_Half4;
                    break;
                case HLSLBaseType_Int2:
                case HLSLBaseType_Int3:
                case HLSLBaseType_Int4:
                    arrayAccess->expressionType.baseType = HLSLBaseType_Int;
                    break;
                case HLSLBaseType_Uint2:
                case HLSLBaseType_Uint3:
                case HLSLBaseType_Uint4:
                    arrayAccess->expressionType.baseType = HLSLBaseType_Uint;
                    break;
                default:
                    m_tokenizer.Error("array, matrix, vector, or indexable object type expected in index expression");
                    return false;
                }
            }

            expression = arrayAccess;
            done = false;
        }

        // Handle function calls. Note, HLSL functions aren't like C function
        // pointers -- we can only directly call on an identifier, not on an
        // expression.
        if (Accept('('))
        {
            HLSLFunctionCall* functionCall = m_tree->AddNode<HLSLFunctionCall>(fileName, line);
            done = false;
            if (!ParseExpressionList(')', false, functionCall->argument, functionCall->numArguments))
            {
                return false;
            }

            if (expression->nodeType != HLSLNodeType_IdentifierExpression)
            {
                m_tokenizer.Error("Expected function identifier");
                return false;
            }

            const HLSLIdentifierExpression* identifierExpression = static_cast<const HLSLIdentifierExpression*>(expression);
            const HLSLFunction* function = MatchFunctionCall( functionCall, identifierExpression->name );
            if (function == NULL)
            {
                return false;
            }

            functionCall->function = function;
            functionCall->expressionType = function->returnType;
            expression = functionCall;
        }

    }
    return true;

}

bool HLSLParser::ParseExpressionList(int endToken, bool allowEmptyEnd, HLSLExpression*& firstExpression, int& numExpressions)
{
    numExpressions = 0;
    HLSLExpression* lastExpression = NULL;
    while (!Accept(endToken))
    {
        if (CheckForUnexpectedEndOfStream(endToken))
        {
            return false;
        }
        if (numExpressions > 0 && !Expect(','))
        {
            return false;
        }
        // It is acceptable for the final element in the initialization list to
        // have a trailing comma in some cases, like array initialization such as {1, 2, 3,}
        if (allowEmptyEnd && Accept(endToken))
        {
            break;
        }
        HLSLExpression* expression = NULL;
        if (!ParseExpression(expression))
        {
            return false;
        }
        if (firstExpression == NULL)
        {
            firstExpression = expression;
        }
        else
        {
            lastExpression->nextExpression = expression;
        }
        lastExpression = expression;
        ++numExpressions;
    }
    return true;
}

bool HLSLParser::ParseArgumentList(int endToken, HLSLArgument*& firstArgument, int& numArguments)
{
    const char* fileName = GetFileName();
    int         line     = GetLineNumber();
        
    HLSLArgument* lastArgument = NULL;
    numArguments = 0;

    while (!Accept(endToken))
    {
        if (CheckForUnexpectedEndOfStream(endToken))
        {
            return false;
        }
        if (numArguments > 0 && !Expect(','))
        {
            return false;
        }

        HLSLArgument* argument = m_tree->AddNode<HLSLArgument>(fileName, line);

        if (Accept(HLSLToken_Uniform))     { argument->modifier = HLSLArgumentModifier_Uniform; }
        else if (Accept(HLSLToken_In))     { argument->modifier = HLSLArgumentModifier_In;      }
        else if (Accept(HLSLToken_InOut))  { argument->modifier = HLSLArgumentModifier_Inout;   }

        if (!ExpectDeclaration(true, argument->type, argument->name))
        {
            return false;
        }

        DeclareVariable( argument->name, argument->type );

        // Optional semantic.
        if (Accept(':') && !ExpectIdentifier(argument->semantic))
        {
            return false;
        }

        // Optional interpolation modifier.
        if (Accept("linear"))               { }
        else if (Accept("centroid"))        { }
        else if (Accept("nointerpolation")) { }
        else if (Accept("noperspective"))   { }
        else if (Accept("sample"))          { }

        // TODO: Optional initialization.

        if (lastArgument != NULL)
        {
            lastArgument->nextArgument = argument;
        }
        else
        {
            firstArgument = argument;
        }
        lastArgument = argument;

        ++numArguments;
    }
    return true;
}

bool HLSLParser::Parse(HLSLTree* tree)
{
    m_tree = tree;
    
    HLSLRoot* root = m_tree->GetRoot();
    HLSLStatement* lastStatement = NULL;

    while (!Accept(HLSLToken_EndOfStream))
    {
        HLSLStatement* statement = NULL;
        if (!ParseTopLevel(statement))
        {
            return false;
        }
        if (statement != NULL)
        {   
            if (lastStatement == NULL)
            {
                root->statement = statement;
            }
            else
            {
                lastStatement->nextStatement = statement;
            }
            lastStatement = statement;
        }

    }
    return true;
}

bool HLSLParser::AcceptType(bool allowVoid, HLSLBaseType& type, const char*& typeName, bool* constant)
{

    if (constant != NULL)
    {
        if (Accept(HLSLToken_Const))
        {
            *constant = true;
        }
        else
        {
            *constant = false;
        }
    }

    int token = m_tokenizer.GetToken();

    // Check built-in types.
    type = HLSLBaseType_Void;
    switch (token)
    {
    case HLSLToken_Float:
        type = HLSLBaseType_Float;
        break;
    case HLSLToken_Float2:      
        type = HLSLBaseType_Float2;
        break;
    case HLSLToken_Float3:
        type = HLSLBaseType_Float3;
        break;
    case HLSLToken_Float4:
        type = HLSLBaseType_Float4;
        break;
    case HLSLToken_Float3x3:
        type = HLSLBaseType_Float3x3;
        break;
    case HLSLToken_Float4x4:
        type = HLSLBaseType_Float4x4;
        break;
    case HLSLToken_Half:
        type = HLSLBaseType_Half;
        break;
    case HLSLToken_Half2:      
        type = HLSLBaseType_Half2;
        break;
    case HLSLToken_Half3:
        type = HLSLBaseType_Half3;
        break;
    case HLSLToken_Half4:
        type = HLSLBaseType_Half4;
        break;
    case HLSLToken_Half3x3:
        type = HLSLBaseType_Half3x3;
        break;
    case HLSLToken_Half4x4:
        type = HLSLBaseType_Half4x4;
        break;
    case HLSLToken_Bool:
        type = HLSLBaseType_Bool;
        break;
    case HLSLToken_Int:
        type = HLSLBaseType_Int;
        break;
    case HLSLToken_Int2:
        type = HLSLBaseType_Int2;
        break;
    case HLSLToken_Int3:
        type = HLSLBaseType_Int3;
        break;
    case HLSLToken_Int4:
        type = HLSLBaseType_Int4;
        break;
    case HLSLToken_Uint:
        type = HLSLBaseType_Uint;
        break;
    case HLSLToken_Uint2:
        type = HLSLBaseType_Uint2;
        break;
    case HLSLToken_Uint3:
        type = HLSLBaseType_Uint3;
        break;
    case HLSLToken_Uint4:
        type = HLSLBaseType_Uint4;
        break;
    case HLSLToken_Texture:
        type = HLSLBaseType_Texture;
        break;
    case HLSLToken_Sampler2D:
        type = HLSLBaseType_Sampler2D;
        break;
    case HLSLToken_SamplerCube:
        type = HLSLBaseType_SamplerCube;
        break;
    }
    if (type != HLSLBaseType_Void)
    {
        m_tokenizer.Next();
        return true;
    }

    if (allowVoid && Accept(HLSLToken_Void))
    {
        type = HLSLBaseType_Void;
        return true;
    }
    if (token == HLSLToken_Identifier)
    {
        const char* identifier = m_tree->AddString( m_tokenizer.GetIdentifier() );
        if (FindUserDefinedType(identifier) != NULL)
        {
            m_tokenizer.Next();
            type        = HLSLBaseType_UserDefined;
            typeName    = identifier;
            return true;
        }
    }
    return false;
}

bool HLSLParser::ExpectType(bool allowVoid, HLSLBaseType& type, const char*& typeName, bool* constant)
{
    if (!AcceptType(allowVoid, type, typeName, constant))
    {
        m_tokenizer.Error("Expected type");
        return false;
    }
    return true;
}

bool HLSLParser::AcceptDeclaration(bool allowUnsizedArray, HLSLType& type, const char*& name)
{
    if (!AcceptType(false, type.baseType, type.typeName, &type.constant))
    {
        return false;
    }

    if (!ExpectIdentifier(name))
    {
        // TODO: false means we didn't accept a declaration and we had an error!
        return false;
    }
    // Handle array syntax.
    if (Accept('['))
    {
        type.array = true;
        // Optionally allow no size to the specified for the array.
        if (Accept(']') && allowUnsizedArray)
        {
            return true;
        }
        if (!ParseExpression(type.arraySize) || !Expect(']'))
        {
            return false;
        }
    }
    return true;
}

bool HLSLParser::ExpectDeclaration(bool allowUnsizedArray, HLSLType& type, const char*& name)
{
    if (!AcceptDeclaration(allowUnsizedArray, type, name))
    {
        m_tokenizer.Error("Expected declaration");
        return false;
    }
    return true;
}

const HLSLStruct* HLSLParser::FindUserDefinedType(const char* name) const
{
    // Pointer comparison is sufficient for strings since they exist in the
    // string pool.
    for (int i = 0; i < m_userTypes.GetSize(); ++i)
    {
        if (m_userTypes[i]->name == name)
        {
            return m_userTypes[i];
        }
    }
    return NULL;
}

bool HLSLParser::CheckForUnexpectedEndOfStream(int endToken)
{
    if (Accept(HLSLToken_EndOfStream))
    {
        char what[HLSLTokenizer::s_maxIdentifier];
        m_tokenizer.GetTokenName(endToken, what);
        m_tokenizer.Error("Unexpected end of file while looking for '%s'", what);
        return true;
    }
    return false;
}

int HLSLParser::GetLineNumber() const
{
    return m_tokenizer.GetLineNumber();
}

const char* HLSLParser::GetFileName()
{
    return m_tree->AddString( m_tokenizer.GetFileName() );
}

void HLSLParser::BeginScope()
{
    // Use NULL as a sentinel that indices a new scope level.
    Variable& variable = m_variables.PushBackNew();
    variable.name = NULL;
}

void HLSLParser::EndScope()
{
    int numVariables = m_variables.GetSize() - 1;
    while (m_variables[numVariables].name != NULL)
    {
        --numVariables;
        ASSERT(numVariables >= 0);
    }
    m_variables.Resize(numVariables);
}

const HLSLType* HLSLParser::FindVariable(const char* name, bool& global) const
{
    for (int i = m_variables.GetSize() - 1; i >= 0; --i)
    {
        if (m_variables[i].name == name)
        {
            global = (i < m_numGlobals);
            return &m_variables[i].type;
        }
    }
    return NULL;
}

const HLSLFunction* HLSLParser::FindFunction(const char* name) const
{
    for (int i = 0; i < m_functions.GetSize(); ++i)
    {
        if (m_functions[i]->name == name)
        {
            return m_functions[i];
        }
    }
    return NULL;
}

void HLSLParser::DeclareVariable(const char* name, const HLSLType& type)
{
    if (m_variables.GetSize() == m_numGlobals)
    {
        ++m_numGlobals;
    }
    Variable& variable = m_variables.PushBackNew();
    variable.name = name;
    variable.type = type;
}

bool HLSLParser::GetIsFunction(const char* name) const
{
    for (int i = 0; i < m_functions.GetSize(); ++i)
    {
        // == is ok here because we're passed the strings through the string pool.
        if (m_functions[i]->name == name)
        {
            return true;
        }
    }
    for (int i = 0; i < _numIntrinsics; ++i)
    {
        // Intrinsic names are not in the string pool (since they are compile time
        // constants, so we need full string compare).
        if (String_Equal(name, _intrinsic[i].function.name))
        {
            return true;
        }
    }

    return false;
}

const HLSLFunction* HLSLParser::MatchFunctionCall(const HLSLFunctionCall* functionCall, const char* name)
{

    const HLSLFunction* matchedFunction     = NULL;

    int  numArguments           = functionCall->numArguments;
    int  numMatchedOverloads    = 0;
    bool nameMatches            = false;

    // Get the user defined functions with the specified name.
    for (int i = 0; i < m_functions.GetSize(); ++i)
    {
        const HLSLFunction* function = m_functions[i];
        if (function->name == name)
        {
            nameMatches = true;
            
            CompareFunctionsResult result = CompareFunctions( functionCall, function, matchedFunction );
            if (result == Function1Better)
            {
                matchedFunction = function;
                numMatchedOverloads = 1;
            }
            else if (result == FunctionsEqual)
            {
                ++numMatchedOverloads;
            }
        }
    }

    // Get the intrinsic functions with the specified name.
    for (int i = 0; i < _numIntrinsics; ++i)
    {
        const HLSLFunction* function = &_intrinsic[i].function;
        if (String_Equal(function->name, name))
        {
            nameMatches = true;

            CompareFunctionsResult result = CompareFunctions( functionCall, function, matchedFunction );
            if (result == Function1Better)
            {
                matchedFunction = function;
                numMatchedOverloads = 1;
            }
            else if (result == FunctionsEqual)
            {
                ++numMatchedOverloads;
            }
        }
    }

    if (matchedFunction != NULL && numMatchedOverloads > 1)
    {
        // Multiple overloads match.
        m_tokenizer.Error("'%s' %d overloads have similar conversions", name, numMatchedOverloads);
        return NULL;
    }
    else if (matchedFunction == NULL)
    {
        if (nameMatches)
        {
            m_tokenizer.Error("'%s' no overloaded function matched all of the arguments", name);
        }
        else
        {
            m_tokenizer.Error("Undeclared identifier '%s'", name);
        }
    }

    return matchedFunction;

}

bool HLSLParser::GetMemberType(const HLSLType& objectType, const char* fieldName, HLSLType& memberType)
{

    if (objectType.baseType == HLSLBaseType_UserDefined)
    {
        const HLSLStruct* structure = FindUserDefinedType( objectType.typeName );
        ASSERT(structure != NULL);

        const HLSLStructField* field = structure->field;
        while (field != NULL)
        {
            if (field->name == fieldName)
            {
                memberType = field->type;
                return true;
            }
            field = field->nextField;
        }

        return false;
    }

    if (_baseTypeDescriptions[objectType.baseType].numericType == NumericType_NaN)
    {
        // Currently we don't have an non-numeric types that allow member access.
        return false;
    }

    int swizzleLength = 0;

    if (_baseTypeDescriptions[objectType.baseType].numDimensions <= 1)
    {
        // Check for a swizzle on the scalar/vector types.
        for (int i = 0; fieldName[i] != 0; ++i)
        {
            if (fieldName[i] != 'x' && fieldName[i] != 'y' && fieldName[i] != 'z' && fieldName[i] != 'w' &&
                fieldName[i] != 'r' && fieldName[i] != 'g' && fieldName[i] != 'b' && fieldName[i] != 'a')
            {
                m_tokenizer.Error("Invalid swizzle '%s'", fieldName);
                return false;
            }
            ++swizzleLength;
        }
        ASSERT(swizzleLength > 0);
    }
    else
    {

        // Check for a matrix element access (e.g. _m00 or _11)

        const char* n = fieldName;
        while (n[0] == '_')
        {
            ++n;
            int base = 1;
            if (n[0] == 'm')
            {
                base = 0;
                ++n;
            }
            if (!isdigit(n[0]) || !isdigit(n[1]))
            {
                return false;
            }

            int r = (n[0] - '0') - base;
            int c = (n[1] - '0') - base;
            if (r >= _baseTypeDescriptions[objectType.baseType].height ||
                c >= _baseTypeDescriptions[objectType.baseType].numComponents)
            {
                return false;
            }
            ++swizzleLength;
            n += 2;

        }

        if (n[0] != 0)
        {
            return false;
        }

    }

    if (swizzleLength > 4)
    {
        m_tokenizer.Error("Invalid swizzle '%s'", fieldName);
        return false;
    }

    static const HLSLBaseType floatType[] = { HLSLBaseType_Float, HLSLBaseType_Float2, HLSLBaseType_Float3, HLSLBaseType_Float4 };
    static const HLSLBaseType halfType[]  = { HLSLBaseType_Half,  HLSLBaseType_Half2,  HLSLBaseType_Half3,  HLSLBaseType_Half4  };
    static const HLSLBaseType intType[]   = { HLSLBaseType_Int,   HLSLBaseType_Int2,   HLSLBaseType_Int3,   HLSLBaseType_Int4   };
    static const HLSLBaseType uintType[]  = { HLSLBaseType_Uint,  HLSLBaseType_Uint2,  HLSLBaseType_Uint3,  HLSLBaseType_Uint4  };
    
    switch (_baseTypeDescriptions[objectType.baseType].numericType)
    {
    case NumericType_Float:
        memberType.baseType = floatType[swizzleLength - 1];
        break;
    case NumericType_Half:
        memberType.baseType = halfType[swizzleLength - 1];
        break;
    case NumericType_Int:
        memberType.baseType = intType[swizzleLength - 1];
        break;
    case NumericType_Uint:
        memberType.baseType = uintType[swizzleLength - 1];
        break;
    default:
        ASSERT(0);
    }
    
    return true;

}

}