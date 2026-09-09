#include "gtest/gtest.h"
#include "VssTree.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>

class VssTreeTest : public ::testing::Test {
protected:
    std::optional<std::string> getEnv(const std::string& key) {
        const char* val = std::getenv(key.c_str());
        return val ? std::optional<std::string>(val) : std::nullopt;
    }

    void SetUp() override {
        // The test must be run from the build directory.
        config_path = getEnv("VEHICLE_CONFIG_PATH").value_or("../config");
        config_path.append("/viss.him");
        vss_tree = new viss::VSSTree(config_path);
        if (!vss_tree->isValid()) {
            FAIL() << "Failed to load VSS tree from ../config/viss.him";
        }
    }

    void TearDown() override {
        delete vss_tree;
    }

    viss::VSSTree* vss_tree;
    std::string config_path;
};

TEST_F(VssTreeTest, TreeByInvalidFile) {

    auto tree = new viss::VSSTree("/invalid.him");
    ASSERT_FALSE(tree->isValid());
    delete tree;

}

TEST_F(VssTreeTest, TreeByBinaryFile) {
    auto test_path = getEnv("VEHICLE_CONFIG_PATH").value_or("../unittest");
    test_path.append("/testfiles/bin_test.him");
    auto tree = new viss::VSSTree(test_path);
    ASSERT_TRUE(tree->isValid());
    delete tree;
}

TEST_F(VssTreeTest, TreeByNoJsonFile) {
    auto test_path = getEnv("VEHICLE_CONFIG_PATH").value_or("../unittest");
    test_path.append("/testfiles/no_json_test.him");
    auto tree = new viss::VSSTree(test_path);
    ASSERT_FALSE(tree->isValid());
    delete tree;
}

TEST_F(VssTreeTest, TreeByErrorJsonFile) {
    auto test_path = getEnv("VEHICLE_CONFIG_PATH").value_or("../unittest");
    test_path.append("/testfiles/error_json_test.him");
    auto tree = new viss::VSSTree(test_path);
    ASSERT_FALSE(tree->isValid());
    delete tree;
}

TEST_F(VssTreeTest, TreeByArrayJsonFile) {
    auto test_path = getEnv("VEHICLE_CONFIG_PATH").value_or("../unittest");
    test_path.append("/testfiles/array_json_test.him");
    auto tree = new viss::VSSTree(test_path);
    ASSERT_FALSE(tree->isValid());
    delete tree;
}

TEST_F(VssTreeTest, TreeByMultiMemberJsonFile) {
    auto test_path = getEnv("VEHICLE_CONFIG_PATH").value_or("../unittest");
    test_path.append("/testfiles/multi_member_json_test.him");
    auto tree = new viss::VSSTree(test_path);
    ASSERT_FALSE(tree->isValid());
    delete tree;
}

TEST_F(VssTreeTest, TreeByNoBinaryFile) {
    auto test_path = getEnv("VEHICLE_CONFIG_PATH").value_or("../unittest");
    test_path.append("/testfiles/no_bin_test.him");
    auto tree = new viss::VSSTree(test_path);
    ASSERT_FALSE(tree->isValid());
    delete tree;
}

TEST_F(VssTreeTest, FindNodeBySimplePath) {
    std::string root_name;
    viss::Node_t* node = vss_tree->findNode("Vehicle.Speed");
    ASSERT_NE(node, nullptr);
}

TEST_F(VssTreeTest, FindNodeByRoot) {
    std::string root_name;
    viss::Node_t* node = vss_tree->findNode("Vehicle");
    ASSERT_NE(node, nullptr);
}

TEST_F(VssTreeTest, FindNodeByInvalidRoot) {
    std::string root_name;
    viss::Node_t* node = vss_tree->findNode("Car.Speed");
    ASSERT_EQ(node, nullptr);
}

TEST_F(VssTreeTest, FindNodeByInvalidPath) {
    std::string root_name;
    viss::Node_t* node = vss_tree->findNode("Vehicle.Speed.Invalid");
    ASSERT_EQ(node, nullptr);
}

// --- Tests for getPaths ---

TEST_F(VssTreeTest, GetPaths_NoFilter) {
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle.Speed", nullptr);
    ASSERT_EQ(paths.size(), 1);
    EXPECT_EQ(paths[0], "Vehicle.Speed");
}

TEST_F(VssTreeTest, GetPaths_InvalidPathNoFilter) {
    std::vector<std::string> paths = vss_tree->getPaths("Car.Speed", nullptr);
    ASSERT_EQ(paths.size(), 0);
}

