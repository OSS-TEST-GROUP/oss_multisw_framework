#include "gtest/gtest.h"
#include "VssTree.h"
#include <iostream>
#include <vector>
#include <string>

class VssTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // The test must be run from the build directory.
        vss_tree = new viss::VSSTree("../config/viss.him");
        if (!vss_tree->isValid()) {
            FAIL() << "Failed to load VSS tree from ../config/viss.him";
        }
    }

    void TearDown() override {
        delete vss_tree;
    }

    viss::VSSTree* vss_tree;
};

TEST_F(VssTreeTest, FindNodeBySimplePath) {
    std::string root_name;
    viss::Node_t* root_node = vss_tree->findRootNode("Vehicle", root_name);
    ASSERT_NE(root_node, nullptr);

    std::vector<std::string> nodes;
    root_node->resolvePathPattern("Vehicle", "Speed", nodes);
    ASSERT_EQ(nodes.size(), 1);
    EXPECT_EQ(nodes[0], "Vehicle.Speed");
}

TEST_F(VssTreeTest, FindNodeByWildcardPath) {
    std::string root_name;
    viss::Node_t* root_node = vss_tree->findRootNode("Vehicle", root_name);
    ASSERT_NE(root_node, nullptr);

    std::vector<std::string> nodes;
    root_node->resolvePathPattern("Vehicle", "ADAS.*.IsEngaged", nodes);
    EXPECT_GT(nodes.size(), 1); 
}

TEST_F(VssTreeTest, PathNotFound) {
    std::string root_name;
    viss::Node_t* root_node = vss_tree->findRootNode("Vehicle", root_name);
    ASSERT_NE(root_node, nullptr);

    std::vector<std::string> nodes;
    root_node->resolvePathPattern("Vehicle", "This.Path.Does.Not.Exist", nodes);
    EXPECT_TRUE(nodes.empty());
}

TEST_F(VssTreeTest, WildcardAtTheEnd) {
    std::string root_name;
    viss::Node_t* root_node = vss_tree->findRootNode("Vehicle", root_name);
    ASSERT_NE(root_node, nullptr);

    std::vector<std::string> nodes;
    root_node->resolvePathPattern("Vehicle", "ADAS.LaneDepartureDetection.*", nodes);
    EXPECT_FALSE(nodes.empty());
}

TEST_F(VssTreeTest, FindBranchAndGetAllChildren) {
    std::string root_name;
    viss::Node_t* root_node = vss_tree->findRootNode("Vehicle", root_name);
    ASSERT_NE(root_node, nullptr);

    std::vector<std::string> nodes;
    root_node->resolvePathPattern("Vehicle", "ADAS", nodes);

    // The result should contain all descendants of ADAS, but not "Vehicle.ADAS" itself.
    EXPECT_GT(nodes.size(), 1);

    // Verify that the branch path "Vehicle.ADAS" is NOT in the results.
    for (const auto& path : nodes) {
        ASSERT_NE(path, "Vehicle.ADAS");
    }
}
