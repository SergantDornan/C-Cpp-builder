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
    auto r3 = runBelder({"--compile-flags", "-Wextra", "-Wall"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
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
    auto r3 = runBelder({"--CXX", "clang++"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}

TEST_F(BelderFixture, RebuildTestChangeCStandarts) {
    if (!toolExists("g++")) GTEST_SKIP() << "gсс not found";

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
    runCommand("rm " + tmpDir + "/sum.cpp");

    auto r1 = runBelder({"main.cpp", "-o", "out"});
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
    runCommand("rm " + tmpDir + "/mult.cpp");

    auto r1 = runBelder({"main.cpp", "-o", "out"});
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
    auto r3 = runBelder({"--link-flags", "-lm"});
    EXPECT_TRUE(r3.hasOutput("belder: nothing to link"));
}


// MANY OUTPUT FILES

TEST_F(BelderFixture, ManyOutputFiles1) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("sum.cpp",
        "int sum(int a, int b){\n"
        "   return a + b;\n"
        "}\n"
    );

    write("main1.cpp",
        "#include <iostream>\n"
        "int sum(int,int);\n"
        "int main(){\n"
        "std::cout << sum(1,2) << std::endl;\n"
        "return 0;\n"
        "}\n"
    );

    write("main2.cpp",
        "#include <iostream>\n"
        "int sum(int,int);"
        "int main(){\n"
        "std::cout << sum(3,4) << std::endl;\n"
        "return 0;\n"
        "}\n"
    );

    auto r1 = runBelder({"main1.cpp", "-o", "out1"});
    EXPECT_BELDER_OK(r1, r1.diagnostic());
    auto r2 = runBelder({"main2.cpp", "-o", "out2"});
    EXPECT_BELDER_OK(r2, r2.diagnostic());

    runCommand("rm " + tmpDir + "/sum.cpp");
    sleep(2);
    write("sum.cpp",
        "int sum(int a, int b){\n"
        "   return a * b;\n"
        "}\n"
    );

    auto r3 = runBelder({"main1.cpp", "-o", "out1"});
    EXPECT_BELDER_OK(r3, r3.diagnostic());
    auto r4 = runBelder({"main2.cpp", "-o", "out2"});
    EXPECT_BELDER_OK(r4, r4.diagnostic());
    EXPECT_TRUE(r4.hasOutput("Linking file: main2.cpp") && 
                r4.hasOutput("Linking file: sum.cpp") &&
                !r4.hasOutput("Compiling"));
    auto r5 = runBelder({"main2.cpp", "-o", "out2"});
    EXPECT_BELDER_OK(r5, r5.diagnostic());
    EXPECT_TRUE(r5.hasOutput("belder: nothing to link"));
}

TEST_F(BelderFixture, ManyOutputFiles2) {
    if (!toolExists("g++")) GTEST_SKIP() << "g++ not found";

    write("sum.cpp",
        "int sum(int a, int b){\n"
        "   return a + b;\n"
        "}\n"
    );

    write("main1.cpp",
        "#include <iostream>\n"
        "int sum(int,int);\n"
        "int main(){\n"
        "std::cout << sum(1,2) << std::endl;\n"
        "return 0;\n"
        "}\n"
    );

    write("main2.cpp",
        "#include <iostream>\n"
        "int sum(int,int);"
        "int main(){\n"
        "std::cout << sum(3,4) << std::endl;\n"
        "return 0;\n"
        "}\n"
    );

    auto r1 = runBelder({"main1.cpp", "-o", "out1"});
    EXPECT_BELDER_OK(r1, r1.diagnostic());
    auto r2 = runBelder({"main2.cpp", "-o", "out2"});
    EXPECT_BELDER_OK(r2, r2.diagnostic());
    auto r5 = runBelder({"main1.cpp", "-o", "out1"});
    EXPECT_BELDER_OK(r5, r5.diagnostic());
    EXPECT_TRUE(r5.hasOutput("belder: nothing to link"));
}


TEST_F(BelderFixture, ManyOutputFiles3) {
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

    write("main1.cpp",
        "#include <iostream>\n"
        "int sum(int,int);\n"
        "int mult(int,int);\n"
        "int main(){\n"
        "std::cout << sum(1,2) << std::endl;\n"
        "std::cout << mult(2,3) << std::endl;\n"
        "return 0;\n"
        "}\n"
    );

    write("main2.cpp",
        "#include <iostream>\n"
        "int sum(int,int);"
        "int main(){\n"
        "std::cout << sum(3,4) << std::endl;\n"
        "return 0;\n"
        "}\n"
    );

    auto r1 = runBelder({"main1.cpp", "-o", "out1"});
    EXPECT_BELDER_OK(r1, r1.diagnostic());
    auto r2 = runBelder({"main2.cpp", "-o", "out2"});
    EXPECT_BELDER_OK(r2, r2.diagnostic());

    runCommand("rm " + tmpDir + "/mult.cpp");
    sleep(2);
    write("mult.cpp",
        "int mult(int a, int b){\n"
        "   return a * b * 2;\n"
        "}\n"
    );

    auto r3 = runBelder({"main1.cpp", "-o", "out1"});
    EXPECT_BELDER_OK(r3, r3.diagnostic());
    EXPECT_TRUE(r3.hasOutput("Linking file: main1.cpp") &&
                r3.hasOutput("Compiling"));
    auto r4 = runBelder({"main2.cpp", "-o", "out2"});
    EXPECT_BELDER_OK(r4, r4.diagnostic());
    EXPECT_TRUE(r4.hasOutput("belder: nothing to link"));
}