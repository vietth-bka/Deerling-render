#include <lightwave/core.hpp>
#include <lightwave/logger.hpp>
#include <lightwave/registry.hpp>
#include <catch_amalgamated.hpp>

#include "parser.hpp"

#include <fstream>

#ifdef LW_OS_WINDOWS
#include <cstdlib>
#include <Windows.h>
#endif

using namespace lightwave;

void print_exception(const std::exception &e, int level = 0) {
    logger(EError, "%s%s", std::string(2 * level, ' '), e.what());
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception &nestedException) {
        print_exception(nestedException, level + 1);
    } catch (...) {
    }
}

int runUnitTests(int argc, const char *argv[]) {
    return Catch::Session().run( argc, argv );
}

int main(int argc, const char *argv[]) {
#ifdef LW_DEBUG
    logger(EWarn, "lightwave was compiled in Debug mode, expect rendering to "
                  "be much slower");
#endif
#ifdef LW_CC_MSC
    logger(EWarn, "lightwave was compiled using MSVC, expect rendering to be "
                  "slower (we recommend using clang instead)");
#endif

#ifdef LW_OS_WINDOWS
    // The error dialog might interfere with the run_tests.py script on Windows,
    // so disable it.
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

    // Set console code page to UTF-8 so console known how to interpret string data
    SetConsoleOutputCP(CP_UTF8);

    // Enable buffering to prevent VS from chopping up UTF-8 byte sequences
    setvbuf(stdout, nullptr, _IOFBF, 1000);
#endif

    try {
        if (argc <= 1 || *argv[1] == '-') {
            logger(EInfo, "running unit tests since no scene path was given");
            return runUnitTests(argc, argv);
        }

        std::filesystem::path scenePath = argv[1];

        SceneParser parser{ scenePath };
        for (auto &object : parser.objects()) {
            if (auto executable = dynamic_cast<Executable *>(object.get())) {
                executable->execute();
            }
        }
    } catch (const std::exception &e) {
        print_exception(e);
        return 1;
    }

    return 0;
}
