// RBXInstance.hpp
#ifndef RBXINSTANCE_HPP
#define RBXINSTANCE_HPP
#pragma once

#include <string>
#include <vector>
#include <optional>

class RBXInstance {
public:
    std::uint64_t self;

    struct vector2_t final { float x, y; };
    struct vector3_t final { float x, y, z; };
    struct quaternion final { float x, y, z, w; };
    struct matrix4_t final { float data[16]; };

    std::string name();
    std::string class_name();
    std::vector<RBXInstance> children();
    RBXInstance FindFirstChild(std::string child);
    RBXInstance FindFirstChildOfClass(std::string child);

    void spoof(RBXInstance script);

    void SetBytecode(std::vector<uint8_t> bytes, int bytecode_size);

    std::string Bytecode();

    void SetModuleBypass();

    void set_scriptable();

    std::uint64_t get_class_descriptor();

    void SetBoolValue(bool num) const;

    void SetCoreBypass(bool Boolean);
};

#endif // RBXINSTANCE_HPP