#include "helpers.h"

// REBUILD TESTS

TEST_F(BelderFixture, RebuildTestCangeCppStandarts) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("main.cpp",
        "#include <iostream>\n"
        "int main(){\n"
        "std::cout << \"ABOBA\" << std::endl;\n"
        "return 0;\n"
        "}\n"
        );

    auto r1 = runBelder({"-std=c++11"});
    auto r2 = runBelder({"-std=c++17"});
    EXPECT_TRUE(r2.hasOutput("Compiling main.cpp"));
    auto r3 = runBelder({"-std=c++17"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}

TEST_F(BelderFixture, RebuildTestChangeGeneralFlags) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("main.cpp",
        "#include <iostream>\n"
        "int main(){\n"
        "std::cout << \"ABOBA\" << std::endl;\n"
        "return 0;\n"
        "}\n"
        );

    auto r1 = runBelder({});
    auto r2 = runBelder({"-w"});
    EXPECT_TRUE(r2.hasOutput("Compiling main.cpp"));
    auto r3 = runBelder({"-w"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}

TEST_F(BelderFixture, RebuildTestChangeCompileFlags) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("main.cpp",
        "#include <iostream>\n"
        "int main(){\n"
        "std::cout << \"ABOBA\" << std::endl;\n"
        "return 0;\n"
        "}\n"
        );

    auto r1 = runBelder({});
    auto r2 = runBelder({"--compile-flags", "-Wextra", "-Wall"});
    EXPECT_TRUE(r2.hasOutput("Compiling main.cpp"));
   // auto r3 = runBelder({"--compile-flags", "-Wextra", "-Wall"});
   // EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}

TEST_F(BelderFixture, RebuildTestChangeOptimization) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("main.cpp",
        "#include <iostream>\n"
        "int main(){\n"
        "std::cout << \"ABOBA\" << std::endl;\n"
        "return 0;\n"
        "}\n"
        );

    auto r1 = runBelder({"-O1"});
    auto r2 = runBelder({"-O2"});
    EXPECT_TRUE(r2.hasOutput("Compiling main.cpp"));
    auto r3 = runBelder({"-O2"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}

TEST_F(BelderFixture, RebuildTestChangeDebugFlags) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("main.cpp",
        "#include <iostream>\n"
        "int main(){\n"
        "std::cout << \"ABOBA\" << std::endl;\n"
        "return 0;\n"
        "}\n"
        );

    auto r1 = runBelder({"-g1"});
    auto r2 = runBelder({"-g3"});
    EXPECT_TRUE(r2.hasOutput("Compiling main.cpp"));
    auto r3 = runBelder({"-g3"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}

TEST_F(BelderFixture, RebuildTestChangeCompilers) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";
    if (!toolExists("clang++")) GTEST_SKIP() << "clang++ not found";

    write("main.cpp",
        "#include <iostream>\n"
        "int main(){\n"
        "std::cout << \"ABOBA\" << std::endl;\n"
        "return 0;\n"
        "}\n"
        );

    auto r1 = runBelder({});
    auto r2 = runBelder({"--CXX", "clang++"});
    EXPECT_TRUE(r2.hasOutput("Compiling main.cpp"));
    //auto r3 = runBelder({"--CXX", "clang++"});
    //EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}

TEST_F(BelderFixture, RebuildTestChangeCStandarts) {
    if (!toolExists("gсс")) GTEST_SKIP() << "gсс not found";

    write("main.c",
        "#include <stdio.h>\n"
        "int main(){\n"
        "printf(\"ABOBA\");\n"
        "return 0;\n"
        "}\n"
        );

    auto r1 = runBelder({"-std=c11"});
    auto r2 = runBelder({"-std=c17"});
    EXPECT_TRUE(r2.hasOutput("Compiling main.c"));
    auto r3 = runBelder({"-std=c17"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}


// RELINK TESTS

TEST_F(BelderFixture, RelinkTestChangeInternalLibLink) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("sum.cpp",
        "int sum(int a, int b){\n"
        "   return a + b;\n"
        "}\n"
    );

    write("main.cpp",
        "#include <iostream>\n"
        "int sum(int,int);\n"
        "int main(){\n"
        "std::cout << sum(3,4) << std::endl;\n"
        "std::cout << \"ABOBA\" << std::endl;\n"
        "return 0;\n"
        "}\n"
    );

    auto r0 = runBelder({"sum.cpp", "-o", "libsum.a"});

    auto r1 = runBelder({"main.cpp", "-o", "out", "-reb", "--no-link-force", "sum.cpp"});
    auto r2 = runBelder({"-lsum"});
    EXPECT_TRUE(r2.hasOutput("Linking file: main.cpp") && r2.hasOutput("Linking lib: libsum.a"));
    auto r3 = runBelder({"-lsum"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}

TEST_F(BelderFixture, RelinkTestChangeExternalLib) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("main.cpp",
        "#include <iostream>\n"
        "#include <cmath>\n"
        "int main(){\n"
        "double x = 0.5;\n"
        "std::cout << sin(x) << std::endl;\n"
        "std::cout << \"ABOBA\" << std::endl;\n"
        "return 0;\n"
        "}\n"
    );

    auto r1 = runBelder({});
    auto r2 = runBelder({"-lm"});
    EXPECT_TRUE(r2.hasOutput("Linking file: main.cpp"));
    auto r3 = runBelder({"-lm"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}

TEST_F(BelderFixture, RelinkTestChangeForceUnlink) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("sum.cpp",
        "int sum(int a, int b){\n"
        "   return a + b;\n"
        "}\n"
    );

    write("main.cpp",
        "#include <iostream>\n"
        "#include <cmath>\n"
        "int main(){\n"
        "double x = 0.5;\n"
        "std::cout << sin(x) << std::endl;\n"
        "std::cout << \"ABOBA\" << std::endl;\n"
        "return 0;\n"
        "}\n"
    );

    auto r1 = runBelder({});
    auto r2 = runBelder({"--no-link-force", "sum.cpp"});
    EXPECT_TRUE(r2.hasOutput("Linking file: main.cpp"));
    auto r3 = runBelder({"--no-link-force", "sum.cpp"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}

TEST_F(BelderFixture, RelinkTestChangeForceLink) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("sum.cpp",
        "int sum(int a, int b){\n"
        "   return a + b;\n"
        "}\n"
    );

    write("main.cpp",
        "#include <iostream>\n"
        "#include <cmath>\n"
        "int main(){\n"
        "double x = 0.5;\n"
        "std::cout << sin(x) << std::endl;\n"
        "std::cout << \"ABOBA\" << std::endl;\n"
        "return 0;\n"
        "}\n"
    );

    auto r1 = runBelder({});
    auto r2 = runBelder({"--link-force", "sum.cpp"});
    EXPECT_TRUE(r2.hasOutput("Linking file: main.cpp"));
    auto r3 = runBelder({"--link-force", "sum.cpp"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}

TEST_F(BelderFixture, RelinkTestChangeInternalLibUnlink) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("sum.cpp",
        "int sum(int a, int b){\n"
        "   return a + b;\n"
        "}\n"
    );

    write("mult.cpp",
        "int mult(int a, int b){\n"
        "   return a * b;\n"
        "}\n"
    );

    write("main.cpp",
        "#include <iostream>\n"
        "#include <cmath>\n"
        "int main(){\n"
        "double x = 0.5;\n"
        "std::cout << sin(x) << std::endl;\n"
        "std::cout << \"ABOBA\" << std::endl;\n"
        "return 0;\n"
        "}\n"
    );

    runBelder({"mult.cpp", "-o", "libmult.a"});

    auto r1 = runBelder({"main.cpp", "-o", "out", "-reb", "--no-link-force", "mult.cpp"});
    auto r2 = runBelder({"--no-link-force", "libmult.a"});
    EXPECT_TRUE(r2.hasOutput("Linking file: main.cpp"));
    auto r3 = runBelder({"--no-link-force", "libmult.a"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}

TEST_F(BelderFixture, RelinkTestChangeLinkerFlags) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("main.cpp",
        "#include <iostream>\n"
        "int main(){\n"
        "double x = 0.5;\n"
        "std::cout << \"ABOBA\" << std::endl;\n"
        "return 0;\n"
        "}\n"
    );

    auto r1 = runBelder({});
    auto r2 = runBelder({"--link-flags", "-lm"});
    EXPECT_TRUE(r2.hasOutput("Linking file: main.cpp"));
    //auto r3 = runBelder({"--link-flags", "-lm"});
    //EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}


// MANY OUTPUT FILES

