#pragma once

// Minimal dependency-free test harness: TEST(name) registers a case, CHECK
// records a failure without aborting, RTLB_TEST_MAIN() runs everything.

#include <cmath>
#include <cstdio>
#include <vector>

namespace rtlb_test {

struct Case {
    const char* name;
    void (*run)();
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

inline int& failures() {
    static int count = 0;
    return count;
}

struct Registrar {
    Registrar(const char* name, void (*run)()) { registry().push_back({name, run}); }
};

inline int run_all() {
    for (const Case& test : registry()) {
        const int before = failures();
        test.run();
        std::printf("[%s] %s\n", failures() == before ? " ok " : "FAIL", test.name);
    }
    return failures() == 0 ? 0 : 1;
}

}  // namespace rtlb_test

#define TEST(name)                                                        \
    static void name();                                                   \
    static const rtlb_test::Registrar registrar_##name(#name, &name);     \
    static void name()

#define CHECK(condition)                                                           \
    do {                                                                           \
        if (!(condition)) {                                                        \
            ++rtlb_test::failures();                                               \
            std::fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__,  \
                         #condition);                                              \
        }                                                                          \
    } while (false)

#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_NEAR(a, b, eps) CHECK(std::fabs((a) - (b)) <= (eps))
#define CHECK_THROWS(expr)      \
    do {                        \
        bool thrown_ = false;   \
        try {                   \
            (void)(expr);       \
        } catch (...) {         \
            thrown_ = true;     \
        }                       \
        CHECK(thrown_);         \
    } while (false)

#define RTLB_TEST_MAIN() \
    int main() { return rtlb_test::run_all(); }
