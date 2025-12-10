docker run --privileged \
    --rm -it -u 1000:1000 \
    -w /home/ossuser/workspace \
    -v ${PWD}/output/x86_64:/home/ossuser/workspace/output \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v /run/user/1000:/run/user/1000 \
    -v ${HOME}/.ssh:/root/.ssh \
    -e DISPLAY=unix:1 \
    -e LD_LIBRARY_PATH=/home/ossuser/workspace/output/lib \
    --network=host --cap-add=NET_RAW --cap-add=NET_ADMIN \
    registry.obigo.com:80/oss/oss-mmf-builder:latest
    /bin/bash

#docker exec -it cfb5993afc84 /bin/bash
