#include <gtest/gtest.h>
#include "diagnostic_analyzer/DataManager.cpp"
#include "diagnostic/VariantData.cpp"

#include "mock/MockLogger.hpp"

using namespace diagnostic_manager;

class DataManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create nested VariantData
        auto engine = std::make_shared<VariantData>();
        engine->SetName("Engine");

        VariantData status;
        status.SetValue("rpm", 3000);
        status.SetValue("temp", 85.5);

        // Add a vector of status reports
        std::vector<VariantData> reports;
        VariantData report1;
        report1.SetValue("rpm", 2500);
        report1.SetValue("code", std::string("OK"));

        VariantData report2;
        report2.SetValue("rpm", 2800);
        report2.SetValue("code", std::string("WARN"));

        reports.push_back(report1);
        reports.push_back(report2);

        status.SetValue("reports", reports);

        engine->SetValue("status", status);

        // Add a vector of floats (primitive type)
        std::vector<float> temperatures = {60.0f, 70.5f, 80.1f};
        engine->SetValue("temperatures", temperatures);

        // Register to manager
        manager.UpdateData(engine);
    }

    DataManager manager;
};

TEST_F(DataManagerTest, FetchPrimitive_TC001) {
    int64_t rpm;
    double temp;

    manager.FetchValue("Engine.status.rpm", rpm);
    manager.FetchValue("Engine.status.temp", temp);
    EXPECT_EQ(rpm, 3000);
    EXPECT_FLOAT_EQ(temp, 85.5f);
}

TEST_F(DataManagerTest, FetchNestedArrayElement_TC002) {
    int64_t rpm, rpm2;
    std::string code;

    manager.FetchValue("Engine.status.reports[0].rpm", rpm);
    manager.FetchValue("Engine.status.reports[1].rpm", rpm2);
    manager.FetchValue("Engine.status.reports[1].code", code);

    EXPECT_EQ(rpm, 2500);
    EXPECT_EQ(rpm2, 2800);
    EXPECT_EQ(code, "WARN");
}

TEST_F(DataManagerTest, FetchPrimitiveArrayElement_TC003) {
    double temperatures[3];
    manager.FetchValue("Engine.temperatures[0]", temperatures[0]);
    manager.FetchValue("Engine.temperatures[1]", temperatures[1]);
    manager.FetchValue("Engine.temperatures[2]", temperatures[2]);
    EXPECT_FLOAT_EQ(temperatures[0], 60.0f);
    EXPECT_FLOAT_EQ(temperatures[1], 70.5f);
    EXPECT_FLOAT_EQ(temperatures[2], 80.1f);
}

TEST_F(DataManagerTest, FetchAsVariantValuePrimitive_TC001) {
    VariantValue rpm, temp;
    EXPECT_TRUE(manager.FetchValueAsVariant("Engine.status.rpm", rpm));
    EXPECT_TRUE(manager.FetchValueAsVariant("Engine.status.temp", temp));

    EXPECT_EQ(rpm.type, std::type_index(typeid(int)));
    EXPECT_EQ(std::get<int>(rpm.value), 3000);

    EXPECT_EQ(temp.type, std::type_index(typeid(double)));
    EXPECT_DOUBLE_EQ(std::get<double>(temp.value), 85.5);
}

TEST_F(DataManagerTest, FetchAsVariantValueNestedArrayElement_TC002) {
    VariantValue rpm1, code2;
    EXPECT_TRUE(manager.FetchValueAsVariant("Engine.status.reports[0].rpm", rpm1));
    EXPECT_TRUE(manager.FetchValueAsVariant("Engine.status.reports[1].code", code2));

    EXPECT_EQ(rpm1.type, std::type_index(typeid(int)));
    EXPECT_EQ(std::get<int>(rpm1.value), 2500);

    EXPECT_EQ(code2.type, std::type_index(typeid(std::string)));
    EXPECT_EQ(std::get<std::string>(code2.value), "WARN");
}

TEST_F(DataManagerTest, FetchAsVariantValuePrimitiveArrayElement_TC003) {
    VariantValue temp1, temp2;
    EXPECT_TRUE(manager.FetchValueAsVariant("Engine.temperatures[0]", temp1));
    EXPECT_TRUE(manager.FetchValueAsVariant("Engine.temperatures[2]", temp2));

    EXPECT_EQ(temp1.type, std::type_index(typeid(float)));
    EXPECT_FLOAT_EQ(std::get<float>(temp1.value), 60.0f);

    EXPECT_EQ(temp2.type, std::type_index(typeid(float)));
    EXPECT_FLOAT_EQ(std::get<float>(temp2.value), 80.1f);
}

