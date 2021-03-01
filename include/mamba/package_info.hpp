// Copyright (c) 2019, QuantStack and Mamba Contributors
//
// Distributed under the terms of the BSD 3-Clause License.
//
// The full license is in the file LICENSE, distributed with this software.

#ifndef MAMBA_PACKAGE_INFO
#define MAMBA_PACKAGE_INFO

extern "C"
{
#include <solv/pool.h>
#include <solv/poolid.h>
#include <solv/repo.h>
#include <solv/solvable.h>
}

#include <string>
#include <vector>

#include "nlohmann/json.hpp"

namespace mamba
{
    enum PackageType
    {
        // NONE,
        NOARCH_GENERIC,
        NOARCH_PYTHON,
        VIRTUAL_SYSTEM
    };

    class PackageInfo
    {
    public:
        using field_getter = std::function<std::string(const PackageInfo&)>;
        using compare_fun = std::function<bool(const PackageInfo&, const PackageInfo&)>;

        static field_getter get_field_getter(const std::string& name);
        static compare_fun less(const std::string& member);
        static compare_fun equal(const std::string& member);

        PackageInfo(Solvable* s);
        PackageInfo(nlohmann::json&& j);
        PackageInfo(const std::string& name);
        PackageInfo(const std::string& name,
                    const std::string& version,
                    const std::string build_string,
                    std::size_t build_number);

        nlohmann::json json() const;
        std::string str() const;
        std::string long_str() const;

        PackageType package_type;
        std::string name;
        std::string version;
        std::string build_string;
        std::size_t build_number = 0;
        std::string channel;
        std::string url;
        std::string subdir;
        std::string fn;
        std::string license;
        std::size_t size = 0;
        std::size_t timestamp = 0;
        std::string md5;
        std::string sha256;
        std::vector<std::string> depends;
        std::vector<std::string> constrains;
    };
}  // namespace mamba

#endif
