//=============================================================================
//
// Render/HLSLParser.h
//
// Created by Max McGuire (max@unknownworlds.com)
// Copyright (c) 2013, Unknown Worlds Entertainment, Inc.
//
//=============================================================================

#ifndef HLSL_PARSER_H
#define HLSL_PARSER_H

#include "Engine/StringPool.h"
#include "Engine/Array.h"

#include "HLSLTokenizer.h"
#include "HLSLTree.h"

namespace M4
{

class HLSLParser
{

public:

    HLSLParser(Allocator* allocator, const char* fileName, const char* buffer, size_t length);

    bool Parse(HLSLTree* tree);

private:

    bool Accept(int token);
    bool Expect(int token);

    /**
     * Special form of Accept for accepting a word that is not actually a token
     * but will be treated like one. This is useful for HLSL keywords that are
     * only tokens in specific contexts (like in/inout in parameter lists).
     */
    bool Accept(const char* token);

    bool AcceptIdentifier(const char*& identifier);
    bool ExpectIdentifier(const char*& identifier);
    bool AcceptFloat(float& value);
    bool AcceptInt(int& value);
    bool AcceptType(bool allowVoid, HLSLBaseType& type, const char*& typeName, bool* constant);
    bool ExpectType(bool allowVoid, HLSLBaseType& type, const char*& typeName, bool* constant);
    bool AcceptBinaryOperator(int priority, HLSLBinaryOp& binaryOp);
    bool AcceptUnaryOperator(bool pre, HLSLUnaryOp& unaryOp);
    bool AcceptAssign(HLSLBinaryOp& binaryOp);

    /**
     * Handles a declaration like: "float2 name[5]". If allowUnsizedArray is true, it is
     * is acceptable for the declaration to not specify the bounds of the array (i.e. name[]).
     */
    bool AcceptDeclaration(bool allowUnsizedArray, HLSLType& type, const char*& name);
    bool ExpectDeclaration(bool allowUnsizedArray, HLSLType& type, const char*& name);

    bool ParseTopLevel(HLSLStatement*& statement);
    bool ParseBlock(HLSLStatement*& firstStatement);
    bool ParseStatementOrBlock(HLSLStatement*& firstStatement);
    bool ParseStatement(HLSLStatement*& statement);
    bool ParseDeclaration(HLSLDeclaration*& declaration);
    bool ParseFieldDeclaration(HLSLStructField*& field);
    bool ParseBufferFieldDeclaration(HLSLBufferField*& field);
    bool ParseExpression(HLSLExpression*& expression);
    bool ParseBinaryExpression(int priority, HLSLExpression*& expression);
    bool ParseTerminalExpression(HLSLExpression*& expression, bool& needsEndParen);
    bool ParseExpressionList(int endToken, bool allowEmptyEnd, HLSLExpression*& firstExpression, int& numExpressions);
    bool ParseArgumentList(int endToken, HLSLArgument*& firstArgument, int& numArguments);
    bool ParseDeclarationAssignment(HLSLDeclaration* declaration);
    bool ParsePartialConstructor(HLSLExpression*& expression, HLSLBaseType type, const char* typeName);

    bool CheckForUnexpectedEndOfStream(int endToken);

    const HLSLStruct* FindUserDefinedType(const char* name) const;

    void BeginScope();
    void EndScope();

    void DeclareVariable(const char* name, const HLSLType& type);

    /** Returned pointer is only valid until Declare or Begin/EndScope is called. */
    const HLSLType* FindVariable(const char* name, bool& global) const;

    const HLSLFunction* FindFunction(const char* name) const;
    
    bool GetIsFunction(const char* name) const;
    
    /** Finds the overloaded function that matches the specified call. */
    const HLSLFunction* MatchFunctionCall(const HLSLFunctionCall* functionCall, const char* name);

    /** Gets the type of the named field on the specified object type (fieldName can also specify a swizzle. ) */
    bool GetMemberType(const HLSLType& objectType, const char* fieldName, HLSLType& memberType);

    const char* GetFileName();
    int GetLineNumber() const;

private:

    struct Variable
    {
        const char*     name;
        HLSLType        type;
    };

    HLSLTokenizer           m_tokenizer;
    Array<HLSLStruct*>      m_userTypes;
    Array<Variable>         m_variables;
    Array<HLSLFunction*>    m_functions;
    int                     m_numGlobals;

    HLSLTree*               m_tree;

};

}

#endif