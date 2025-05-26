# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(morninglangTests LANGUAGES CXX)

# ---- Tests ----

add_executable(morninglang_test source/morninglang_test.cpp)
target_link_libraries(morninglang_test PRIVATE morninglang_lib)
target_compile_features(morninglang_test PRIVATE cxx_std_17)

add_test(NAME morninglang_test COMMAND morninglang_test)

# ---- End-of-file commands ----

add_folders(Test)
