pipeline {
    environment {
        // Agent Node
        AGENT_LABEL="OSS-Middleware"

        // 이미지 관련 환경변수
        IMAGE_REGISTRY = "172.16.10.66"
        IMAGE_NAMESPACE = "oss"
        APP_NAME = "oss-multimw"
        
        // Builder image
        //BUILDER_IMAGE = "${IMAGE_REGISTRY}/${IMAGE_NAMESPACE}/${APP_NAME}-builder"
        // Runtime image
        //RUNTIME_IMAGE = "${IMAGE_REGISTRY}/${IMAGE_NAMESPACE}/${APP_NAME}"
        
        // Tag
        IMAGE_TAG = "${BUILD_NUMBER}"
        COMMIT_HASH = sh(script: "git rev-parse --short HEAD", returnStdout: true).trim()
        
        // Credentials
        GITLAB_CREDENTIALS = 'brian.doh'
        //REGISTRY_CREDENTIALS = 'docker-registry-credentials'
        
        // GitOps 레포지토리
        GITOPS_REPO_URL = 'git@lotte-autops.com:oss_group1/oss_multisw_framework.git'
    }
    agent { label "${AGENT_LABEL}" } 
    
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
    }
}