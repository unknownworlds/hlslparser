#include "Engine/Allocator.h"
#include "Engine/Log.h"
#include "Engine/String.h"

#include "HLSLParser.h"
#include "GLSLGenerator.h"

#include <fstream>
#include <sstream>
#include <iostream>

std::string ReadFile(const char* fileName)
{
    std::ifstream ifs(fileName);
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

void PrintUsage()
{
    std::cerr << "usage: hlslparser [-h] [-fs | -vs] FILENAME ENTRYNAME\n"
              << "\n"
              << "Translate HLSL shader to GLSL shader.\n"
              << "\n"
              << "positional arguments:\n"
              << " FILENAME    input file name\n"
              << " ENTRYNAME   entry point of the shader\n"
              << "\n"
              << "optional arguments:\n"
              << " -h, --help  show this help message and exit\n"
              << " -fs         generate fragment shader (default)\n"
              << " -vs         generate vertex shader\n";
}

int main(int argc, char* argv[])
{
    using namespace M4;

    // Parse arguments
    const char* fileName = NULL;
    const char* entryName = NULL;
    GLSLGenerator::Target target = GLSLGenerator::Target_FragmentShader;

    for (int argn = 1; argn < argc; ++argn)
    {
        const char* const arg = argv[argn];

        if (String_Equal(arg, "-h") || String_Equal(arg, "--help"))
        {
            PrintUsage();
            return 0;
        }
        else if (String_Equal(arg, "-fs"))
        {
           target = GLSLGenerator::Target_FragmentShader;
        }
        else if (String_Equal(arg, "-vs"))
        {
            target = GLSLGenerator::Target_VertexShader;
        }
        else if (fileName == NULL)
        {
            fileName = arg;
        }
        else if (entryName == NULL)
        {
            entryName = arg;
        }
        else
        {
            Log_Error("Too many arguments");
            PrintUsage();
            return 1;
        }
    }

    if (fileName == NULL || entryName == NULL)
    {
        Log_Error("Missing arguments");
        PrintUsage();
        return 1;
    }

    // Read input file
    const std::string source = ReadFile(fileName);

    // Parse input file
    Allocator allocator;
    HLSLParser parser(&allocator, fileName, source.data(), source.size());
    HLSLTree tree(&allocator);
    if (!parser.Parse(&tree))
    {
        Log_Error("Parsing failed, aborting");
        return 1;
    }

    // Generate output
    GLSLGenerator generator(&allocator);
    generator.Generate(&tree, target, entryName);
    std::cout << generator.GetResult();

    return 0;
}
