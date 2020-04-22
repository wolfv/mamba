#include <sstream>

#include "query.hpp"
#include "util.hpp"

namespace mamba
{
    void cut_repo_name(std::ostream& out, const std::string_view& reponame)
    {
        if (starts_with(reponame, "https://conda.anaconda.org/"))
        {
            out << reponame.substr(27, std::string::npos);
            return;
        }
        if (starts_with(reponame, "https://repo.anaconda.com/"))
        {
            out << reponame.substr(26, std::string::npos);
            return;
        }
        out << reponame;
    }

    void solvable_to_stream(std::ostream& out, Solvable* s)
    {
        auto* pool = s->repo->pool;
        cut_repo_name(out, s->repo->name);
        out << ": " << pool_id2str(pool, s->name) << " ("
            << pool_id2str(pool, s->evr) << ", " << solvable_lookup_str(s, SOLVABLE_BUILDFLAVOR) << ")\n";
    }

    /************************
     * Query implementation *
     ************************/
    
    Query::Query(MPool& pool)
        : m_pool(pool)
    {
        m_pool.get().create_whatprovides();
    }

    std::string Query::find(const std::string& query)
    {
        Queue job, solvables;
        queue_init(&job);
        queue_init(&solvables);

        Id id = pool_conda_matchspec(m_pool.get(), query.c_str());
        if (id)
        {
            queue_push2(&job, SOLVER_SOLVABLE_PROVIDES, id);
        }
        else
        {
            throw std::runtime_error("Could not generate query for " + query);
        }

        selection_solvables(m_pool.get(), &job, &solvables);

        std::stringstream out;
        if (solvables.count == 0)
        {
            out << "No entries matching \"" << query << "\" found";
        }

        if (solvables.count > 0)
        {
            Solvable* s = pool_id2solvable(m_pool.get(), solvables.elements[0]);
            std::cout << pool_id2str(m_pool.get(), s->name) << std::endl;
            if (s->requires)
            {
                Id *reqp = s->repo->idarraydata + s->requires;
                Id req = *reqp;
                while (req != 0)            /* go through all requires */
                {
                    std::cout << "├─" << pool_id2str(m_pool.get(), req) << std::endl;
                    ++reqp;
                    req = *reqp;
                }
            }
        }
        // for (int i = 0; i < solvables.count; i++)
        // {
        //     Solvable* s = pool_id2solvable(m_pool.get(), solvables.elements[i]);
        //     solvable_to_stream(out, s);
        // }
        out << std::endl;

        queue_free(&job);
        queue_free(&solvables);

        return out.str();
    }

    std::string Query::whatrequires(const std::string& query)
    {
        Queue job, solvables;
        queue_init(&job);
        queue_init(&solvables);

        Id id = pool_conda_matchspec(m_pool.get(), query.c_str());
        if (id)
        {
            queue_push2(&job, SOLVER_SOLVABLE_PROVIDES, id);
        }
        else
        {
            throw std::runtime_error("Could not generate query for " + query);
        }

        pool_whatmatchesdep(m_pool.get(), SOLVABLE_REQUIRES, id, &solvables, -1);

        std::stringstream out;
        if (solvables.count == 0)
        {
            out << "No entries matching \"" << query << "\" found";
        }
        for (int i = 0; i < solvables.count; i++)
        {
            Solvable* s = pool_id2solvable(m_pool.get(), solvables.elements[i]);
            solvable_to_stream(out, s);
        }

        queue_free(&job);
        queue_free(&solvables);

        return out.str();
    }
}