TEST_F(DataManagerTest, InvalidKey_TC001) {
    int64_t test;
    EXPECT_FALSE(manager.FetchValue("Engine.invalidKey", test));
}

TEST_F(DataManagerTest, InvalidIndex_TC001) {
    double temperature;
    int64_t rpm;
    EXPECT_FALSE(manager.FetchValue("Engine.temperatures[5]", temperature));
    EXPECT_FALSE(manager.FetchValue("Engine.status.reports[3].rpm", rpm));
}

TEST_F(DataManagerTest, GetPathTokens_TC001_SimplePath) {
    std::string path = "root.child.leaf[1]";
    auto tokens = manager.GetPathTokens(path);

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "root");
    EXPECT_EQ(tokens[1], "child");
    EXPECT_EQ(tokens[2], "leaf[1]");
}

TEST_F(DataManagerTest, GetPathTokens_TC002_SingleToken) {
    std::string path = "root";
    auto tokens = manager.GetPathTokens(path);

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0], "root");
}

TEST_F(DataManagerTest, GetPathTokens_TC003_TrailingDot) {
    std::string path = "root.child.";
    auto tokens = manager.GetPathTokens(path);

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], "root");
    EXPECT_EQ(tokens[1], "child");
}

TEST_F(DataManagerTest, GetPathTokens_TC004_EmptyPath) {
    std::string path = "";
    auto tokens = manager.GetPathTokens(path);

    ASSERT_EQ(tokens.size(), 0);
}

TEST_F(DataManagerTest, GetIndiciesInfo_TC001_ValidKeyOnly) {
    std::string token = "keyOnly";

    std::string key;
    std::vector<size_t> indices;
    EXPECT_TRUE(manager.GetIndiciesInfo(token, key, indices));
    EXPECT_EQ(key, "keyOnly");
    EXPECT_TRUE(indices.empty());
}

TEST_F(DataManagerTest, GetIndiciesInfo_TC002_ValidKeyWithOneIndex) {
    std::string token = "array[3]";

    std::string key;
    std::vector<size_t> indices;
    EXPECT_TRUE(manager.GetIndiciesInfo(token, key, indices));
    EXPECT_EQ(key, "array");
    ASSERT_EQ(indices.size(), 1);
    EXPECT_EQ(indices[0], 3);
}

TEST_F(DataManagerTest, GetIndiciesInfo_TC003_ValidKeyWithMultipleIndices) {
    std::string token = "matrix[2][5][10]";

    std::string key;
    std::vector<size_t> indices;
    EXPECT_TRUE(manager.GetIndiciesInfo(token, key, indices));
    EXPECT_EQ(key, "matrix");
    ASSERT_EQ(indices.size(), 3);
    EXPECT_EQ(indices[0], 2);
    EXPECT_EQ(indices[1], 5);
    EXPECT_EQ(indices[2], 10);
}

TEST_F(DataManagerTest, GetIndiciesInfo_TC004_MissingOpeningBracket) {
    std::string token = "invalid]3]";

    std::string key;
    std::vector<size_t> indices;
    EXPECT_FALSE(manager.GetIndiciesInfo(token, key, indices));
}

TEST_F(DataManagerTest, GetIndiciesInfo_TC005_MismatchedBracketsThrows) {
    std::string token = "broken[12";

    std::string key;
    std::vector<size_t> indices;
    EXPECT_FALSE(manager.GetIndiciesInfo(token, key, indices));
}

TEST_F(DataManagerTest, GetIndiciesInfo_TC006_NonNumericIndexThrows) {
    std::string token = "bad[index]";

    std::string key;
    std::vector<size_t> indices;
    EXPECT_FALSE(manager.GetIndiciesInfo(token, key, indices));
}

TEST_F(DataManagerTest, GetIndiciesInfo_TC007_UnexpectedCharacterThrows) {
    std::string token = "obj[2]x";

    std::string key;
    std::vector<size_t> indices;
    EXPECT_FALSE(manager.GetIndiciesInfo(token, key, indices));
}

TEST_F(DataManagerTest, GetIndiciesInfo_TC008_StartsWithUnderscore) {
    std::string token = "_data[1]";

    std::string key;
    std::vector<size_t> indices;
    EXPECT_TRUE(manager.GetIndiciesInfo(token, key, indices));
    EXPECT_EQ(key, "_data");
    ASSERT_EQ(indices.size(), 1);
    EXPECT_EQ(indices[0], 1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
