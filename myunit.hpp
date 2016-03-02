/*
 * Copyright David Wilson, 2016.
 * License: http://opensource.org/licenses/MIT
 */

#ifndef MYUNIT_H
#define MYUNIT_H

/**
 * myunit: Ultra tiny test framework.
 *
 * Works across multiple files, only one of which may contain MU_MAIN. Example:
 *
 * @code
 *      #include <cassert>
 *      #include "myunit.hpp"
 *
 *      MU_TEST(testName)
 *      {
 *          assert(1 == 2);
 *      }
 *
 *      MU_MAIN
 *  @endcode
 *
 */

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>


namespace myunit {


struct Test;
static inline std::string getUnitName(const char *n_);
static inline std::string getTestName(const Test *t);
typedef void (*test_func)();


/**
 * Internal descriptor for a test case, tracks the name of the unit where the
 * test was defined, its name, and a reference to its closure.
 */
struct Test {
    std::string unit;
    std::string name;
    test_func func;

    Test(const char *file, const char *name_, test_func func_)
        : name(name_)
        , func(func_)
    {
        unit = getUnitName(file);
        Test::getTests().push_back(this);
    }

    static std::vector<Test*> &getTests() {
        static std::vector<Test*> tests;
        return tests;
    }
};


/**
 * Internal descriptor for a function to be executed once during unit
 * initialization.
 */
struct TestSetup {
    test_func func;

    TestSetup(test_func func_)
        : func(func_)
    {
        TestSetup::getSetups().push_back(this);
    }

    static std::vector<TestSetup*> &getSetups() {
        static std::vector<TestSetup*> setups;
        return setups;
    }
};


static inline int
main(int argc, const char **argv)
{
    for(auto setup : TestSetup::getSetups()) {
        setup->func();
    }

    auto filtered(Test::getTests());
    filtered.erase(
        std::remove_if(filtered.begin(), filtered.end(),
            [&](const Test *x) {
                return argc > 1 && !std::any_of(argv + 1, argv + argc,
                    [&](const char *substr) {
                        return getTestName(x).find(substr) != std::string::npos;
                    });
            }),
        filtered.end());

    std::sort(filtered.begin(), filtered.end(),
        [](const Test *x, const Test *y) {
            return getTestName(x) < getTestName(y);
        });

    int run_count = 1;
    if(getenv("RUN_COUNT")) {
        run_count = std::stoi(getenv("RUN_COUNT"));
    }

    std::cout << "Selected " << filtered.size() << " tests." << std::endl;
    for(int i = 0; i < run_count; i++) {
        for(auto test : filtered) {
            std::cout << "Running " << getTestName(test) << " ..." << std::endl;
            test->func();
        }
    }

    return 0;
}


/**
 * Given a string returned by __FILE__, attempt to extract only its filename
 * part.
 */
static inline std::string
getUnitName(const char *n_)
{
    std::string n(n_);
    n = n.erase(n.find_last_of("."));

    size_t slash_p = n.find_last_of("/");
    if(slash_p != std::string::npos) {
        n = n.erase(0, 1+slash_p);
    }

    return n;
}


static inline std::string
getTestName(const Test *t)
{
    std::string out;
    out.append(t->unit);
    out.append(".");
    out.append(t->name);
    return out;
}


/**
 * Crash if an exception was not thrown. Return the exception if it was thrown.
 * the thrown exception value.
 *
 * @code
 *      auto e = myunit::raises<std::runtime_error>([]() {
 *          throw std::runtime_error("broken!");
 *      });
 * @endcode
 */
template<typename Exception,
         typename Expr>
Exception
raises(Expr expr)
{
    try {
        (expr)();
        std::string s;
        s += "myunit :";
        s += typeid(Exception).name();
        s += " was not raised";
        fprintf(stderr, "%s\n", s.c_str());
        throw std::runtime_error(s);
    } catch(Exception &e) {
        return e;
    }
}


} // ::myunit


#define MU_DEBUG(x, ...) \
    fprintf(stderr, __FILE__ ": " x "\n", __VA_ARGS__);


#define MU_SETUP(name) \
    static void setup_##name(); \
    namespace myunit { \
        static TestSetup _##name##_testsetup (&setup_##name); \
    } \
    static void setup_##name()


/**
 * Declare a test function and register it for later execution by MU_MAIN().
 *
 * @code
 *      MU_TEST(printfSucceeds)
 *      {
 *          assert(4 == printf("%s\n", "test"));
 *      }
 * @endcode
 */
#define MU_TEST(name) \
    static void test_##name(); \
    namespace myunit { \
        static Test _##name##_testinfo (__FILE__, #name, &test_##name); \
    } \
    static void test_##name()


/**
 * Macro that expands to a C main() function that will execute registered
 * tests. The tests may be filtered by specifying a list of substrings on the
 * command line.
 */
#define MU_MAIN() \
    int main(int argc, const char **argv) { \
        return myunit::main(argc, argv); \
    }


#endif /* !MYUNIT_H */
