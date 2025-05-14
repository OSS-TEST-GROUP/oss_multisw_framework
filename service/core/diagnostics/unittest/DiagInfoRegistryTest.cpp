#include <gtest/gtest.h>

#include "diagnostic/DiagInfoRegistry.cpp"

#include "unittest/mock/DiagInfo_mock.hpp"

using ::testing::Return;
using ::testing::_;
using ::testing::A;
using ::testing::ReturnRef;
using ::testing::Values;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::DoAll;
using ::testing::Args;
using ::testing::AllOf;
using ::testing::AtLeast;
using ::testing::Combine;

using namespace diagnostic_manager;

class DiagInfoRegistryTest : public ::testing::Test {
protected:
    virtual void SetUp(){
        Mock_DiagInfo = new DiagInfoMock;

        ins = new DiagInfoRegistry;
    }
    virtual void TearDown(){
        delete ins;

        delete Mock_DiagInfo;
    }

    DiagInfoRegistry* ins;
};

TEST_F(DiagInfoRegistryTest, UpdateDiagInfo_TC001) {
    std::shared_ptr diag_info = std::make_shared<DiagInfo>();
    EXPECT_EQ(0, ins->diag_info_list_.size());

    EXPECT_CALL(*Mock_DiagInfo, GetDiagCode()).WillOnce(Return(123));

    ins->UpdateDiagInfo(diag_info);
    EXPECT_EQ(1, ins->diag_info_list_.size());
    EXPECT_EQ(123, ins->diag_info_list_.begin()->first);
    EXPECT_EQ(diag_info, ins->diag_info_list_.begin()->second);
}

TEST_F(DiagInfoRegistryTest, UpdateDiagInfo_TC002_NullTest) {
    ins->UpdateDiagInfo(nullptr);
    EXPECT_EQ(0, ins->diag_info_list_.size());
}

TEST_F(DiagInfoRegistryTest, RemoveDiagInfo_TC001) {
    std::shared_ptr diag_info = std::make_shared<DiagInfo>();
    ins->diag_info_list_[123] = diag_info;

    EXPECT_EQ(1, ins->diag_info_list_.size());

    ins->RemoveDiagInfo(123);
    EXPECT_EQ(0, ins->diag_info_list_.size());
}

TEST_F(DiagInfoRegistryTest, ClearDiagInfoList_TC001) {
    std::shared_ptr diag_info = std::make_shared<DiagInfo>();
    ins->diag_info_list_[123] = diag_info;

    EXPECT_EQ(1, ins->diag_info_list_.size());

    ins->ClearDiagInfoList();
    EXPECT_EQ(0, ins->diag_info_list_.size());
}

TEST_F(DiagInfoRegistryTest, GetDiagInfo_TC001) {
    std::shared_ptr diag_info = std::make_shared<DiagInfo>();
    ins->diag_info_list_[123] = diag_info;

    EXPECT_EQ(1, ins->diag_info_list_.size());

    std::shared_ptr ret = ins->GetDiagInfo(123);
    EXPECT_EQ(ret, diag_info);
}

TEST_F(DiagInfoRegistryTest, GetDiagInfo_TC002_NoItem) {
    std::shared_ptr diag_info = std::make_shared<DiagInfo>();
    ins->diag_info_list_[123] = diag_info;

    EXPECT_EQ(1, ins->diag_info_list_.size());

    std::shared_ptr ret = ins->GetDiagInfo(234);
    EXPECT_EQ(nullptr, ret);
}

TEST_F(DiagInfoRegistryTest, GetDiagInfoByName_TC001) {
    std::shared_ptr diag_info = std::make_shared<DiagInfo>();
    ins->diag_info_list_[123] = diag_info;

    EXPECT_CALL(*Mock_DiagInfo, GetDiagName()).WillRepeatedly(Return("helloworld"));

    EXPECT_EQ(1, ins->diag_info_list_.size());

    std::shared_ptr ret = ins->GetDiagInfoByName("helloworld");
    EXPECT_EQ(ret, diag_info);
}