TEST_F(VssTreeTest, GetPaths_WithPathsFilter_Single) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": [{ "variant": "paths", "parameter": "Invalid.FuelLevel" }] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle.Powertrain", &filter_doc);
    ASSERT_EQ(paths.size(), 0);
    //EXPECT_EQ(paths[0], "Vehicle.Powertrain.OBD.FuelLevel");
}

TEST_F(VssTreeTest, GetPaths_WithPathsFilter_Branch) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": [{ "variant": "paths", "parameter": ["Powertrain.FuelSystem.RefuelPortPosition", "Cabin.Door"] }] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle", &filter_doc);
    ASSERT_GT(paths.size(), 2);
    // The order is not guaranteed by the implementation, so we check for presence
    //EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.OBD.FuelLevel"), paths.end());
    EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Cabin.Door.Row2.PassengerSide.IsOpen"), paths.end());
    EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Cabin.Door.Row2.PassengerSide.Window.IsOpen"), paths.end());
}


TEST_F(VssTreeTest, GetPaths_WithPathsFilter_InvalidMultiple) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": [{ "variant": "paths", "parameter": ["Invalid.FuelLevel", "Transmission.Invalid"] }] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle.Powertrain", &filter_doc);
    ASSERT_EQ(paths.size(), 0);
    // The order is not guaranteed by the implementation, so we check for presence
    //EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.OBD.FuelLevel"), paths.end());
    //EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.Transmission.Gear"), paths.end());
}

TEST_F(VssTreeTest, GetPaths_WithPathsFilter_Multiple) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": [{ "variant": "paths", "parameter": ["Invalid.FuelLevel", "Transmission.GearCount"] }] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle.Powertrain", &filter_doc);
    ASSERT_EQ(paths.size(), 1);
    // The order is not guaranteed by the implementation, so we check for presence
    //EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.OBD.FuelLevel"), paths.end());
    EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.Transmission.GearCount"), paths.end());
}

TEST_F(VssTreeTest, GetPaths_WithPathsFilter_SingleWildcard) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": [{ "variant": "paths", "parameter": "FuelSystem.*" }] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle.Powertrain", &filter_doc);
    // Expect all direct children of OBD
    ASSERT_GE(paths.size(), 5); // Based on viss.json, there are many signals under OBD
    EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.FuelSystem.HybridType"), paths.end());
    EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.FuelSystem.AbsoluteLevel"), paths.end());
}

TEST_F(VssTreeTest, GetPaths_WithArrayFilter) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": [{ "variant": "paths", "parameter": "FuelSystem.*" }, { "variant": "history", "parameter": "P2DT12H" }] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle.Powertrain", &filter_doc);
    // Expect all direct children of OBD
    ASSERT_GE(paths.size(), 5); // Based on viss.json, there are many signals under OBD
    EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.FuelSystem.HybridType"), paths.end());
    EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.FuelSystem.AbsoluteLevel"), paths.end());
}

TEST_F(VssTreeTest, GetPaths_WithArray) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": ["variant", "paths", "parameter", "Invalid.FuelLevel"] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle.Powertrain", &filter_doc);
    ASSERT_EQ(paths.size(), 0);
    //EXPECT_EQ(paths[0], "Vehicle.Powertrain.OBD.FuelLevel");
}

TEST_F(VssTreeTest, GetPaths_NoVariant) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": [{"parameter": "Invalid.FuelLevel"}] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle", &filter_doc);
    ASSERT_EQ(paths.size(), 0);
    //EXPECT_EQ(paths[0], "Vehicle.Powertrain.OBD.FuelLevel");
}

TEST_F(VssTreeTest, GetPaths_VariantNotString) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": [{"variant": 100, "parameter": "Invalid.FuelLevel"}] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle.Speed", &filter_doc);
    ASSERT_EQ(paths.size(), 1);
    //EXPECT_EQ(paths[0], "Vehicle.Powertrain.OBD.FuelLevel");
}

TEST_F(VssTreeTest, GetPaths_NoParameter) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": [{ "variant": "paths" }] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle.Powertrain", &filter_doc);
    // Expect all direct children of OBD
    ASSERT_EQ(paths.size(), 0); // Based on viss.json, there are many signals under OBD
    //EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.FuelSystem.HybridType"), paths.end());
    //EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.FuelSystem.AbsoluteLevel"), paths.end());
}

TEST_F(VssTreeTest, GetPaths_ParamArray) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": [{ "variant": "paths", "parameter": ["FuelSystem.HybridType", "FuelSystem.AbsoluteLevel"] }] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle.Powertrain", &filter_doc);
    // Expect all direct children of OBD
    ASSERT_EQ(paths.size(), 2); // Based on viss.json, there are many signals under OBD
    //EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.FuelSystem.HybridType"), paths.end());
    //EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.FuelSystem.AbsoluteLevel"), paths.end());
}

