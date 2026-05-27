#!/bin/bash - 

export LD_LIBRARY_PATH=/oss/lib
export ADSW_CONFIG=${ADSW_CONFIG:-/oss/bin/config/agl-service-list.yaml}
export OTA_CLIENT_CONFIG=${OTA_CLIENT_CONFIG:-/oss/bin/config/ota_client_config.json}

/oss/bin/agl_ota_client &

exec /oss/bin/agl_adsw_manager
