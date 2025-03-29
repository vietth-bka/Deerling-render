#pragma once

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <tinyformat.h>

#if defined(__aarch64__)
// uses inline assembly
#elif defined(_WIN32)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace lightwave {

static uint64_t now() {
#if defined(__aarch64__)
    int64_t virtual_timer_value;
    asm volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
    return virtual_timer_value;
#else
    return __rdtsc();
#endif
}

static std::string thousands(uint64_t number) {
    std::string result;
    do {
        int part = number % 1000;
        number /= 1000;
        result = tfm::format(number ? "%03d" : "%d", part) +
                 (result.empty() ? "" : "," + result);
    } while (number);
    return result;
}

class Profiler {
protected:
    struct Scope {
        uint64_t time    = 0;
        uint64_t counter = 0;

        Scope *parent;
        std::map<const char *, Scope *> children;

        Scope(Scope *parent) : parent(parent) {}

        Scope *get(const char *child) {
            auto it = children.find(child);
            if (it == children.end()) {
                Scope *scope = new Scope(this);
                children.insert(std::make_pair(child, scope));
                return scope;
            }

            return it->second;
        }

        void operator+=(const Scope &other) {
            time += other.time;
            counter += other.counter;

            for (const auto &child : other.children) {
                *get(child.first) += *child.second;
            }
        }

        void dump(uint64_t globalTime, std::ostream &stream,
                  const std::string &indent = "") const {
            if (children.empty())
                return;

            std::vector<std::pair<const char *, const Scope *>> sortedChildren;
            for (auto &it : children) {
                sortedChildren.push_back(it);
            }
            std::sort(
                sortedChildren.begin(),
                sortedChildren.end(),
                [](auto a, auto b) { return a.second->time > b.second->time; });

            uint64_t childTotalTime = 0;
            for (const auto &child : sortedChildren) {
                auto percentageGlobal =
                    100 * double(child.second->time) / double(globalTime);
                auto percentageLocal =
                    100 * double(child.second->time) / double(time);
                childTotalTime += child.second->time;
                stream << tfm::format("%-24s %5.1f%%   %5.1f%%   %s",
                                      indent + child.first,
                                      percentageGlobal,
                                      percentageLocal,
                                      thousands(child.second->counter))
                       << std::endl;
                child.second->dump(globalTime, stream, indent + "  ");
            }

            auto percentageGlobal =
                100 * double(time - childTotalTime) / double(globalTime);
            auto percentageLocal =
                100 * double(time - childTotalTime) / double(time);
            stream << tfm::format("%-24s %5.1f%%   %5.1f%%",
                                  indent + "Remainder",
                                  percentageGlobal,
                                  percentageLocal)
                   << std::endl;
        }
    };

    Scope m_root{ nullptr };

    friend class GlobalProfiler;
    friend class ThreadProfiler;
};

class GlobalProfiler : public Profiler {
private:
    std::mutex m_mutex;

public:
    ~GlobalProfiler() {
        if (m_root.counter == 0) {
            std::cout << std::endl;
            std::cout << "no profiling data was captured" << std::endl;
            std::cout << std::endl;
            return;
        }

        std::cout << std::endl;
        std::cout << tfm::format("%-24s global    local   times called",
                                 "block")
                  << std::endl;
        std::cout << std::endl;
        m_root.dump(m_root.time, std::cout);
    }

    void operator+=(const Profiler &other) {
        /// this operation needs to be atomic since ThreadProfilers might be
        /// deconstructed in parallel and write to us at the same time.
        std::lock_guard<std::mutex> lock(m_mutex);
        m_root += other.m_root;
    }
};

extern GlobalProfiler globalProfiler;

class ThreadProfiler : public Profiler {
    uint64_t m_startTime{ now() };
    Scope *m_currentScope;

public:
    ThreadProfiler() : Profiler() { m_currentScope = &m_root; }

    ~ThreadProfiler() {
        m_root.time += now() - m_startTime;
        m_root.counter += 1;
        globalProfiler += *this;
    }

    void push(const char *scope) {
        m_currentScope = m_currentScope->get(scope);
    }

    void pop(uint64_t time) {
        m_currentScope->time += time;
        m_currentScope->counter += 1;
        m_currentScope = m_currentScope->parent;
    }
};

extern thread_local ThreadProfiler threadProfiler;

class ProfilerBlock {
    uint64_t m_startTime = now();

public:
    ProfilerBlock(const char *scope) { threadProfiler.push(scope); }

    ~ProfilerBlock() { threadProfiler.pop(now() - m_startTime); }
};

#define PROFILE(scope) ProfilerBlock __profiler_block{ scope };
// #define PROFILE(scope)

} // namespace lightwave