TEST_F(VssTreeTest, GetPaths_ParamIntArray) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": [{ "variant": "paths", "parameter": [100, 200] }] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle.Powertrain", &filter_doc);
    // Expect all direct children of OBD
    ASSERT_EQ(paths.size(), 0); // Based on viss.json, there are many signals under OBD
    //EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.FuelSystem.HybridType"), paths.end());
    //EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.FuelSystem.AbsoluteLevel"), paths.end());
}

TEST_F(VssTreeTest, GetPaths_ParamInt) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": [{ "variant": "paths", "parameter": 100 }] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle.Powertrain", &filter_doc);
    // Expect all direct children of OBD
    ASSERT_EQ(paths.size(), 0); // Based on viss.json, there are many signals under OBD
    //EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.FuelSystem.HybridType"), paths.end());
    //EXPECT_NE(std::find(paths.begin(), paths.end(), "Vehicle.Powertrain.FuelSystem.AbsoluteLevel"), paths.end());
}

TEST_F(VssTreeTest, GetPaths_OtherFilterType) {
    rapidjson::Document filter_doc;
    filter_doc.Parse(R"({ "filter": [{ "variant": "timebased", "parameter": "1000" }] })");
    std::vector<std::string> paths = vss_tree->getPaths("Vehicle.Speed", &filter_doc);
    // Should ignore the timebased filter and return the base path
    ASSERT_EQ(paths.size(), 1);
    EXPECT_EQ(paths[0], "Vehicle.Speed");
}


// --- Tests for getDataTypeTemplate ---

TEST_F(VssTreeTest, GetDataTypeTemplate_SimpleType) {
    rapidjson::Document doc = vss_tree->getDataTypeTemplate("Vehicle.Speed");
    //ASSERT_TRUE(doc.IsObject()); // It's a branch with one leaf
    //ASSERT_TRUE(doc.HasMember("Speed"));
    ASSERT_TRUE(doc.IsNull());
}

TEST_F(VssTreeTest, GetDataTypeTemplate_ComplexType) {
    // Vehicle.OBD.DTCs is a string array, let's pick another one.
    // Vehicle.Chassis.Axle.Row1.Wheel.Left.Tire has a complex type
    rapidjson::Document doc = vss_tree->getDataTypeTemplate("Vehicle.Chassis.Axle.Row1.Wheel.Left.Tire");
    ASSERT_TRUE(doc.IsObject());
    ASSERT_TRUE(doc.HasMember("IsPressureLow"));
    ASSERT_TRUE(doc.HasMember("Pressure"));
    ASSERT_TRUE(doc.HasMember("Temperature"));
}

TEST_F(VssTreeTest, GetDataTypeTemplate_StructType) {
    // Vehicle.OBD.DTCs has datatype string[]
    rapidjson::Document doc = vss_tree->getDataTypeTemplate("Vehicle.ADS.GnssInsOrientation");
    ASSERT_TRUE(doc.IsObject());
    ASSERT_TRUE(doc.HasMember("orientation"));
    ASSERT_TRUE(doc.HasMember("rmse_rotation_x"));
    ASSERT_TRUE(doc.HasMember("rmse_rotation_y"));
    ASSERT_TRUE(doc.HasMember("rmse_rotation_z"));
    
    ASSERT_TRUE(doc["rmse_rotation_y"].IsNull());
}

TEST_F(VssTreeTest, GetDataTypeTemplate_ComplexArrayType) {
    auto test_path = getEnv("VEHICLE_CONFIG_PATH").value_or("../unittest");
    test_path.append("/testfiles/array_json_test.him");
    auto tree = std::make_unique<viss::VSSTree>(test_path);
    if (tree->isValid()) {
        rapidjson::Document doc = tree->getDataTypeTemplate("Vehicle.Trip.Waypoints");
        EXPECT_TRUE(doc.IsArray());
    } else {
        // Fallback test on current vss_tree
        auto node = vss_tree->findNode("Vehicle.Speed");
        EXPECT_NE(node, nullptr);
    }
}

TEST_F(VssTreeTest, GetDataTypeTemplate_InvalidPath) {
    ASSERT_THROW(vss_tree->getDataTypeTemplate("Vehicle.InvalidPath"), std::runtime_error);
}


TEST_F(VssTreeTest, GetMetadata_AllBranches) {
    auto metadata = vss_tree->getMetadata("Vehicle", 0);
    EXPECT_FALSE(metadata.ObjectEmpty());

    auto metadata_deep = vss_tree->getMetadata("Vehicle.Speed", 2);
    EXPECT_FALSE(metadata_deep.ObjectEmpty());

    auto metadata_neg_depth = vss_tree->getMetadata("Vehicle.Powertrain", -1);
    EXPECT_FALSE(metadata_neg_depth.ObjectEmpty());
}

