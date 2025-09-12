#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
//   setenv("FASTDDS_BUILTIN_TRANSPORTS", "UDPv4", 1);         // SHM off, UDP만
//   unsetenv("FASTDDS_DEFAULT_PROFILES_FILE");  // XML 프로파일 무시
//   std::fprintf(stderr, "[MAIN] FASTDDS_BUILTIN_TRANSPORTS=%s\n",
//                getenv("FASTDDS_BUILTIN_TRANSPORTS"));

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}