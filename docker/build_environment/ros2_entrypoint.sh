#!/bin/bash - 

export LD_LIBRARY_PATH=/oss/lib
export SAVED_REGISTRY_BACKUP=/oss/bin/config

/oss/bin/extension_communication &

/oss/bin/ota_client &

/oss/bin/security_client &

exec /oss/bin/adsw_manager

