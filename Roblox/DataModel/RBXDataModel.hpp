#pragma once

#include <cstdint>
#include <fstream>

class RBXDataModel {
    static RBXDataModel* g_Singleton;

public:
    static RBXDataModel* get_singleton() noexcept;

    std::uint64_t datamodel;

    std::uint64_t get_datamodel();

    uint64_t get_renderView();
};
