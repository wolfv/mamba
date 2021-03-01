#include "mamba/virtual_packages.hpp"
#include "mamba/context.hpp"
#include "mamba/util.hpp"
#include "mamba/output.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include "dlfcn.h"
#endif

#include <vector>


namespace mamba
{
    namespace detail
    {
        std::string macos_version()
        {
            std::string out, err;
            // Note: we could also inspect /System/Library/CoreServices/SystemVersion.plist which is
            // an XML file
            //       that contains the same information. However, then we'd either need an xml
            //       parser or some other crude method to read the data
            std::vector<std::string> args = { "sw_vers", "-productVersion" };
            auto [status, ec] = reproc::run(
                args, reproc::options{}, reproc::sink::string(out), reproc::sink::string(err));

            if (ec)
            {
                LOG_ERROR
                    << "Could not find macOS version by calling 'sw_vers -productVersion'\nPlease file a bug report.\nError: "
                    << ec.message();
                return "";
            }
            return std::string(strip(out));
        }

        std::string cuda_version()
        {
#ifdef _WIN32
            std::string lib_filename = "nvcuda.dll";
            // TODO: implement for Windows

#else
            std::vector<std::string> lib_filenames;

            if (on_linux)
            {
                lib_filenames = {
                    "libcuda.so",                           // check library path first
                    "/usr/lib64/nvidia/libcuda.so",         // Redhat/CentOS/Fedora
                    "/usr/lib/x86_64-linux-gnu/libcuda.so"  // Ubuntu
                };
            }
            else if (on_mac)
            {
                lib_filenames = { "libcuda.dylib",  // check library path first
                                  "/usr/local/cuda/lib/libcuda.dylib" };
            }

            for (auto lib : lib_filenames)
            {
                void* handle = dlopen(lib.c_str(), RTLD_NOW);
                if (handle)
                {
                    LOG_DEBUG << "CUDA DLL found at " << lib;
                    dlerror();

                    int ret;

                    int (*cuInit)(u_int);
                    *(void**) (&cuInit) = dlsym(handle, "cuInit");

                    u_int flags = 0;
                    ret = cuInit(flags);
                    if (ret != 0)
                    {
                        LOG_WARNING << "Failed to init CUDA driver API (skipped)";
                        return "";
                    }
                    LOG_DEBUG << "CUDA driver API initialized";

                    int (*cuVersion)(int*);
                    *(void**) (&cuVersion) = dlsym(handle, "cuDriverGetVersion");

                    int version = 0;
                    ret = cuVersion(&version);
                    if (ret != 0)
                    {
                        LOG_WARNING << "Failed to get CUDA version";
                        return "";
                    }
                    std::string version_str = std::to_string(version / 1000) + "."
                                              + std::to_string((version % 1000) / 10);

                    LOG_DEBUG << "CUDA driver version found: " << version_str;

                    dlclose(handle);
                    return version_str;
                }
            }
#endif
            return "";
        }

        PackageInfo make_virtual_package(const std::string& name,
                                         const std::string& version,
                                         const std::string& build_string)
        {
            PackageInfo res(name);
            res.version = version.size() ? version : "0";
            res.build_string = build_string.size() ? build_string : "0";
            res.build_number = 0;
            res.channel = "@";
            // res.subdir = Context::instance().subdir;
            res.subdir = "osx-64";  // TODO: fix this
            res.md5 = "12345678901234567890123456789012";
            res.fn = name;
            return res;
        }

        std::vector<PackageInfo> dist_packages()
        {
            std::vector<PackageInfo> res;
            if (on_win)
            {
                res.push_back(make_virtual_package("__win"));
            }
            if (on_linux || on_mac)
            {
                res.push_back(make_virtual_package("__unix"));
            }
            if (on_linux)
            {
                res.push_back(make_virtual_package("__linux"));
            }
            if (on_mac)
            {
                res.push_back(make_virtual_package("__osx", macos_version()));
            }
            return res;
        }
    }

    std::vector<PackageInfo> get_virtual_packages()
    {
        auto res = detail::dist_packages();

        auto cuda_ver = detail::cuda_version();
        if (cuda_ver != "")
        {
            res.push_back(detail::make_virtual_package("__cuda", cuda_ver));
        }

        return res;
    }
}
