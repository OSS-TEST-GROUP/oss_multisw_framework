// Unit tests for Container class using Google Test
#include <gtest/gtest.h>

#include "service/Container.cpp"

using namespace ros2::adsw_manager;

class ContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        container = std::make_unique<Container>("test-container", "test-image:latest", 
                                                  "/path/to/deploy.yaml", "test-namespace");
    }

    void TearDown() override {
        container.reset();
    }

    std::unique_ptr<Container> container;
};

TEST_F(ContainerTest, Constructor_InitialValues_TC001) {
    EXPECT_EQ(container->GetName(), "test-container");
    EXPECT_EQ(container->GetImage(), "test-image:latest");
    EXPECT_EQ(container->GetDeployFile(), "/path/to/deploy.yaml");
    EXPECT_EQ(container->GetNamespace(), "test-namespace");
}

TEST_F(ContainerTest, GetPodName_TC002) {
    // Pod name should return the container name
    EXPECT_EQ(container->GetPodName(), "test-container");
}

TEST_F(ContainerTest, EmptyValues_TC003) {
    Container empty("", "", "", "");
    EXPECT_EQ(empty.GetName(), "");
    EXPECT_EQ(empty.GetImage(), "");
    EXPECT_EQ(empty.GetDeployFile(), "");
    EXPECT_EQ(empty.GetNamespace(), "");
}

TEST_F(ContainerTest, LongValues_TC004) {
    std::string long_name(500, 'a');
    std::string long_image(500, 'b');
    std::string long_deploy(500, 'c');
    std::string long_namespace(500, 'd');
    
    Container long_container(long_name, long_image, long_deploy, long_namespace);
    EXPECT_EQ(long_container.GetName(), long_name);
    EXPECT_EQ(long_container.GetImage(), long_image);
    EXPECT_EQ(long_container.GetDeployFile(), long_deploy);
    EXPECT_EQ(long_container.GetNamespace(), long_namespace);
}

TEST_F(ContainerTest, SpecialCharacters_TC005) {
    Container special("container-name_123", "image:v1.0.0-beta", 
                      "/path/to/deploy-file.yaml", "namespace-test");
    EXPECT_EQ(special.GetName(), "container-name_123");
    EXPECT_EQ(special.GetImage(), "image:v1.0.0-beta");
    EXPECT_EQ(special.GetDeployFile(), "/path/to/deploy-file.yaml");
    EXPECT_EQ(special.GetNamespace(), "namespace-test");
}

TEST_F(ContainerTest, ImageWithRegistry_TC006) {
    Container registry_container("web-app", "docker.io/library/nginx:alpine", 
                                  "/deploy/nginx.yaml", "production");
    EXPECT_EQ(registry_container.GetImage(), "docker.io/library/nginx:alpine");
}

TEST_F(ContainerTest, ComplexPaths_TC007) {
    Container complex("app", "image:latest", 
                      "/very/long/path/to/deployment/files/deploy.yaml", 
                      "default");
    EXPECT_EQ(complex.GetDeployFile(), "/very/long/path/to/deployment/files/deploy.yaml");
}

TEST_F(ContainerTest, NamespaceVariations_TC008) {
    Container ns1("c1", "img", "file", "default");
    Container ns2("c2", "img", "file", "kube-system");
    Container ns3("c3", "img", "file", "my-custom-namespace");
    
    EXPECT_EQ(ns1.GetNamespace(), "default");
    EXPECT_EQ(ns2.GetNamespace(), "kube-system");
    EXPECT_EQ(ns3.GetNamespace(), "my-custom-namespace");
}

TEST_F(ContainerTest, ImageTagVariations_TC009) {
    Container latest("c1", "app:latest", "file", "ns");
    Container versioned("c2", "app:1.2.3", "file", "ns");
    Container sha("c3", "app@sha256:abc123", "file", "ns");
    
    EXPECT_EQ(latest.GetImage(), "app:latest");
    EXPECT_EQ(versioned.GetImage(), "app:1.2.3");
    EXPECT_EQ(sha.GetImage(), "app@sha256:abc123");
}

TEST_F(ContainerTest, DeployFileExtensions_TC010) {
    Container yaml("c1", "img", "deploy.yaml", "ns");
    Container yml("c2", "img", "deploy.yml", "ns");
    Container json("c3", "img", "deploy.json", "ns");
    
    EXPECT_EQ(yaml.GetDeployFile(), "deploy.yaml");
    EXPECT_EQ(yml.GetDeployFile(), "deploy.yml");
    EXPECT_EQ(json.GetDeployFile(), "deploy.json");
}

TEST_F(ContainerTest, PodNameMatchesContainerName_TC011) {
    Container c1("my-pod", "image", "file", "ns");
    EXPECT_EQ(c1.GetPodName(), c1.GetName());
}

TEST_F(ContainerTest, MultipleContainers_TC012) {
    Container c1("container1", "img1", "file1", "ns1");
    Container c2("container2", "img2", "file2", "ns2");
    Container c3("container3", "img3", "file3", "ns3");
    
    EXPECT_NE(c1.GetName(), c2.GetName());
    EXPECT_NE(c2.GetName(), c3.GetName());
    EXPECT_NE(c1.GetName(), c3.GetName());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
