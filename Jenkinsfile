pipeline {
    agent {
        label 'OSS-Agent'
    }

    environment {
        // Agent Node
        AGENT_LABEL="OSS-Agent"

        // Docker Registry
        IMAGE_REGISTRY = "172.16.10.216:80"
        IMAGE_NAMESPACE = "oss"
        APP_NAME = "oss-mmf"
        REGISTRY_ID = "builder"
        REGISTRY_PWD = "Obigoam1@se"

        // Dockerfile Path
        DOCKERFILE_PATH = "docker/Dockerfile" 
        // Docker images
        BUILDER_IMAGE = "${IMAGE_REGISTRY}/${IMAGE_NAMESPACE}/${APP_NAME}-builder:latest"
        //RUNTIME_IMAGE = "${IMAGE_REGISTRY}/${IMAGE_NAMESPACE}/${APP_NAME}"
        //ANALYSIS_IMAGE = "${IMAGE_REGISTRY}/${IMAGE_NAMESPACE}/${APP_NAME}-analysis"
        //DEBUG_IMAGE = "${IMAGE_REGISTRY}/${IMAGE_NAMESPACE}/${APP_NAME}-debug" 

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
        GITHUB_CREDENTIALS = 'github-cred'
        GITLAB_CREDENTIALS = 'brian.doh'
        REGISTRY_CREDENTIALS = 'Harbor_Jenkins'
        
        GITHUB_REPO_URL = 'https://github.com/obigo-gov/oss_multisw_framework.git'
        GITOPS_REPO_URL = 'https://lotte-autops.com/oss_group1/oss_multisw_framework.git'
        GIT_BRANCH = "develop"

        // Sysroot
        SDK_DIR="${HOME}/obigo-sdk"
        //SYSROOT_DIR = "${SDK_DIR}/sysroot-aarch64"
        //TOOLCHAIN_FILE = "${SDK_DIR}/toolchain-aarch64.cmake"
    }

   options {
        timeout(time: 120, unit: 'MINUTES')
        disableConcurrentBuilds()
        timestamps()
    }

    stages {
        stage('Setup Environment') {
            steps {
                script {
                    echo "Login Docker Private Registry. If registry didn't saved"
                    sh '''
                        if ! docker info --format '{{json .RegistryConfig}}' 2>/dev/null \
                            | grep -q "${IMAGE_REGISTRY}"; then
                            if [ ! -f /etc/docker/daemon.json ]; then
                                sudo mkdir -p /etc/docker
                                echo '{"insecure-registries": []}' | sudo tee /etc/docker/daemon.json > /dev/null
                            else
                                jq 'if has("insecure-registries") then . else . + {"insecure-registries": []} end' \
                                    /etc/docker/daemon.json > /tmp/daemon.json
                                sudo cp /etc/docker/daemon.json /etc/docker/daemon.json.bak
                                sudo cp /tmp/daemon.json /etc/docker/daemon.json
                            fi
                            sudo sed -i "s/\\[\\]/[\"${IMAGE_REGISTRY}\"]/" /etc/docker/daemon.json
                            sudo systemctl restart docker
                        
                            echo "${REGISTRY_PWD}" | docker login ${IMAGE_REGISTRY} -u ${REGISTRY_ID} --password-stdin
                        fi
                    '''
                }
            }
        }
        stage('Sync Sources') {
            steps {
                echo "Cleaning up the workspace..."
                cleanWs()

                echo "Checking out source code..."
                checkout([
                    $class: 'GitSCM',
                    branches: [[name: "*/${GIT_BRANCH}"]],
                    userRemoteConfigs: [[
                        url: "${GITHUB_REPO_URL}",
                        credentialsId: "${GITHUB_CREDENTIALS}"
                    ]]
                ])
                // Tag
                script {
                    env.COMMIT_HASH = sh(script: "git rev-parse --short HEAD", returnStdout: true).trim()
                    env.IMAGE_TAG = "${BUILD_NUMBER}-${COMMIT_HASH}"
                    env.BUILD_DATE = sh(script: "date -u +'%Y-%m-%dT%H:%M:%SZ'", returnStdout: true).trim()
                    env.BRANCH_CLEAN = sh(script: "echo ${GIT_BRANCH} | sed 's/[^a-zA-Z0-9._-]/-/g'", returnStdout: true).trim()
                }
                // Dependency library - FastDDS
                script {
                    sh '''
                        if [ ! -d ${SDK_DIR}/Fast-DDS ]; then
                            if [ ! -d ${SDK_DIR}/venv ]; then
                                python3 -m venv ${SDK_DIR}/venv
                            fi
                            . ${SDK_DIR}/venv/bin/activate
                            if [ -z $(which vcs) ]; then
                                pip3 install --no-cache-dir \
                                    colcon-common-extensions \
                                    vcstool \
                                    >/dev/null 2>&1
                            fi
                            
                            mkdir -p ${SDK_DIR}/Fast-DDS/src
                            pushd ${SDK_DIR}/Fast-DDS >/dev/null 2>&1
                            cd ${SDK_DIR}/Fast-DDS
                            wget https://raw.githubusercontent.com/eProsima/Fast-DDS/3.2.2/fastdds.repos
                            vcs import src < fastdds.repos
                            sed -i "s#-Wpedantic##g" ${SDK_DIR}/Fast-DDS/src/fastdds/CMakeLists.txt
                            deactivate
                        fi
                    '''
                }
            }
        }
        stage('Pull Builder Container') {
            steps {
                script {
                    sh '''
                        if ! curl -u "${REGISTRY_ID}:${REGISTRY_PWD}" -s http://${IMAGE_REGISTRY}/v2/_catalog 2>/dev/null \
                            | grep -q "${APP_NAME}-builder"; then
                            docker build -t ${BUILDER_IMAGE} \
                                -f ${PWD}/docker/build_environment/Dockerfile .
                            docker push ${BUILDER_IMAGE}
                        else
                            if [[ -z $(docker images -q "${BUILDER_IMAGE}" ]]; then
                                docker pull ${BUILDER_IMAGE}
                            fi
                        fi
                    '''
                }
                // Build Fast-DDS for x86_64 indisde builder
                script {
                   if (!fileExists("${SDK_DIR}/Fast-DDS/output")) {
                       docker.image("${BUILDER_IMAGE}").inside('-u 1000:1000 -v ${SDK_DIR}:/opt') {
                           sh '''
                               #!/bin/bash
                               set -e
                               cd /opt/Fast-DDS
                               colcon build \
                                   --merge-install \
                                   --parallel-workers $(nproc) \
                                   --build-base build/ros2_x86_64 \
                                   --install-base output/ros2_x86_64
                           '''
                       }
                    }
                }
            }
        }
        // Static Analalysis
        stage('SlocCount') {
            steps {
                script {
                    echo "Running SlocCount..."
                    sh '''
                        if [ -d /tmp/sloccount-clean ]; then rm -rf /tmp/sloccount-clean; fi
                        mkdir -p /tmp/sloccount-clean
                        mkdir -p ${WORKSPACE}/output/reports/sloccount

                        find ${WORKSPACE}/service/ \
                            -not -path "*/unittest/*" \
                            -not -path "*/rapidjson/*" \
                            -not -path "*/nlohmann/*" \
                            \\( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cc" \\) \
                            -exec cp --parents {} /tmp/sloccount-clean/ \\; 2>/dev/null || true

                        cd /tmp/sloccount-clean
                        if [ -z $(which sloccount) ]; then
                            apt update && apt install sloccount -y
                        fi
                        sloccount --duplicates --wide --details . > ${WORKSPACE}/output/reports/sloccount/sloccount-detailed.xml || true
                        sloccount . > ${WORKSPACE}/output/reports/sloccount/sloccount-summary.txt || true
                        cd ${WORKSPACE}
                    '''
                }
            }
            post {
                always {
                    sloccountPublish pattern: 'output/reports/sloccount/sloccount-detailed.xml'
                    archiveArtifacts artifacts: 'output/reports/sloccount/sloccount-summary.txt', allowEmptyArchive: true
                }
            }
        }
        stage('Cppcheck') {
            steps {
                script {
                    echo "Running Cppcheck..."
                    sh '''
                        mkdir -p ${WORKSPACE}/output/reports/cppcheck
                        if [ ! -f ${WORKSPACE}/output/reports/files_to_analysis.txt ]; then
                            find ${WORKSPACE}/service/ \
                                -not -path "*/unittest/*" \
                                -not -path "*/rapidjson/*" \
                                -not -path "*/nlohmann/*" \
                                \\( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cc" \\) \
                                > ${WORKSPACE}/output/reports/files_to_analysis.txt || true
                        fi

                        cppcheck --enable=all \
                            --xml \
                            --xml-version=2 \
                            --output-file=${WORKSPACE}/output/reports/cppcheck/cppcheck-result.xml \
                            --suppress=missingIncludeSystem \
                            --suppress=unmatchedSuppression \
                            --suppress=unusedFunction \
                            --suppress=missingInclude \
                            --std=c++20 \
                            --file-list=${WORKSPACE}/output/reports/cppcheck/files_to_cppcheck.txt || true
                    '''
                }
            }
            post {
                always {
                    archiveArtifacts artifacts: 'output/reports/cppcheck/cppcheck-result.xml', allowEmptyArchive: true
                    recordIssues(
                        tools: [
                            cppCheck(name: 'CppCheck', pattern: 'output/reports/cppcheck/cppcheck-result.xml')
                        ]
                    )
                }
            }
        }
        stage('CppLint') {
            steps {
                script {
                    echo "Running CppLint..."
                    sh '''
                        mkdir -p ${WORKSPACE}/output/reports/cpplint
                        if [ ! -f ${WORKSPACE}/output/reports/files_to_analysis.txt ]; then
                            find ${WORKSPACE}/service/ \
                                -not -path "*/unittest/*" \
                                -not -path "*/rapidjson/*" \
                                -not -path "*/nlohmann/*" \
                                \\( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cc" \\) \
                                > ${WORKSPACE}/output/reports/files_to_analysis.txt
                        fi
                        cat ${WORKSPACE}/output/reports/files_to_analysis.txt | xargs cpplint \
                            --filter=-whitespace/tab,-legal/copyright,-build/include_order,-build/c++11,-build/header_guard \
                            --linelength=80 \
                            --extensions=cpp,cc,h,hpp \
                            --counting=detailed \
                            > ${WORKSPACE}/output/reports/cpplint/cpplint-result.txt 2>&1 || true
                    '''
                }
            }
            post {
                always {
                    archiveArtifacts artifacts: 'output/reports/cpplint/cpplint-result.txt', allowEmptyArchive: true
                    recordIssues(
                        tools: [
                            cppLint(name: 'CppLint', pattern: 'output/reports/cpplint/cpplint-result.txt')
                        ]
                    )
                    
                }
            }
        }
        stage('Clang-tidy') {
            steps {
                script {
                    sh 'bash -c "mkdir -p ${WORKSPACE}/output"'
                    def dockerArgs = [
                        '-u 1000:1000',
                        '-w /home/ossuser/workspace',
                        "-e BUILD_MODE=Debug",
                        "-v ${SDK_DIR}:/opt",
                        "-v ${WORKSPACE}/CMakeLists.txt:/home/ossuser/workspace/CMakeLists.txt",
                        "-v ${WORKSPACE}/service:/home/ossuser/workspace/service",
                        "-v ${WORKSPACE}/idl:/home/ossuser/workspace/idl",
                        "-v ${WORKSPACE}/output:/home/ossuser/workspace/output"
                    ].join(' ')
                    docker.image("${BUILDER_IMAGE}").inside(dockerArgs) {
                        sh '''
                            bash -c '
                            set -e
                            workdir=/home/ossuser/workspace
                            mkdir -p ${workdir}/output/reports/clang-tidy
                            targets=(core ros2 agl)
                            source /opt/Fast-DDS/output/ros2_x86_64/setup.bash
                            for t in ${targets[@]}; do
                                echo "configure for ${t^^}"
                                mkdir -p ${workdir}/.build/${t}
                                cd ${workdir}/.build/${t}
                                build_ext=TRUE
                                src_dir=${workdir}/service/extension
                                if [[ ${t} = core ]]; then
                                    build_ext=FALSE
                                    src_dir=${workdir}/service/core
                                fi
                                cmake ${workdir} \
                                    -DBUILD_UNITTEST=FALSE \
                                    -DBUILD_EXTENSION=${build_ext} \
                                    -DEXTENSION_PLATFORM=${t^^} \
                                    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
                                if [ -f compile_commands.json ]; then
                                    mkdir -p ${workdir}/output/clang-tidy/${t}
                                    cp compile_commands.json ${workdir}/output/clang-tidy/${t}
                                    run-clang-tidy -p ${workdir}/output/clang-tidy/${t} \
                                        -j \$(nproc) \
                                        $(find ${src_dir} \
                                            -not -path "*/unittest/*" \
                                            -not -path "*/rapidjson/*" \
                                            -not -path "*/nlohmann/*" \
                                            \\( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cc" \\)) \
                                        > ${workdir}/output/reports/clang-tidy/clang-tidy-${t}.txt 2>&1 || true
                                fi
                            done
                            '
                        '''
                    }
                }
                script {
                    sh '''
                        bash -c '
                            set -e
                            workdir=/home/ossuser/workspace
                            report_dir=${WORKSPACE}/output/reports/clang-tidy
                            targets=(core ros2 agl)
                            for t in ${targets[@]}; do
                                if [[ -f ${report_dir}/clang-tidy-${t}.txt ]]; then
                                    sed -i "s#\${workdir}#\${WORKSPACE}#g" \
                                        ${report_dir}/clang-tidy-${t}.txt
                                fi
                            done
                        '
                    '''
                }
                script {
                    sh '''
                        workdir=/home/ossuser/workspace
                        if [ -d ${workdir}/clang-tidy ]; then
                            rm -rf ${workdir}/clang-tidy
                        fi
                        if [ -d ${workdir}/.build ]; then
                            rm -rf ${workdir}/.build
                        fi
                    '''
                }
            }
            post {
                always {
                    recordIssues(
                        tools: [
                            clangTidy(id: 'Core', name: 'Core', pattern: 'output/reports/clang-tidy/clang-tidy-core.txt'),
                            clangTidy(id: 'Ext-ros2', name: 'Extension for ROS2', pattern: 'output/reports/clang-tidy/clang-tidy-ros2.txt'),
                            clangTidy(id: 'Ext-agl', name: 'Extension for AGL', pattern: 'output/reports/clang-tidy/clang-tidy-agl.txt')
                        ],
                        aggregatingResults: true,
                        qualityGates: [[threshold: 1000, type: 'TOTAL', unstable: true]]
                    )
                    archiveArtifacts artifacts: 'output/reports/clang-tidy/clang-tidy-core.txt', allowEmptyArchive: true
                    archiveArtifacts artifacts: 'output/reports/clang-tidy/clang-tidy-ros2.txt', allowEmptyArchive: true
                    archiveArtifacts artifacts: 'output/reports/clang-tidy/clang-tidy-agl.txt', allowEmptyArchive: true
                }
            }
        }
    }
}