TEST_F(DiagInfoRegistryTest, GetDiagInfoByName_TC002_NoItem) {
    std::shared_ptr diag_info = std::make_shared<DiagInfo>();
    ins->diag_info_list_[123] = diag_info;

    EXPECT_CALL(*Mock_DiagInfo, GetDiagName()).WillRepeatedly(Return("helloworld2"));

    EXPECT_EQ(1, ins->diag_info_list_.size());

    std::shared_ptr ret = ins->GetDiagInfoByName("helloworld");
    EXPECT_EQ(nullptr, ret);
}

TEST_F(DiagInfoRegistryTest, GetAllDiagInfo_TC001) {
    std::shared_ptr diag_info1 = std::make_shared<DiagInfo>();
    ins->diag_info_list_[123] = diag_info1;
    std::shared_ptr diag_info2 = std::make_shared<DiagInfo>();
    ins->diag_info_list_[124] = diag_info2;
    std::shared_ptr diag_info3 = std::make_shared<DiagInfo>();
    ins->diag_info_list_[125] = diag_info3;

    EXPECT_EQ(3, ins->diag_info_list_.size());

    std::vector<std::shared_ptr<DiagInfo>> ret = ins->GetAllDiagInfo();
    EXPECT_EQ(3, ret.size());
    EXPECT_TRUE(std::find(ret.begin(), ret.end(), diag_info1) != ret.end());
    EXPECT_TRUE(std::find(ret.begin(), ret.end(), diag_info2) != ret.end());
    EXPECT_TRUE(std::find(ret.begin(), ret.end(), diag_info3) != ret.end());
}

TEST_F(DiagInfoRegistryTest, GetDiagInfoCodeList_TC001) {
    std::shared_ptr diag_info1 = std::make_shared<DiagInfo>();
    ins->diag_info_list_[123] = diag_info1;
    std::shared_ptr diag_info2 = std::make_shared<DiagInfo>();
    ins->diag_info_list_[124] = diag_info2;
    std::shared_ptr diag_info3 = std::make_shared<DiagInfo>();
    ins->diag_info_list_[125] = diag_info3;

    EXPECT_EQ(3, ins->diag_info_list_.size());

    std::vector<int32_t> ret = ins->GetDiagInfoCodeList();
    EXPECT_EQ(3, ret.size());
    EXPECT_TRUE(std::find(ret.begin(), ret.end(), 123) != ret.end());
    EXPECT_TRUE(std::find(ret.begin(), ret.end(), 124) != ret.end());
    EXPECT_TRUE(std::find(ret.begin(), ret.end(), 125) != ret.end());
}

TEST_F(DiagInfoRegistryTest, GetDiagInfoNameList_TC001) {
    std::shared_ptr diag_info1 = std::make_shared<DiagInfo>();
    ins->diag_info_list_[123] = diag_info1;
    std::shared_ptr diag_info2 = std::make_shared<DiagInfo>();
    ins->diag_info_list_[124] = diag_info2;
    std::shared_ptr diag_info3 = std::make_shared<DiagInfo>();
    ins->diag_info_list_[125] = diag_info3;

    EXPECT_EQ(3, ins->diag_info_list_.size());

    EXPECT_CALL(*Mock_DiagInfo, GetDiagName()).WillOnce(Return("hello1"))
                                              .WillOnce(Return("hello2"))
                                              .WillOnce(Return("hello3"));

    std::vector<std::string> ret = ins->GetDiagInfoNameList();
    EXPECT_EQ(3, ret.size());
    EXPECT_TRUE(std::find(ret.begin(), ret.end(), "hello1") != ret.end());
    EXPECT_TRUE(std::find(ret.begin(), ret.end(), "hello2") != ret.end());
    EXPECT_TRUE(std::find(ret.begin(), ret.end(), "hello3") != ret.end());
}


int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
