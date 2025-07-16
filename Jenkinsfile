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
        DEBUG_IMAGE = "${IMAGE_REGISTRY}/${IMAGE_NAMESPACE}/${APP_NAME}-debug"
        
        // Tag
        COMMIT_HASH = sh(script: "git rev-parse --short HEAD", returnStdout: true).trim()
        IMAGE_TAG = "${BUILD_NUMBER}-${COMMIT_HASH}"
        BUILD_DATE = sh(script: "date -u +'%Y-%m-%dT%H:%M:%SZ'", returnStdout: true).trim()
        BRANCH_CLEAN = sh(script: "echo ${BRANCH_NAME} | sed 's/[^a-zA-Z0-9._-]/-/g'", returnStdout: true).trim()

        // Docker Buildx
        DOCKER_BUILDKIT = '1'
        BUILDX_BUILDER_NAME = 'multiarch-builder'
        
        // Multi Architecture
        PLATFORMS_AMD64 = 'linux/amd64'
        PLATFORMS_ARM64 = 'linux/arm64'
        PLATFORMS_ALL = 'linux/amd64,linux/arm64'

        // Project
        BUILD_UNITTEST = 'TRUE'
        EXTENSION_PLATFORM = 'ROS2'
        
        // Credentials
        GITLAB_CREDENTIALS = 'brian.doh'
        REGISTRY_CREDENTIALS = 'Harbor_Jenkins'
        
        GITOPS_REPO_URL = 'https://lotte-autops.com/oss_group1/oss_multisw_framework.git'
    }

   options {
        //buildDiscarder(logRotator(numToKeepStr: '20'))
        timeout(time: 120, unit: 'MINUTES')
        disableConcurrentBuilds()
        timestamps()
    }

    stages {
        stage('Checkout') {
            steps {
                script {
                    echo "Checking out source code..."
                }
                git branch: 'dev_Brian-GOASP-1273',
                    credentialsId: "${GITLAB_CREDENTIALS}",
                    url: "${GITOPS_REPO_URL}"
                
                script {
                    env.FULL_TAG = "${IMAGE_TAG}-${BRANCH_CLEAN}"
                }
            }
        }

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
                        docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
                    '''
                }
            }
        }

        stage('Multi-Architecture Build') {
            parallel {
                stage('Builder Image') {
                    steps {
                        script {
                            sh '''
                                echo "Building builder image..."
                                docker buildx build \
                                    --platform ${env.PLATFORM_ARM64} \
                                    --build-arg BUILD_UNITTEST=${env.BUILD_UNITTEST} \
                                    --build-arg EXTENSION_PLATFORM=${env.EXTENSION_PLATFORM} \
                                    -f ${env.DOCKERFILE_PATH} \
                                    --target build-stage \
                                    --load \
                                    -t ${env.BUILDER_IMAGE}:${env.IMAGE_TAG} \
                                    .
                            '''
                        }
                    }
                }
                stage('Runtime Image') {
                    steps {
                        script {
                            sh '''
                                echo "Building runtime image..."
                                docker buildx build \
                                    --platform ${env.PLATFORM_ARM64} \
                                    -f ${env.DOCKERFILE_PATH} \
                                    --target runtime \
                                    --load \
                                    -t ${env.RUNTIME_IMAGE}:${env.IMAGE_TAG} \
                                    .
                            '''
                        }
                    }
                }
                stage('Debug Image') {
                    steps {
                        script {
                            sh '''
                                echo "Building debugging image..."
                                docker buildx build \
                                    --platform ${env.PLATFORM_ARM64} \
                                    -f ${env.DOCKERFILE_PATH} \
                                    --target debug \
                                    --load \
                                    -t ${env.DEBUG_IMAGE}:${env.IMAGE_TAG} \
                                    .
                            '''
                        }
                    }
                }
                stage('Static Analysis Image') {
                    steps {
                        script {
                            sh '''
                                echo "Building static analysis image..."
                                docker buildx build \
                                    --platform ${env.PLATFORM_ARM64} \
                                    -f ${env.DOCKERFILE_PATH} \
                                    --target static-analysis \
                                    --load \
                                    -t ${env.BUILDER_IMAGE}:analysis-${env.PLATFORM_ARM64} \
                                    .
                            '''
                        }
                    }
                }
            }
        }
        stage('Static Analysis') {
            parallel {
                stage('Cppcheck') {
                    steps {
                        script {
                            echo "Running Cppcheck..."
                            docker.image("${BUILDER_IMAGE}:analysis-${PLATFORM_ARM64}").inside() {
                                sh """
                                    find /workspace/service/ \
                                        -not -path "*/unittest/*" \
                                        -name "*.cpp" \
                                        > /workspace/reports/files_to_cppcheck.txt || true

                                    cppcheck --enable=all \
                                        --xml \
                                        --xml-version=2 \
                                        --output-file=/workspace/reports/cppcheck-result.xml \
                                        --suppress=missingIncludeSystem \
                                        --suppress=unmatchedSuppression \
                                        --suppress=unusedFunction \
                                        --std=c++20 \
                                        --file-list=/workspace/files_to_cppcheck.txt || true
                                """
                            }
                        }
                    }
                    post {
                        always {
                            publishCppcheck pattern: 'reports/cppcheck-result.xml'
                            archiveArtifacts artifacts: 'reports/cppcheck-result.xml', allowEmptyArchive: true
                        }
                    }
                }
                stage('SlocCount') {
                    steps {
                        script {
                            echo "Running SlocCount..."
                            docker.image("${BUILDER_IMAGE}:analysis-${PLATFORM_ARM64}").inside() {
                                sh '''
                                    mkdir -p /tmp/sloccount-clean

                                    find /workspace/service/ \
                                        -not -path "*/unittest/*" \
                                        \\( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \\) \
                                        -exec cp --parents {} /tmp/sloccount-clean/ \\; 2>/dev/null || true

                                    cd /tmp/sloccount-clean
                                    if [ -z $(which sloccount) ]; then
                                        apt update && apt install sloccount -y
                                    fi
                                    sloccount --duplicates --wide --details . > /workspace/reports/sloccount-detailed.xml || true
                                    sloccount . > /workspace/reports/sloccount-summary.txt || true
                                    cd /workspace
                                '''
                            }
                        }
                    }
                    post {
                        always {
                            sloccountPublish pattern: 'reports/sloccount-detailed.xml'
                            archiveArtifacts artifacts: 'reports/sloccount-summary.txt', allowEmptyArchive: true
                        }
                    }
                } 
                stage('CppLint') {
                    steps {
                        script {
                            echo "Running CppLint..."
                            docker.image("${BUILDER_IMAGE}:analysis-${PLATFORM_ARM64}").inside() {
                                sh '''
                                    find /workspace/service/ \
                                        -not -path "*/unittest/*" \
                                        \\( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \\) \
                                        > /workspace/files_to_lintcheck.txt
                                    
                                    if [ -f /workspace/files_to_lintcheck.txt ]; then
                                        cat /workspace/files_to_lintcheck.txt | xargs cpplint \
                                            --filter=-whitespace/tab,-legal/copyright,-build/include_order \
                                            --linelength=120 \
                                            --extensions=cpp,cc,h,hpp \
                                            --output=vs7 \
                                            --counting=detailed \
                                            > /workspace/reports/cpplint-result.txt 2>&1 || true
                                    else
                                        echo "No files to check" > /workspace/reports/cpplint-result.txt
                                    fi
                                '''
                            }
                        }
                    }
                    post {
                        always {
                            archiveArtifacts artifacts: 'reports/cpplint-result.txt', allowEmptyArchive: true
                        }
                    }
                }
            }
        }
    }
}