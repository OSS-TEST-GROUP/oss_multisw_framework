#!/bin/bash - 

export LD_LIBRARY_PATH=/oss/lib
export SAVED_REGISTRY_BACKUP=/oss/bin/config

exec /oss/bin/adsw_manager

