pipeline {
    agent {
        label 'OSS-Middleware'
    }
    parameters {
        choice(
            name: 'BUILD_ARCHITECTURE',
            choices: ['both', 'amd64', 'arm64'],
            description: 'Target architecture for build'
        )
        choice(
            name: 'DOCKERFILE_TYPE',
            choices: ['auto', 'production', 'development', 'debug'],
            description: 'Select Dockerfile type'
        )
        booleanParam(
            name: 'SKIP_STATIC_ANALYSIS',
            defaultValue: false,
            description: 'Skip static analysis for faster builds'
        )
        booleanParam(
            name: 'SKIP_TESTS',
            defaultValue: true,
            description: 'Skip unit tests'
        )
        booleanParam(
            name: 'PUSH_TO_REGISTRY',
            defaultValue: false,
            description: 'Push images to registry'
        )
    }
    environment {
        // Agent Node
        AGENT_LABEL="OSS-Middleware"

        // 이미지 관련 환경변수
        IMAGE_REGISTRY = "172.16.10.66"
        IMAGE_NAMESPACE = "oss"
        APP_NAME = "oss-multimw"
        
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
        
        // Credentials
        GITLAB_CREDENTIALS = 'brian.doh'
        //REGISTRY_CREDENTIALS = 'docker-registry-credentials'
        
        // GitOps 레포지토리
        GITOPS_REPO_URL = 'git@lotte-autops.com:oss_group1/oss_multisw_framework.git'
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
                    url: 'git@lotte-autops.com:oss_group1/oss_multisw_framework.git'
                
                script {
                    env.FULL_TAG = "${IMAGE_TAG}-${COMMIT_HASH}"
                }
            }
        }
        stage('Build Static Analysis Image') {
            steps {
                script {
                    echo "Building static analysis/builder image..."
                    def builderImage = docker.build(
                        "${BUILDER_IMAGE}:${FULL_TAG}", 
                        "--target builder --cache-from ${BUILDER_IMAGE}:latest ."
                    )
                    // set TAG
                    builderImage.tag("latest")
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
                                        src/ || true
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
            }
        }
    }
}

// ========== Helper Functions ==========

def determineDockerfilePath() {
    switch(params.DOCKERFILE_TYPE) {
        case 'production':
            return fileExists('docker/Dockerfile.prod') ? 'docker/Dockerfile.prod' : 'docker/Dockerfile'
        case 'development':
            return fileExists('docker/Dockerfile.dev') ? 'docker/Dockerfile.dev' : 'docker/Dockerfile'
        case 'debug':
            return fileExists('docker/Dockerfile.debug') ? 'docker/Dockerfile.debug' : 'docker/Dockerfile'
        case 'auto':
        default:
            // 브랜치에 따른 자동 선택
            switch(env.BRANCH_NAME) {
                case 'main':
                    return fileExists('docker/Dockerfile.prod') ? 'docker/Dockerfile.prod' : 'docker/Dockerfile'
                case 'develop':
                    return fileExists('docker/Dockerfile.dev') ? 'docker/Dockerfile.dev' : 'docker/Dockerfile'
                default:
                    return 'docker/Dockerfile'
            }
    }
}

def buildMultiArchImage(Map config) {
    def imageName = config.imageName
    def target = config.target ?: ''
    def dockerfile = config.dockerfile ?: env.DOCKERFILE_PATH
    def platforms = config.platforms ?: env.TARGET_PLATFORMS
    def push = config.push ?: false
    def cache = config.cache ?: false
    
    def buildCommand = "docker buildx build"
    buildCommand += " --platform ${platforms}"
    buildCommand += " -f ${dockerfile}"
    
    if (target) {
        buildCommand += " --target ${target}"
    }
    
    if (cache) {
        buildCommand += " --cache-from type=registry,ref=${imageName}:cache"
        buildCommand += " --cache-to type=registry,ref=${imageName}:cache,mode=max"
    }
    
    // 빌드 인자 추가
    buildCommand += " --build-arg BUILDPLATFORM=\$BUILDPLATFORM"
    buildCommand += " --build-arg TARGETPLATFORM=\$TARGETPLATFORM"
    buildCommand += " --build-arg BUILD_DATE=${env.BUILD_DATE}"
    buildCommand += " --build-arg GIT_COMMIT=${env.COMMIT_HASH}"
    buildCommand += " --build-arg VERSION=${env.IMAGE_TAG}"
    
    if (push) {
        buildCommand += " --push"
    } else {
        buildCommand += " --load"
    }
    
    buildCommand += " -t ${imageName}:${env.IMAGE_TAG}"
    buildCommand += " ."
    
    echo "Executing: ${buildCommand}"
    sh buildCommand
}

def pushMultiArchImage(Map config) {
    def imageName = config.imageName
    def tag = config.tag
    def platforms = config.platforms
    def additionalTags = config.additionalTags ?: []
    
    // 메인 태그로 푸시
    sh """
        docker buildx build \\
            --platform ${platforms} \\
            -f ${env.DOCKERFILE_PATH} \\
            --push \\
            -t ${imageName}:${tag} \\
            .
    """
    
    // 추가 태그들 푸시
    additionalTags.each { additionalTag ->
        sh """
            docker buildx build \\
                --platform ${platforms} \\
                -f ${env.DOCKERFILE_PATH} \\
                --push \\
                -t ${imageName}:${additionalTag} \\
                .
        """
    }
}