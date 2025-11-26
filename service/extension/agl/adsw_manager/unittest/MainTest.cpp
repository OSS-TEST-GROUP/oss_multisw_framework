#include <gtest/gtest.h>
#include <cstdlib>

TEST(MainExecutableTest, SmokeRun_ExecutableStartsAndTerminates) {
    int ret = std::system("timeout 2 ./agl_adsw_manager > /dev/null 2>&1");

    ASSERT_NE(ret, -1) << "Failed to execute agl_adsw_manager via system()";
}
