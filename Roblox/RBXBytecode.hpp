#pragma once
#include <iostream>

class RBXBytecode
{
public:
    static RBXBytecode* get_singleton() noexcept;

    std::string compress_bytecode(std::string uncompressed_bytecode);
    std::string compile_to_bytecode(std::string script);

    static std::string Decompress(const std::string &source) noexcept;

private:
    static RBXBytecode *g_Singleton;
};
