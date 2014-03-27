//=============================================================================
//
// Render/CodeWriter.h
//
// Created by Max McGuire (max@unknownworlds.com)
// Copyright (c) 2013, Unknown Worlds Entertainment, Inc.
//
//=============================================================================

#ifndef CODE_WRITER_H
#define CODE_WRITER_H

#include <string>

namespace M4
{

class Allocator;

/**
 * This class is used for outputting code. It handles indentation and inserting #line markers
 * to match the desired output line numbers.
 */
class CodeWriter
{

public:

    explicit CodeWriter(Allocator* allocator);

    void BeginLine(int indent, const char* fileName = NULL, int lineNumber = -1);
    void Write(const char* format, ...);
    void EndLine(const char* text = NULL);

    void WriteLine(int indent, const char* format, ...);
    void WriteLine(int indent, const char* fileName, int lineNumber, const char* format, ...);

    const char* GetResult() const;

private:

    Allocator*      m_allocator;
    std::string     m_buffer;
    int             m_currentLine;
    const char*     m_currentFileName;
    int             m_spacesPerIndent;
    bool            m_writeLines;
    bool            m_writeFileNames;

};

}

#endif