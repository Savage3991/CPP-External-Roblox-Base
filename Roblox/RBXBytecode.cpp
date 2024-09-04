#include "RBXBytecode.hpp"
#include "Luau/BytecodeBuilder.h"
#include "Luau/BytecodeUtils.h"
#include "Luau/Compiler.h"
#include "zstd.h"
#include "xxhash.h"
#include <cstdint>
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <Psapi.h>

class bytecode_encoder_t : public Luau::BytecodeEncoder {
    inline void encode(uint32_t* data, size_t count) override {

        for (auto i = 0u; i < count;) {

            auto& opcode = *reinterpret_cast<uint8_t*>(data + i);

            i += Luau::getOpLength(LuauOpcode(opcode));

            opcode *= 227;
        }
    }
};

RBXBytecode* RBXBytecode::g_Singleton = nullptr;

RBXBytecode* RBXBytecode::get_singleton() noexcept {
    if (g_Singleton == nullptr)
        g_Singleton = new RBXBytecode();
    return g_Singleton;
}

std::string RBXBytecode::compress_bytecode(std::string uncompressed_bytecode) {
    const auto data_size = uncompressed_bytecode.size();
    const auto max_size = ZSTD_compressBound(data_size);
    auto buffer = std::vector<char>(max_size + 8);

    strcpy_s(&buffer[0], buffer.capacity(), "RSB1");
    memcpy_s(&buffer[4], buffer.capacity(), &data_size, sizeof(data_size));

    const auto compressed_size = ZSTD_compress(&buffer[8], max_size, uncompressed_bytecode.data(), data_size, ZSTD_maxCLevel());
    if (ZSTD_isError(compressed_size))
        throw std::runtime_error("Failed to compress the bytecode.");

    const auto size = compressed_size + 8;
    const auto key = XXH32(buffer.data(), size, 42u);
    const auto bytes = reinterpret_cast<const uint8_t*>(&key);

    for (auto i = 0u; i < size; ++i)
        buffer[i] ^= bytes[i % 4] + i * 41u;

    return std::string(buffer.data(), size);
}

std::string RBXBytecode::compile_to_bytecode(std::string script)
{
    static auto encoder = bytecode_encoder_t();
    std::string compiled = Luau::compile(script, {}, {}, &encoder);
    std::string compressed = this->compress_bytecode(compiled);

    return compressed;
}

std::string RBXBytecode::Decompress(const std::string& source) noexcept {
    const uint8_t kBytecodeMagic[4] = { 'R', 'S', 'B', '1' };
    const int kBytecodeHashMultiplier = 41;
    const int kBytecodeHashSeed = 42;

    try {
        std::vector<uint8_t> ss(source.begin(), source.end());
        std::vector<uint8_t> hb(4);

        for (size_t i = 0; i < 4; ++i) {
            hb[i] = ss[i] ^ kBytecodeMagic[i];
            hb[i] = (hb[i] - i * kBytecodeHashMultiplier) % 256;
        }

        for (size_t i = 0; i < ss.size(); ++i) {
            ss[i] ^= (hb[i % 4] + i * kBytecodeHashMultiplier) % 256;
        }

        uint32_t hash_value = 0;
        for (size_t i = 0; i < 4; ++i) {
            hash_value |= hb[i] << (i * 8);
        }

        uint32_t rehash = XXH32(ss.data(), ss.size(), kBytecodeHashSeed);
        if (rehash != hash_value) {
            throw std::runtime_error("Failed to decompress bytecode. (1)");
        }

        uint32_t decompressed_size = 0;
        for (size_t i = 4; i < 8; ++i) {
            decompressed_size |= ss[i] << ((i - 4) * 8);
        }

        std::vector<uint8_t> compressed_data(ss.begin() + 8, ss.end());
        std::vector<uint8_t> decompressed(decompressed_size);

        size_t const decompressed_size_actual = ZSTD_decompress(decompressed.data(), decompressed_size, compressed_data.data(), compressed_data.size());
        if (ZSTD_isError(decompressed_size_actual)) {
            throw std::runtime_error("Failed to decompress bytecode. (2)");
        }

        decompressed.resize(decompressed_size_actual);
        return std::string(decompressed.begin(), decompressed.end());

    }
    catch (const std::exception& e) {
        return "failed to decompress bytecode";
    }
}