#!/bin/bash

export PACKAGE_INSTALL_DIR=/opt


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

do_setup_environment