//=============================================================================
//
// Render/HLSLGenerator.cpp
//
// Created by Max McGuire (max@unknownworlds.com)
// Copyright (c) 2013, Unknown Worlds Entertainment, Inc.
//
//=============================================================================

#include "Engine/String.h"
#include "Engine/Log.h"

#include "HLSLGenerator.h"
#include "HLSLParser.h"
#include "HLSLTree.h"

namespace M4
{

static const char* GetTypeName(const HLSLType& type)
{
    switch (type.baseType)
    {
    case HLSLBaseType_Void:         return "void";
    case HLSLBaseType_Float:        return "float";
    case HLSLBaseType_Float2:       return "float2";
    case HLSLBaseType_Float3:       return "float3";
    case HLSLBaseType_Float4:       return "float4";
    case HLSLBaseType_Float3x3:     return "float3x3";
    case HLSLBaseType_Float4x4:     return "float4x4";
    case HLSLBaseType_Half:         return "half";
    case HLSLBaseType_Half2:        return "half2";
    case HLSLBaseType_Half3:        return "half3";
    case HLSLBaseType_Half4:        return "half4";
    case HLSLBaseType_Half3x3:      return "half3x3";
    case HLSLBaseType_Half4x4:      return "half4x4";
    case HLSLBaseType_Bool:         return "bool";
    case HLSLBaseType_Int:          return "int";
    case HLSLBaseType_Int2:         return "int2";
    case HLSLBaseType_Int3:         return "int3";
    case HLSLBaseType_Int4:         return "int4";
    case HLSLBaseType_Uint:         return "uint";
    case HLSLBaseType_Uint2:        return "uint2";
    case HLSLBaseType_Uint3:        return "uint3";
    case HLSLBaseType_Uint4:        return "uint4";
    case HLSLBaseType_Texture:      return "texture";
    case HLSLBaseType_Sampler2D:    return "sampler2D";
    case HLSLBaseType_SamplerCube:  return "samplerCUBE";
    case HLSLBaseType_UserDefined:  return type.typeName;
    }
    return "?";
}

static bool GetIsSamplerType(const HLSLType& type)
{
    return type.baseType == HLSLBaseType_Sampler2D ||
           type.baseType == HLSLBaseType_SamplerCube;
}

static int GetFunctionArguments(HLSLFunctionCall* functionCall, HLSLExpression* expression[], int maxArguments)
{
    HLSLExpression* argument = functionCall->argument;
    int numArguments = 0;
    while (argument != NULL)
    {
        if (numArguments < maxArguments)
        {
            expression[numArguments] = argument;
        }
        argument = argument->nextExpression;
        ++numArguments;
    }
    return numArguments;
}

HLSLGenerator::HLSLGenerator(Allocator* allocator) :
    m_writer(allocator)
{
    m_tree                          = NULL;
    m_entryName                     = NULL;
    m_legacy                        = false;
    m_textureSampler2DStruct[0]     = 0;
    m_textureSampler2DCtor[0]       = 0;
    m_textureSamplerCubeStruct[0]   = 0;
    m_textureSamplerCubeCtor[0]     = 0;
    m_tex2DFunction[0]              = 0;
    m_tex2DProjFunction[0]          = 0;
    m_tex2DLodFunction[0]           = 0;
    m_texCubeFunction[0]            = 0;
    m_texCubeBiasFunction[0]        = 0;
}

bool HLSLGenerator::Generate(const HLSLTree* tree, Target target, const char* entryName, bool legacy)
{

    m_tree      = tree;
    m_entryName = entryName;
    m_legacy    = legacy;

    HLSLRoot* root = m_tree->GetRoot();
    HLSLStatement* statement = root->statement;

    ChooseUniqueName("TextureSampler2D",            m_textureSampler2DStruct,   sizeof(m_textureSampler2DStruct));
    ChooseUniqueName("CreateTextureSampler2D",      m_textureSampler2DCtor,     sizeof(m_textureSampler2DCtor));
    ChooseUniqueName("TextureSamplerCube",          m_textureSamplerCubeStruct, sizeof(m_textureSamplerCubeStruct));
    ChooseUniqueName("CreateTextureSamplerCube",    m_textureSamplerCubeCtor,   sizeof(m_textureSamplerCubeCtor));
    ChooseUniqueName("tex2D",                       m_tex2DFunction,            sizeof(m_tex2DFunction));
    ChooseUniqueName("tex2Dproj",                   m_tex2DProjFunction,        sizeof(m_tex2DProjFunction));
    ChooseUniqueName("tex2Dlod",                    m_tex2DLodFunction,         sizeof(m_tex2DLodFunction));
    ChooseUniqueName("texCUBE",                     m_texCubeFunction,          sizeof(m_texCubeFunction));
    ChooseUniqueName("texCUBEbias",                 m_texCubeBiasFunction,      sizeof(m_texCubeBiasFunction));

    if (!m_legacy)
    {
        m_writer.WriteLine(0, "struct %s {", m_textureSampler2DStruct);
        m_writer.WriteLine(1, "Texture2D    t;");
        m_writer.WriteLine(1, "SamplerState s;");
        m_writer.WriteLine(0, "};");

        m_writer.WriteLine(0, "struct %s {", m_textureSamplerCubeStruct);
        m_writer.WriteLine(1, "TextureCube   t;");
        m_writer.WriteLine(1, "SamplerState s;");
        m_writer.WriteLine(0, "};");

        m_writer.WriteLine(0, "%s %s(Texture2D t, SamplerState s) {", m_textureSampler2DStruct, m_textureSampler2DCtor);
        m_writer.WriteLine(1, "%s ts;", m_textureSampler2DStruct);
        m_writer.WriteLine(1, "ts.t = t; ts.s = s;");
        m_writer.WriteLine(1, "return ts;");
        m_writer.WriteLine(0, "}");

        m_writer.WriteLine(0, "%s %s(TextureCube t, SamplerState s) {", m_textureSamplerCubeStruct, m_textureSamplerCubeCtor);
        m_writer.WriteLine(1, "%s ts;", m_textureSamplerCubeStruct);
        m_writer.WriteLine(1, "ts.t = t; ts.s = s;");
        m_writer.WriteLine(1, "return ts;");
        m_writer.WriteLine(0, "}");

         m_writer.WriteLine(0, "float4 %s(%s ts, float2 texCoord) {", m_tex2DFunction, m_textureSampler2DStruct);
         m_writer.WriteLine(1, "return ts.t.Sample(ts.s, texCoord);");
         m_writer.WriteLine(0, "}");

         m_writer.WriteLine(0, "float4 %s(%s ts, float4 texCoord) {", m_tex2DProjFunction, m_textureSampler2DStruct);
         m_writer.WriteLine(1, "return ts.t.Sample(ts.s, texCoord.xy / texCoord.w);");
         m_writer.WriteLine(0, "}");

         m_writer.WriteLine(0, "float4 %s(%s ts, float4 texCoord) {", m_tex2DLodFunction, m_textureSampler2DStruct);
         m_writer.WriteLine(1, "return ts.t.SampleLevel(ts.s, texCoord.xy, texCoord.w);");
         m_writer.WriteLine(0, "}");
         
         m_writer.WriteLine(0, "float4 %s(%s ts, float3 texCoord) {", m_texCubeFunction, m_textureSamplerCubeStruct);
         m_writer.WriteLine(1, "return ts.t.Sample(ts.s, texCoord);");
         m_writer.WriteLine(0, "}");

         m_writer.WriteLine(0, "float4 %s(%s ts, float4 texCoord) {", m_texCubeBiasFunction, m_textureSamplerCubeStruct);
         m_writer.WriteLine(1, "return ts.t.SampleBias(ts.s, texCoord.xyz, texCoord.w);");
         m_writer.WriteLine(0, "}");

    }

    OutputStatements(0, statement);

    m_tree = NULL;
    return true;

}

const char* HLSLGenerator::GetResult() const
{
    return m_writer.GetResult();
}

void HLSLGenerator::OutputExpressionList(HLSLExpression* expression)
{
    int numExpressions = 0;
    while (expression != NULL)
    {
        if (numExpressions > 0)
        {
            m_writer.Write(", ");
        }
        OutputExpression(expression);
        expression = expression->nextExpression;
        ++numExpressions;
    }
}

void HLSLGenerator::OutputExpression(HLSLExpression* expression)
{
    if (expression->nodeType == HLSLNodeType_IdentifierExpression)
    {
        HLSLIdentifierExpression* identifierExpression = static_cast<HLSLIdentifierExpression*>(expression);
        const char* name = identifierExpression->name;
        if (!m_legacy && GetIsSamplerType(identifierExpression->expressionType) && identifierExpression->global)
        {
            if (identifierExpression->expressionType.baseType == HLSLBaseType_Sampler2D)
            {
                m_writer.Write("%s(%s_texture, %s_sampler)", m_textureSampler2DCtor, name, name);
            }
            else if (identifierExpression->expressionType.baseType == HLSLBaseType_SamplerCube)
            {
                m_writer.Write("%s(%s_texture, %s_sampler)", m_textureSamplerCubeCtor, name, name);
            }
        }
        else
        {
            m_writer.Write("%s", name);
        }
    }
    else if (expression->nodeType == HLSLNodeType_CastingExpression)
    {
        HLSLCastingExpression* castingExpression = static_cast<HLSLCastingExpression*>(expression);
        m_writer.Write("(");
        OutputDeclaration(castingExpression->type, "");
        m_writer.Write(")(");
        OutputExpression(castingExpression->expression);
        m_writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_ConstructorExpression)
    {
        HLSLConstructorExpression* constructorExpression = static_cast<HLSLConstructorExpression*>(expression);
        m_writer.Write("%s(", GetTypeName(constructorExpression->type));
        OutputExpressionList(constructorExpression->argument);
        m_writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_LiteralExpression)
    {
        HLSLLiteralExpression* literalExpression = static_cast<HLSLLiteralExpression*>(expression);
        switch (literalExpression->type)
        {
        case HLSLBaseType_Half:
        case HLSLBaseType_Float:
            {
                // Don't use printf directly so that we don't use the system locale.
                char buffer[64];
                String_FormatFloat(buffer, sizeof(buffer), literalExpression->fValue);
                m_writer.Write("%s", buffer);
            }
            break;        
        case HLSLBaseType_Int:
            m_writer.Write("%d", literalExpression->iValue);
            break;
        case HLSLBaseType_Bool:
            m_writer.Write("%s", literalExpression->bValue ? "true" : "false");
            break;
        default:
            ASSERT(0);
        }
    }
    else if (expression->nodeType == HLSLNodeType_UnaryExpression)
    {
        HLSLUnaryExpression* unaryExpression = static_cast<HLSLUnaryExpression*>(expression);
        const char* op = "?";
        bool pre = true;
        switch (unaryExpression->unaryOp)
        {
        case HLSLUnaryOp_Negative:      op = "-";  break;
        case HLSLUnaryOp_Positive:      op = "+";  break;
        case HLSLUnaryOp_Not:           op = "!";  break;
        case HLSLUnaryOp_PreIncrement:  op = "++"; break;
        case HLSLUnaryOp_PreDecrement:  op = "++"; break;
        case HLSLUnaryOp_PostIncrement: op = "++"; pre = false; break;
        case HLSLUnaryOp_PostDecrement: op = "--"; pre = false; break;
        }
        m_writer.Write("(");
        if (pre)
        {
            m_writer.Write("%s", op);
            OutputExpression(unaryExpression->expression);
        }
        else
        {
            OutputExpression(unaryExpression->expression);
            m_writer.Write("%s", op);
        }
        m_writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_BinaryExpression)
    {
        HLSLBinaryExpression* binaryExpression = static_cast<HLSLBinaryExpression*>(expression);
        m_writer.Write("(");
        OutputExpression(binaryExpression->expression1);
        const char* op = "?";
        switch (binaryExpression->binaryOp)
        {
        case HLSLBinaryOp_Add:          op = " + "; break;
        case HLSLBinaryOp_Sub:          op = " - "; break;
        case HLSLBinaryOp_Mul:          op = " * "; break;
        case HLSLBinaryOp_Div:          op = " / "; break;
        case HLSLBinaryOp_Less:         op = " < "; break;
        case HLSLBinaryOp_Greater:      op = " > "; break;
        case HLSLBinaryOp_LessEqual:    op = " <= "; break;
        case HLSLBinaryOp_GreaterEqual: op = " >= "; break;
        case HLSLBinaryOp_Equal:        op = " == "; break;
        case HLSLBinaryOp_NotEqual:     op = " != "; break;
        case HLSLBinaryOp_Assign:       op = " = "; break;
        case HLSLBinaryOp_AddAssign:    op = " += "; break;
        case HLSLBinaryOp_SubAssign:    op = " -= "; break;
        case HLSLBinaryOp_MulAssign:    op = " *= "; break;
        case HLSLBinaryOp_DivAssign:    op = " /= "; break;
        case HLSLBinaryOp_And:          op = " && "; break;
        case HLSLBinaryOp_Or:           op = " || "; break;
        default:
            ASSERT(0);
        }
        m_writer.Write("%s", op);
        OutputExpression(binaryExpression->expression2);
        m_writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_ConditionalExpression)
    {
        HLSLConditionalExpression* conditionalExpression = static_cast<HLSLConditionalExpression*>(expression);
        m_writer.Write("((");
        OutputExpression(conditionalExpression->condition);
        m_writer.Write(")?(");
        OutputExpression(conditionalExpression->trueExpression);
        m_writer.Write("):(");
        OutputExpression(conditionalExpression->falseExpression);
        m_writer.Write("))");
    }
    else if (expression->nodeType == HLSLNodeType_MemberAccess)
    {
        HLSLMemberAccess* memberAccess = static_cast<HLSLMemberAccess*>(expression);
        m_writer.Write("(");
        OutputExpression(memberAccess->object);
        m_writer.Write(").%s", memberAccess->field);
    }
    else if (expression->nodeType == HLSLNodeType_ArrayAccess)
    {
        HLSLArrayAccess* arrayAccess = static_cast<HLSLArrayAccess*>(expression);
        OutputExpression(arrayAccess->array);
        m_writer.Write("[");
        OutputExpression(arrayAccess->index);
        m_writer.Write("]");
    }
    else if (expression->nodeType == HLSLNodeType_FunctionCall)
    {
        HLSLFunctionCall* functionCall = static_cast<HLSLFunctionCall*>(expression);
        const char* name = functionCall->function->name;
        if (!m_legacy)
        {
            if (String_Equal(name, "tex2D"))
            {
                name = m_tex2DFunction;
            }
            else if (String_Equal(name, "tex2Dproj"))
            {
                name = m_tex2DProjFunction;
            }
            else if (String_Equal(name, "tex2Dlod"))
            {
                name = m_tex2DLodFunction;
            }
            else if (String_Equal(name, "texCUBE"))
            {
                name = m_texCubeFunction;
            }
            else if (String_Equal(name, "texCUBEbias"))
            {
                name = m_texCubeBiasFunction;
            }
        }
        m_writer.Write("%s(", name);
        OutputExpressionList(functionCall->argument);
        m_writer.Write(")");
    }
    else
    {
        m_writer.Write("<unknown expression>");
    }
}

void HLSLGenerator::OutputArguments(HLSLArgument* argument)
{
    int numArgs = 0;
    while (argument != NULL)
    {
        if (numArgs > 0)
        {
            m_writer.Write(", ");
        }

        switch (argument->modifier)
        {
        case HLSLArgumentModifier_In:
            m_writer.Write("in ");
            break;
        case HLSLArgumentModifier_Inout:
            m_writer.Write("inout ");
            break;
        case HLSLArgumentModifier_Uniform:
            m_writer.Write("uniform ");
            break;
        }

        OutputDeclaration(argument->type, argument->name, argument->semantic);
        argument = argument->nextArgument;
        ++numArgs;
    }
}

void HLSLGenerator::OutputStatements(int indent, HLSLStatement* statement)
{

    while (statement != NULL)
    {

        if (statement->nodeType == HLSLNodeType_Declaration)
        {
            HLSLDeclaration* declaration = static_cast<HLSLDeclaration*>(statement);
            m_writer.BeginLine(indent, declaration->fileName, declaration->line);
            OutputDeclaration(declaration);
            m_writer.EndLine(";");
        }
        else if (statement->nodeType == HLSLNodeType_Struct)
        {
            HLSLStruct* structure = static_cast<HLSLStruct*>(statement);
            m_writer.WriteLine(indent, "struct %s {", structure->name);
            HLSLStructField* field = structure->field;
            while (field != NULL)
            {
                m_writer.BeginLine(indent + 1, field->fileName, field->line);
                OutputDeclaration(field->type, field->name, field->semantic);
                m_writer.Write(";");
                m_writer.EndLine();
                field = field->nextField;
            }
            m_writer.WriteLine(indent, "};");
        }
        else if (statement->nodeType == HLSLNodeType_Buffer)
        {
            HLSLBuffer* buffer = static_cast<HLSLBuffer*>(statement);
            HLSLBufferField* field = buffer->field;

            if (!m_legacy)
            {
                m_writer.BeginLine(indent, buffer->fileName, buffer->line);
                m_writer.Write("cbuffer %s", buffer->name);
                if (buffer->registerName != NULL)
                {
                    m_writer.Write(" : register(%s)", buffer->registerName);
                }
                m_writer.EndLine(" {");
            }

            while (field != NULL)
            {
                m_writer.BeginLine(indent + 1, field->fileName, field->line);
                OutputDeclaration(field->type, field->name);
                m_writer.Write(";");
                m_writer.EndLine();
                field = field->nextField;
            }

            if (!m_legacy)
            {
                m_writer.WriteLine(indent, "};");
            }
        }
        else if (statement->nodeType == HLSLNodeType_Function)
        {
            HLSLFunction* function = static_cast<HLSLFunction*>(statement);

            // Use an alternate name for the function which is supposed to be entry point
            // so that we can supply our own function which will be the actual entry point.
            const char* functionName   = function->name;
            const char* returnTypeName = GetTypeName(function->returnType);

            m_writer.BeginLine(indent, function->fileName, function->line);
            m_writer.Write("%s %s(", returnTypeName, functionName);

            OutputArguments(function->argument);

            if (function->semantic != NULL)
            {
                m_writer.Write(") : %s {", function->semantic);
            }
            else
            {
                m_writer.Write(") {");
            }

            m_writer.EndLine();

            OutputStatements(indent + 1, function->statement);
            m_writer.WriteLine(indent, "};");


        }
        else if (statement->nodeType == HLSLNodeType_ExpressionStatement)
        {
            HLSLExpressionStatement* expressionStatement = static_cast<HLSLExpressionStatement*>(statement);
            m_writer.BeginLine(indent, statement->fileName, statement->line);
            OutputExpression(expressionStatement->expression);
            m_writer.EndLine(";");
        }
        else if (statement->nodeType == HLSLNodeType_ReturnStatement)
        {
            HLSLReturnStatement* returnStatement = static_cast<HLSLReturnStatement*>(statement);
            if (returnStatement->expression != NULL)
            {
                m_writer.BeginLine(indent, returnStatement->fileName, returnStatement->line);
                m_writer.Write("return ");
                OutputExpression(returnStatement->expression);
                m_writer.EndLine(";");
            }
            else
            {
                m_writer.WriteLine(indent, returnStatement->fileName, returnStatement->line, "return;");
            }
        }
        else if (statement->nodeType == HLSLNodeType_DiscardStatement)
        {
            HLSLDiscardStatement* discardStatement = static_cast<HLSLDiscardStatement*>(statement);
            m_writer.WriteLine(indent, discardStatement->fileName, discardStatement->line, "discard;");
        }
        else if (statement->nodeType == HLSLNodeType_BreakStatement)
        {
            HLSLBreakStatement* breakStatement = static_cast<HLSLBreakStatement*>(statement);
            m_writer.WriteLine(indent, breakStatement->fileName, breakStatement->line, "break;");
        }
        else if (statement->nodeType == HLSLNodeType_ContinueStatement)
        {
            HLSLContinueStatement* continueStatement = static_cast<HLSLContinueStatement*>(statement);
            m_writer.WriteLine(indent, continueStatement->fileName, continueStatement->line, "continue;");
        }
        else if (statement->nodeType == HLSLNodeType_IfStatement)
        {
            HLSLIfStatement* ifStatement = static_cast<HLSLIfStatement*>(statement);
            m_writer.BeginLine(indent, ifStatement->fileName, ifStatement->line);
            m_writer.Write("if (");
            OutputExpression(ifStatement->condition);
            m_writer.Write(") {");
            m_writer.EndLine();
            OutputStatements(indent + 1, ifStatement->statement);
            m_writer.WriteLine(indent, "}");
            if (ifStatement->elseStatement != NULL)
            {
                m_writer.WriteLine(indent, "else {");
                OutputStatements(indent + 1, ifStatement->elseStatement);
                m_writer.WriteLine(indent, "}");
            }
        }
        else if (statement->nodeType == HLSLNodeType_ForStatement)
        {
            HLSLForStatement* forStatement = static_cast<HLSLForStatement*>(statement);
            m_writer.BeginLine(indent, forStatement->fileName, forStatement->line);
            m_writer.Write("for (");
            OutputDeclaration(forStatement->initialization);
            m_writer.Write("; ");
            OutputExpression(forStatement->condition);
            m_writer.Write("; ");
            OutputExpression(forStatement->increment);
            m_writer.Write(") {");
            m_writer.EndLine();
            OutputStatements(indent + 1, forStatement->statement);
            m_writer.WriteLine(indent, "}");
        }
        else
        {
            // Unhanded statement type.
            ASSERT(0);
        }

        statement = statement->nextStatement;

    }

}

void HLSLGenerator::OutputDeclaration(HLSLDeclaration* declaration)
{

    if (!m_legacy && GetIsSamplerType(declaration->type))
    {
        int reg = -1;
        if (declaration->registerName != NULL)
        {
            sscanf(declaration->registerName, "s%d", &reg);
        }

        const char* textureType = NULL;
        if (declaration->type.baseType == HLSLBaseType_Sampler2D)
        {
            textureType = "Texture2D";
        }
        else if (declaration->type.baseType == HLSLBaseType_SamplerCube)
        {
            textureType = "TextureCube";
        }

        if (reg != -1)
        {
            m_writer.Write("%s %s_texture : register(t%d); SamplerState %s_sampler : register(s%d)", textureType, declaration->name, reg, declaration->name, reg);
        }
        else
        {
            m_writer.Write("%s %s_texture; SamplerState %s_sampler", textureType, declaration->name, declaration->name);
        }
        return;
    }


    OutputDeclaration(declaration->type, declaration->name);
    // Registers only really matter for our samplers.
    if (GetIsSamplerType(declaration->type) && declaration->registerName != NULL)
    {
        m_writer.Write(" : register(%s)", declaration->registerName);
    }
    if (declaration->assignment != NULL)
    {
        m_writer.Write(" = ");
        if (declaration->type.array)
        {
            m_writer.Write("{ ");
            OutputExpressionList(declaration->assignment);
            m_writer.Write(" }");
        }
        else
        {
            OutputExpression(declaration->assignment);
        }
    }
}

void HLSLGenerator::OutputDeclaration(const HLSLType& type, const char* name, const char* semantic)
{
    const char* typeName = GetTypeName(type);
    if (!m_legacy)
    {
        if (type.baseType == HLSLBaseType_Sampler2D)
        {
            typeName = m_textureSampler2DStruct;
        }
        else if (type.baseType == HLSLBaseType_SamplerCube)
        {
            typeName = m_textureSamplerCubeStruct;
        }
    }

    if (type.constant)
    {
        m_writer.Write("const ");
    }
    if (!type.array)
    {
        if (semantic == NULL)
        {
            m_writer.Write("%s %s", typeName, name);
        }
        else
        {
            m_writer.Write("%s %s : %s", typeName, name, semantic);
        }
    }
    else
    {
        ASSERT(semantic == NULL);
        m_writer.Write("%s %s[", typeName, name);
        if (type.arraySize != NULL)
        {
            OutputExpression(type.arraySize);
        }
        m_writer.Write("]");
    }
}

bool HLSLGenerator::ChooseUniqueName(const char* base, char* dst, int dstLength) const
{
    for (int i = 0; i < 1024; ++i)
    {
        String_Printf(dst, dstLength, "%s%d", base, i);
        if (!m_tree->GetContainsString(dst))
        {
            return true;
        }
    }
    return false;
}

}