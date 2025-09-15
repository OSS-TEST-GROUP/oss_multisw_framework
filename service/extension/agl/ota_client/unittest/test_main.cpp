#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}