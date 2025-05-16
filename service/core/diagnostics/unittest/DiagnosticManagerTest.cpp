#include <gtest/gtest.h>

#include "DiagnosticManager.cpp"

using namespace diagnostic_manager;

class DiagnosticManagerTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        ins = new DiagnosticManager();
    }
    virtual void TearDown(){
        delete ins;
    }

    DiagnosticManager* ins;
};

TEST_F(DiagnosticManagerTest, Initialize_TC001){
    EXPECT_TRUE(ins->Initialize());
}

TEST_F(DiagnosticManagerTest, Start_TC001){
    EXPECT_TRUE(ins->Start());
}

TEST_F(DiagnosticManagerTest, Stop_TC001){
    ins->Stop();
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
