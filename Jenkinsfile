pipeline {
    agent {
        label 'OSS-Middleware'
    }

    environment {
        // Agent Node
        AGENT_LABEL="OSS-Middleware"

        // Docker Registry
        IMAGE_REGISTRY = "172.16.10.66"
        IMAGE_NAMESPACE = "oss"
        APP_NAME = "oss-multimw"

        // Dockerfile Path
        DOCKERFILE_PATH = "docker/Dockerfile" 
        // Docker images
        BUILDER_IMAGE = "${IMAGE_REGISTRY}/${IMAGE_NAMESPACE}/${APP_NAME}-builder"
        RUNTIME_IMAGE = "${IMAGE_REGISTRY}/${IMAGE_NAMESPACE}/${APP_NAME}"
        ANALYSIS_IMAGE = "${IMAGE_REGISTRY}/${IMAGE_NAMESPACE}/${APP_NAME}-analysis"
        DEBUG_IMAGE = "${IMAGE_REGISTRY}/${IMAGE_NAMESPACE}/${APP_NAME}-debug" 

        // Docker Buildx
        DOCKER_BUILDKIT = '1'
        BUILDX_BUILDER_NAME = 'multiarch-builder'
        
        // Multi Architecture
        PLATFORM_AMD64 = 'linux/amd64'
        PLATFORM_ARM64 = 'linux/arm64'
        PLATFORMS_ALL = 'linux/amd64,linux/arm64'

        // Project
        BUILD_UNITTEST = 'TRUE'
        EXTENSION_PLATFORM = 'ROS2'
        
        // Credentials
        GITLAB_CREDENTIALS = 'brian.doh'
        REGISTRY_CREDENTIALS = 'Harbor_Jenkins'
        
        GITOPS_REPO_URL = 'https://lotte-autops.com/oss_group1/oss_multisw_framework.git'
        GIT_BRANCH = "dev_Brian-GOASP-1273"

        // Sysroot
        AGENT_DIR="/jenkins/agent"
        SYSROOT_DIR = "${AGENT_DIR}/sysroot-aarch64"
        TOOLCHAIN_FILE = "${AGENT_DIR}/toolchain-aarch64.cmake"
    }

   options {
        timeout(time: 120, unit: 'MINUTES')
        disableConcurrentBuilds()
        timestamps()
    }

    stages {
        stage('Setup Docker Buildx') {
            steps {
                script {
                    sh '''
                        echo "Setting up Docker Buildx..."
                        docker buildx rm ${BUILDX_BUILDER_NAME} || true
                        docker buildx create --name ${BUILDX_BUILDER_NAME} --driver docker-container --use || true

                        echo "Check builder information..."
                        docker buildx inspect --bootstrap

                        echo "Check supported platform information..."
                        docker buildx inspect | grep Platforms

                        echo "Check Qemu support"
                        docker run --privileged --rm tonistiigi/binfmt --install arm64
                    '''
                }
            }
        }

        stage('Initialize') {
            steps {
                script {
                    echo "Checking out source code..."
                }
                git branch: "${GIT_BRANCH}",
                    credentialsId: "${GITLAB_CREDENTIALS}",
                    url: "${GITOPS_REPO_URL}"
                // Tag
                script {
                    env.COMMIT_HASH = sh(script: "git rev-parse --short HEAD", returnStdout: true).trim()
                    env.IMAGE_TAG = "${BUILD_NUMBER}-${COMMIT_HASH}"
                    env.BUILD_DATE = sh(script: "date -u +'%Y-%m-%dT%H:%M:%SZ'", returnStdout: true).trim()
                    env.BRANCH_CLEAN = sh(script: "echo ${GIT_BRANCH} | sed 's/[^a-zA-Z0-9._-]/-/g'", returnStdout: true).trim()
                }
            }
        }

        stage('Prepare sysroot for cross-compile') {
            steps {
                sh '''
                    if [ ! -f ${AGENT_DIR}/.aarch64_sysroot_completed ]; then
                        if [ -d ${SYSROOT_DIR} ]; then
                            rm -rf ${SYSROOT_DIR}
                        fi
                    fi
                    if [ ! -d ${SYSROOT_DIR} ]; then
                        debootstrap --arch=arm64 jammy ${SYSROOT_DIR} http://ports.ubuntu.com/
                        if [ ! -f ${AGENT_DIR}/ubuntu-arm64.list ]; then
                            cat <<EOF > ${AGENT_DIR}/ubuntu-arm64.list
deb http://ports.ubuntu.com/ubuntu-ports jammy main universe multiverse restricted
deb http://ports.ubuntu.com/ubuntu-ports jammy-updates main universe multiverse restricted
deb http://ports.ubuntu.com/ubuntu-ports jammy-security main universe multiverse restricted
EOF
                        fi
                        if [ ! -f ${AGENT_DIR}/toolchain-aarch64.cmake ]; then
                            cat <<EOF > ${AGENT_DIR}/toolchain-aarch64.cmake
# Target system and processor architecture
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross compiler location
SET(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
SET(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)

# Where target environment includes and libraries are
SET(CMAKE_FIND_ROOT_PATH /opt/sysroot-aarch64/)

# Search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Optional: set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON )
EOF
                        fi
                        cp ${AGENT_DIR}/ubuntu-arm64.list ${SYSROOT_DIR}/etc/apt/sources.list.d/
                        chroot ${SYSROOT_DIR} /bin/bash -c '\
                            DEBIAN_FRONTEND=noninteractive apt update \
                            && apt install -y --no-install-recommends \
                            libasio-dev \
                            libtinyxml2-dev \
                            libyaml-cpp-dev \
                            && rm -rf /var/lib/apt/lists/*
                        '
                        touch ${AGENT_DIR}/.aarch64_sysroot_completed
                    fi
                    cp ${AGENT_DIR}/toolchain-aarch64.cmake .
                    cp -rpa ${SYSROOT_DIR} .
                '''.stripIndent() 
            }
        }
        stage('Builder Stage') {
            steps {
                script {
                    sh '''
                        echo "Building builder image (AMD64 for development)..."
                        docker buildx build \
                            --platform ${PLATFORM_AMD64} \
                            --build-arg BUILD_UNITTEST=${BUILD_UNITTEST} \
                            --build-arg EXTENSION_PLATFORM=${EXTENSION_PLATFORM} \
                            -f ${DOCKERFILE_PATH} \
                            --target build-stage \
                            --load \
                            -t ${BUILDER_IMAGE}:${IMAGE_TAG} \
                            .
                    '''
                }
            }
        }
        stage('Runtime Stage') {
            steps {
                script {
                    sh '''
                        echo "Building runtime image (ARM64 for target deployment)..."
                        docker buildx build \
                            --platform ${PLATFORM_ARM64} \
                            -f ${DOCKERFILE_PATH} \
                            --target runtime \
                            --load \
                            -t ${RUNTIME_IMAGE}:${IMAGE_TAG} \
                            .
                    '''
                }
            }
        }
        stage('Static Analysis') {
            parallel {
                stage('Cppcheck') {
                    steps {
                        script {
                            echo "Running Cppcheck..."
                            sh 'mkdir -p ${WORKSPACE}/reports'
                            docker.image("${BUILDER_IMAGE}:${IMAGE_TAG}").inside("-v ${WORKSPACE}/reports:/workspace/reports") {
                                sh """
                                    mkdir -p /workspace/reports/cppcheck
                                    find /workspace/service/ \
                                        -not -path "*/unittest/*" \
                                        -name "*.cpp" \
                                        > /workspace/reports/cppcheck/files_to_cppcheck.txt || true

                                    cppcheck --enable=all \
                                        --xml \
                                        --xml-version=2 \
                                        --output-file=/workspace/reports/cppcheck/cppcheck-result.xml \
                                        --suppress=missingIncludeSystem \
                                        --suppress=unmatchedSuppression \
                                        --suppress=unusedFunction \
                                        --std=c++20 \
                                        --file-list=/workspace/reports/cppcheck/files_to_cppcheck.txt || true
                                """
                            }
                        }
                    }
                    post {
                        always {
                            publishCppcheck pattern: 'reports/cppcheck/cppcheck-result.xml'
                            archiveArtifacts artifacts: 'reports/cppcheck/cppcheck-result.xml', allowEmptyArchive: true
                        }
                    }
                }
                stage('SlocCount') {
                    steps {
                        script {
                            echo "Running SlocCount..."
                            sh 'mkdir -p ${WORKSPACE}/reports'
                            docker.image("${BUILDER_IMAGE}:${IMAGE_TAG}").inside("-v ${WORKSPACE}/reports:/workspace/reports") {
                                sh '''
                                    mkdir -p /tmp/sloccount-clean
                                    mkdir -p /workspace/reports/sloccount

                                    find /workspace/service/ \
                                        -not -path "*/unittest/*" \
                                        \\( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \\) \
                                        -exec cp --parents {} /tmp/sloccount-clean/ \\; 2>/dev/null || true

                                    cd /tmp/sloccount-clean
                                    if [ -z $(which sloccount) ]; then
                                        apt update && apt install sloccount -y
                                    fi
                                    sloccount --duplicates --wide --details . > /workspace/reports/sloccount/sloccount-detailed.xml || true
                                    sloccount . > /workspace/reports/sloccount/sloccount-summary.txt || true
                                    cd /workspace
                                '''
                            }
                        }
                    }
                    post {
                        always {
                            sloccountPublish pattern: 'reports/sloccount/sloccount-detailed.xml'
                            archiveArtifacts artifacts: 'reports/sloccount/sloccount-summary.txt', allowEmptyArchive: true
                        }
                    }
                } 
                stage('CppLint') {
                    steps {
                        script {
                            echo "Running CppLint..."
                            sh 'mkdir -p ${WORKSPACE}/reports'
                            docker.image("${BUILDER_IMAGE}:${IMAGE_TAG}").inside("-v ${WORKSPACE}/reports:/workspace/reports") {
                                sh '''
                                    mkdir -p /workspace/reports/cpplint
                                    find /workspace/service/ \
                                        -not -path "*/unittest/*" \
                                        \\( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \\) \
                                        > /workspace/reports/cpplint/files_to_lintcheck.txt
                                    
                                    if [ -f /workspace/reports/cpplint/files_to_lintcheck.txt ]; then
                                        cat /workspace/reports/cpplint/files_to_lintcheck.txt | xargs cpplint \
                                            --filter=-whitespace/tab,-legal/copyright,-build/include_order \
                                            --linelength=120 \
                                            --extensions=cpp,cc,h,hpp \
                                            --output=vs7 \
                                            --counting=detailed \
                                            > /workspace/reports/cpplint/cpplint-result.txt 2>&1 || true
                                    else
                                        echo "No files to check" > /workspace/reports/cpplint/cpplint-result.txt
                                    fi
                                '''
                            }
                        }
                    }
                    post {
                        always {
                            archiveArtifacts artifacts: 'reports/cpplint/cpplint-result.txt', allowEmptyArchive: true
                        }
                    }
                }
            }
        }
    }
}