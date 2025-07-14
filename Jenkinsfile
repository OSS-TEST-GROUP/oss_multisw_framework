pipeline {
    agent {
        label 'OSS-Middleware'
    }

    environment {
        // Agent Node
        AGENT_LABEL="OSS-Middleware"

        // 이미지 관련 환경변수
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
        IMAGE_TAG = "${BUILD_NUMBER}"
        COMMIT_HASH = sh(script: "git rev-parse --short HEAD", returnStdout: true).trim()
        BUILD_DATE = sh(script: "date -u +'%Y-%m-%dT%H:%M:%SZ'", returnStdout: true).trim()

        // Docker Buildx
        DOCKER_BUILDKIT = '1'
        BUILDX_BUILDER_NAME = 'multiarch-builder'
        
        // Multi Architecture
        PLATFORMS_AMD64 = 'linux/amd64'
        PLATFORMS_ARM64 = 'linux/arm64'
        PLATFORMS_ALL = 'linux/amd64,linux/arm64'

        // Project
        BUILD_UNITTEST = 'FALSE'
        EXTENSION_PLATFORM = 'ROS2'
        
        // Credentials
        GITLAB_CREDENTIALS = 'brian.doh'
        //REGISTRY_CREDENTIALS = 'docker-registry-credentials'
        
        // GitOps 레포지토리
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
                git branch: 'dev_brian-GOASP-1272',
                    credentialsId: "${GITLAB_CREDENTIALS}",
                    url: "${GITOPS_REPO_URL}"
                
                script {
                    env.FULL_TAG = "${IMAGE_TAG}-${COMMIT_HASH}"
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

        stage('Build Static Analysis Image') {
            steps {
                script {
                    env.PLATFORM_AMD64 = "${PLATFORMS_AMD64}"
                    env.EXTENSION_PLATFORM = "${EXTENSION_PLATFORM}"
                    env.BUILD_UNITTEST = "${BUILD_UNITTEST}"
                    env.DOCKERFILE_PATH = "${DOCKERFILE_PATH}"

                    sh '''
                        echo "Building static analysis/builder image..."
                        docker buildx build \
                            --platform ${PLATFORM_AMD64} \
                            --build-arg BUILD_UNITTEST=${BUILD_UNITTEST} \
                            --build-arg EXTENSION_PLATFORM=${EXTENSION_PLATFORM} \
                            -f ${DOCKERFILE_PATH} \
                            --target build-stage \
                            --load \
                            -t ${BUILDER_IMAGE}:${FULL_TAG} \
                            .
                    ''' 
                }
            }
        }
        stage('Static Analysis') {
            parallel {
                stage('Cppcheck Analysis') {
                    steps {
                        script {
                            echo "Running Cppcheck..."
                            docker.image("${BUILDER_IMAGE}:${FULL_TAG}").inside() {
                                sh '''
                                    mkdir -p reports
                                    cppcheck --enable=all \
                                        --xml --xml-version=2 \
                                        --output-file=reports/cppcheck-result.xml \
                                        --suppress=missingIncludeSystem \
                                        --suppress=unmatchedSuppression \
                                        /workspace/service || true
                                '''
                            }
                        }
                    }
                    post {
                        always {
                            publishCppcheck pattern: 'reports/cppcheck-result.xml'
                        }
                    }
                }
                stage('Unittest') {
                    steps {
                        script {
                            echo "Running Unittest..."
                        }
                    }
                }
            }
        }
    }
}