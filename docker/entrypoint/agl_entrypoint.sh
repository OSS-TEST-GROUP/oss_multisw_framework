#!/bin/bash - 

export LD_LIBRARY_PATH=/oss/lib

/oss/bin/agl_ota_client &

exec /oss/bin/agl_adsw_manager
