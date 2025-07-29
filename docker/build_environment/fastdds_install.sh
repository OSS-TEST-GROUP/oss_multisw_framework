#!/bin/bash

export PACKAGE_INSTALL_DIR=/opt

do_install_fastdds ()
{
    # Activate the FastDDS environment
    mkdir -p ${PACKAGE_INSTALL_DIR}/Fast-DDS
    cd ${PACKAGE_INSTALL_DIR}/Fast-DDS
    wget https://raw.githubusercontent.com/eProsima/Fast-DDS/3.2.2/fastdds.repos  && mkdir src && \
    vcs import src < fastdds.repos && colcon build --packages-up-to fastdds && source ${PACKAGE_INSTALL_DIR}/Fast-DDS/install/setup.bash && \
    cd ${PACKAGE_INSTALL_DIR} && git clone https://github.com/eProsima/Fast-DDS-Gen.git && cd Fast-DDS-Gen && \
    chmod +x gradlew && ./gradlew clean && ./gradlew build ; ./gradlew install

}

do_setup_fastdds ()
{
    # Activate the FastDDS environment
    . ${PACKAGE_INSTALL_DIR}/Fast-DDS/install/setup.bash
}

do_setup_environment ()
{
    echo "Setting up the build envirionment for MW Framework."

    do_setup_fastdds
}

do_install_fastdds
do_setup_environment