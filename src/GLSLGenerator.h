//=============================================================================
//
// Render/GLSLGenerator.h
//
// Created by Max McGuire (max@unknownworlds.com)
// Copyright (c) 2013, Unknown Worlds Entertainment, Inc.
//
//=============================================================================

#ifndef GLSL_GENERATOR_H
#define GLSL_GENERATOR_H

#include "CodeWriter.h"
#include "HLSLTree.h"

namespace M4
{

class GLSLGenerator
{

public:

    enum Target
    {
        Target_VertexShader,
        Target_FragmentShader,
    };

    explicit GLSLGenerator(Allocator* allocator);
    
    bool Generate(const HLSLTree* tree, Target target, const char* entryName);
    const char* GetResult() const;

private:

    void OutputExpressionList(HLSLExpression* expression, HLSLArgument* argument = NULL);
    void OutputExpression(HLSLExpression* expression, const HLSLType* dstType = NULL);
    void OutputIdentifier(const char* name);
    void OutputArguments(HLSLArgument* argument);
    
    /**
     * If the statements are part of a function, then returnType can be used to specify the type
     * that a return statement is expected to produce so that correct casts will be generated.
     */
    void OutputStatements(int indent, HLSLStatement* statement, const HLSLType* returnType = NULL);

    void OutputAttribute(const HLSLType& type, const char* semantic, const char* attribType, const char* prefix);
    void OutputAttributes(HLSLFunction* entryFunction);
    void OutputEntryCaller(HLSLFunction* entryFunction);
    void OutputDeclaration(HLSLDeclaration* declaration);
    void OutputDeclaration(const HLSLType& type, const char* name);

    void OutputSetOutAttribute(const char* semantic, const char* resultName);

    HLSLFunction* FindFunction(HLSLRoot* root, const char* name);
    HLSLStruct* FindStruct(HLSLRoot* root, const char* name);

    void Error(const char* format, ...);

    /** GLSL contains some reserved words that don't exist in HLSL. This function will
     * sanitize those names. */
    const char* GetSafeIdentifierName(const char* name) const;

    /** Generates a name of the format "base+n" where n is an integer such that the name
     * isn't used in the syntax tree. */
    bool ChooseUniqueName(const char* base, char* dst, int dstLength) const;

private:

    static const int    s_numReservedWords = 4;
    static const char*  s_reservedWord[s_numReservedWords];

    CodeWriter          m_writer;

    const HLSLTree*     m_tree;
    const char*         m_entryName;
    Target              m_target;
    bool                m_outputPosition;

    const char*         m_outAttribPrefix;
    const char*         m_inAttribPrefix;

    char                m_matrixRowFunction[64];
    char                m_clipFunction[64];
    char                m_tex2DlodFunction[64];
    char                m_texCUBEbiasFunction[64];
    char                m_scalarSwizzle2Function[64];
    char                m_scalarSwizzle3Function[64];
    char                m_scalarSwizzle4Function[64];
    char                m_sinCosFunction[64];

    bool                m_error;

    char                m_reservedWord[s_numReservedWords][64];

};

}

#endif