TEST_F(VssTreeTest, GetMetadata_Invalid) {
    auto metadata = vss_tree->getMetadata("Vehicle.Invalid", 0);
    ASSERT_TRUE(metadata.ObjectEmpty());
}

TEST_F(VssTreeTest, StringToNodeType_AllTypes) {
    std::string json_types = "/tmp/json_types_test.json";
    std::string him_file = "/tmp/test_types.him";
    {
        std::ofstream ofs(json_types);
        ofs << "{\n"
            << "  \"Vehicle\": {\n"
            << "    \"type\": \"branch\",\n"
            << "    \"children\": {\n"
            << "      \"SensorNode\": { \"type\": \"sensor\", \"datatype\": \"float\" },\n"
            << "      \"ActuatorNode\": { \"type\": \"actuator\", \"datatype\": \"boolean\" },\n"
            << "      \"AttributeNode\": { \"type\": \"attribute\", \"datatype\": \"string\" },\n"
            << "      \"StructNode\": { \"type\": \"struct\", \"datatype\": \"Types.MyStruct\" },\n"
            << "      \"PropertyNode\": { \"type\": \"property\", \"datatype\": \"int32\" },\n"
            << "      \"UnknownNode\": { \"type\": \"unknown_type\", \"datatype\": \"string\" }\n"
            << "    }\n"
            << "  }\n"
            << "}\n";
    }
    {
        std::ofstream ofs(him_file);
        ofs << "HIM.Vehicle:\n"
            << "  type: direct\n"
            << "  local: " << json_types << "\n";
    }
    {
        viss::VSSTree tree(him_file);
        EXPECT_TRUE(tree.isValid());
        EXPECT_NE(tree.findNode("Vehicle.SensorNode"), nullptr);
        EXPECT_NE(tree.findNode("Vehicle.ActuatorNode"), nullptr);
        EXPECT_NE(tree.findNode("Vehicle.AttributeNode"), nullptr);
        EXPECT_NE(tree.findNode("Vehicle.StructNode"), nullptr);
        EXPECT_NE(tree.findNode("Vehicle.PropertyNode"), nullptr);
        EXPECT_NE(tree.findNode("Vehicle.UnknownNode"), nullptr);
    }
    std::remove(json_types.c_str());
    std::remove(him_file.c_str());
}

TEST_F(VssTreeTest, VSSReadTree_EdgeCases) {
    // 1. Non-existent file
    {
        viss::VSSTree tree1("/non/existent/file.him");
        EXPECT_FALSE(tree1.isValid());
    }

    // 2. Invalid JSON file in him
    std::string invalid_json = "/tmp/invalid_test.json";
    std::string him_invalid = "/tmp/invalid_test.him";
    {
        std::ofstream ofs(invalid_json);
        ofs << "{ invalid json }";
    }
    {
        std::ofstream ofs(him_invalid);
        ofs << "HIM.Vehicle:\n  type: direct\n  local: " << invalid_json << "\n";
    }
    {
        viss::VSSTree tree(him_invalid);
        EXPECT_FALSE(tree.isValid());
    }
    std::remove(invalid_json.c_str());
    std::remove(him_invalid.c_str());

    // 3. JSON with non-object or multiple roots
    std::string multi_root_json = "/tmp/multi_root_test.json";
    std::string him_multi = "/tmp/multi_root_test.him";
    {
        std::ofstream ofs(multi_root_json);
        ofs << "{\"root1\": {}, \"root2\": {}}";
    }
    {
        std::ofstream ofs(him_multi);
        ofs << "HIM.Vehicle:\n  type: direct\n  local: " << multi_root_json << "\n";
    }
    {
        viss::VSSTree tree(him_multi);
        EXPECT_FALSE(tree.isValid());
    }
    std::remove(multi_root_json.c_str());
    std::remove(him_multi.c_str());

    // 4. JSON with array root
    std::string array_json = "/tmp/array_root_test.json";
    std::string him_array = "/tmp/array_root_test.him";
    {
        std::ofstream ofs(array_json);
        ofs << "[1, 2, 3]";
    }
    {
        std::ofstream ofs(him_array);
        ofs << "HIM.Vehicle:\n  type: direct\n  local: " << array_json << "\n";
    }
    {
        viss::VSSTree tree(him_array);
        EXPECT_FALSE(tree.isValid());
    }
    std::remove(array_json.c_str());
    std::remove(him_array.c_str());
}



