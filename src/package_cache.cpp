#include "nlohmann/json.hpp"

#include "validate.hpp"
#include "package_cache.hpp"

namespace mamba
{
    PackageCacheData::PackageCacheData(const fs::path& pkgs_dir)
        : m_pkgs_dir(pkgs_dir)
    {
    }

    bool PackageCacheData::create_directory()
    {
        try
        {
            LOG_INFO << "Attempt to create package cache directory " << m_pkgs_dir;
            bool sudo_safe = path::starts_with_home(m_pkgs_dir);
            path::touch(m_pkgs_dir / PACKAGE_CACHE_MAGIC_FILE, sudo_safe, /*mkdir*/true);
            // TODO why magic string "urls" here? is it necessary?
            path::touch(m_pkgs_dir / "urls", sudo_safe);
            return true;
        }
        catch(...)
        {
            // TODO better error handling
            LOG_ERROR << "cannot create package cache directory " << m_pkgs_dir;
            return false;
        }
    }

    void PackageCacheData::set_writable(Writable writable)
    {
        m_writable = writable;
    }

    auto PackageCacheData::is_writable() -> Writable
    {
        if (m_writable == UNKNOWN)
        {
            check_writable();
        }
        return m_writable;
    }

    PackageCacheData
    PackageCacheData::first_writable(const std::vector<fs::path>* pkgs_dirs)
    {
        const std::vector<fs::path>* dirs = pkgs_dirs ? pkgs_dirs : &Context::instance().pkgs_dirs;
        for (const auto& dir : (*dirs))
        {
            LOG_INFO << "Checking dir " << dir;
            PackageCacheData pkgs_cache(dir);
            auto is_wri = pkgs_cache.is_writable();

            if (is_wri == WRITABLE)
            {
                return pkgs_cache;
            }
            else if (is_wri == DIR_DOES_NOT_EXIST)
            {
                bool created = pkgs_cache.create_directory();
                if (created)
                {
                    pkgs_cache.set_writable(WRITABLE);
                    return pkgs_cache;
                }
            }
        }
        // TODO better error class?!
        throw std::runtime_error("Did not find a writable package cache directory!");
    }

    void PackageCacheData::check_writable()
    {
        fs::path magic_file = m_pkgs_dir / PACKAGE_CACHE_MAGIC_FILE;
        if (fs::is_regular_file(magic_file))
        {
            LOG_INFO << magic_file << " exists, checking if writable";
            if (path::is_writable(magic_file))
            {
                LOG_INFO << magic_file << " writable";
                m_writable = WRITABLE;
            }
            else
            {
                m_writable = NOT_WRITABLE;
                LOG_INFO << magic_file << " not writable";
            }
        }
        else
        {
            LOG_INFO << magic_file << " does not exists";
            m_writable = DIR_DOES_NOT_EXIST;
        }
    }

    bool PackageCacheData::query(const PackageInfo& s)
    {
        std::string pkg = s.str();
        if (m_valid_cache.find(pkg) != m_valid_cache.end())
        {
            return m_valid_cache[pkg];
        }

        assert (!s.fn.empty());
        // TODO move entire cache checking logic here (including md5 sum check)
        bool valid = false;
        if (fs::exists(m_pkgs_dir / s.fn))
        {
            fs::path tarball_path = m_pkgs_dir / s.fn;
            // validate that this tarball has the right size and MD5 sum
            valid = validate::file_size(tarball_path, s.size);
            valid = valid && validate::md5(tarball_path, s.md5);
            LOG_INFO << tarball_path << " is " << valid;
            m_valid_cache[pkg] = valid;
        }
        else if (fs::exists(m_pkgs_dir / strip_package_name(s.fn)))
        {
            auto repodata_record_path = m_pkgs_dir / strip_package_name(s.fn) / "info" / "repodata_record.json";
            if (fs::exists(repodata_record_path))
            {
                try
                {
                    std::ifstream repodata_record_f(repodata_record_path);
                    nlohmann::json repodata_record;
                    repodata_record_f >> repodata_record;
                    valid = repodata_record["size"].get<std::size_t>() == s.size;
                    valid = valid && repodata_record["sha256"].get<std::string>() == s.sha256;
                    valid = valid && repodata_record["channel"].get<std::string>() == s.channel;
                    valid = valid && repodata_record["url"].get<std::string>() == s.url;
                    if (!valid)
                    {
                        LOG_WARNING << "Found directory with same name, but different size, channel, url or checksum " << repodata_record_path;
                    }
                }
                catch (...)
                {
                    LOG_WARNING << "Found corrupted repodata_record file " << repodata_record_path;
                    valid = false;
                }
            }
        }
        m_valid_cache[pkg] = valid;
        return valid;
    }

    MultiPackageCache::MultiPackageCache(const std::vector<fs::path>& cache_paths)
    {
        m_caches.reserve(cache_paths.size());
        for (auto& c : cache_paths)
        {
            m_caches.emplace_back(c);
        }
    }

    PackageCacheData& MultiPackageCache::first_writable()
    {
        for (auto& pc : m_caches)
        {
            auto status = pc.is_writable();
            if (status == PackageCacheData::WRITABLE)
            {
                return pc;
            }
            else if (status == PackageCacheData::DIR_DOES_NOT_EXIST)
            {
                bool created = pc.create_directory();
                if (created)
                {
                    pc.set_writable(PackageCacheData::WRITABLE);
                    return pc;
                }
            }
        }
        // TODO better error class?!
        throw std::runtime_error("Did not find a writable package cache directory!");
    }

    bool MultiPackageCache::query(const PackageInfo& s)
    {
        for (auto& c : m_caches)
        {
            if (c.query(s)) return true;
        }
        return false;
    }
}
