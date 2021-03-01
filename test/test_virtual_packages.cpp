#include "mamba/util.hpp"
#include "mamba/virtual_packages.hpp"

#include <gtest/gtest.h>


namespace mamba
{
    namespace testing
    {
        TEST(virtual_packages, make_virtual_package)
        {
            auto pkg = detail::make_virtual_package("test", "0.1.5", "abcd");

            EXPECT_EQ(pkg.name, "test");
            EXPECT_EQ(pkg.version, "0.1.5");
            EXPECT_EQ(pkg.build_string, "abcd");
            EXPECT_EQ(pkg.build_number, 0);
            EXPECT_EQ(pkg.channel, "@");
            // EXPECT_EQ(pkg.subdir, "osx-64");  // TODO: fix this
            EXPECT_EQ(pkg.md5, "12345678901234567890123456789012");
            EXPECT_EQ(pkg.fn, pkg.name);
        }

        TEST(virtual_packages, dist_packages)
        {
            auto pkgs = detail::dist_packages();

            if (on_win)
            {
                EXPECT_EQ(pkgs.size(), 1);
            }
            if (on_linux)
            {
                EXPECT_EQ(pkgs.size(), 2);
            }
            if (on_mac)
            {
                EXPECT_EQ(pkgs.size(), 2);
            }
        }

        TEST(virtual_packages, get_virtual_packages)
        {
            auto pkgs = get_virtual_packages();
            int pkgs_count;

            if (on_win)
            {
                pkgs_count = 1;
                EXPECT_EQ(pkgs[0].name, "__win");
            }
            if (on_linux)
            {
                pkgs_count = 2;
                EXPECT_EQ(pkgs[0].name, "__unix");
                EXPECT_EQ(pkgs[1].name, "__linux");
            }
            if (on_mac)
            {
                pkgs_count = 2;
                EXPECT_EQ(pkgs[0].name, "__unix");
                EXPECT_EQ(pkgs[1].name, "__osx");
            }

            if (detail::cuda_version() != "")
            {
                EXPECT_EQ(pkgs[pkgs_count++].name, "__cuda");
            }
        }
    }
}
