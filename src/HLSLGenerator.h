//=============================================================================
//
// Render/HLSLGenerator.h
//
// Created by Max McGuire (max@unknownworlds.com)
// Copyright (c) 2013, Unknown Worlds Entertainment, Inc.
//
//=============================================================================

#ifndef HLSL_GENERATOR_H
#define HLSL_GENERATOR_H

#include "CodeWriter.h"
#include "HLSLTree.h"

namespace M4
{

class  HLSLTree;
struct HLSLFunction;
struct HLSLStruct;

/**
 * This class is used to generate HLSL which is compatible with the D3D9
 * compiler (i.e. no cbuffers).
 */
class HLSLGenerator
{

public:

    enum Target
    {
        Target_VertexShader,
        Target_PixelShader,
    };

    explicit HLSLGenerator(Allocator* allocator);
    
    bool Generate(const HLSLTree* tree, Target target, const char* entryName, bool legacy);
    const char* GetResult() const;

private:

    void OutputExpressionList(HLSLExpression* expression);
    void OutputExpression(HLSLExpression* expression);
    void OutputArguments(HLSLArgument* argument);
    void OutputStatements(int indent, HLSLStatement* statement);
    void OutputDeclaration(const HLSLType& type, const char* name, const char* semantic = NULL);
    void OutputDeclaration(HLSLDeclaration* declaration);

    /** Generates a name of the format "base+n" where n is an integer such that the name
     * isn't used in the syntax tree. */
    bool ChooseUniqueName(const char* base, char* dst, int dstLength) const;

private:

    CodeWriter      m_writer;

    const HLSLTree* m_tree;
    const char*     m_entryName;
    bool            m_legacy;

    char            m_textureSampler2DStruct[64];
    char            m_textureSampler2DCtor[64];
    char            m_textureSamplerCubeStruct[64];
    char            m_textureSamplerCubeCtor[64];
    char            m_tex2DFunction[64];
    char            m_tex2DProjFunction[64];
    char            m_tex2DLodFunction[64];
    char            m_texCubeFunction[64];
    char            m_texCubeBiasFunction[64];

};

}

#endif