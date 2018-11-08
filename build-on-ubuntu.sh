#!/bin/bash
sudo docker run --rm -v ${PWD}:/work -w /work kaelgodlike/ubuntu-trusty-qt59-aws-dev bash -c 'mkdir build \
&& cd build/ \
&& qmake .. \
&& make \
&& mkdir libs \
&& cp /usr/lib/x86_64-linux-gnu/libaws-cpp-sdk-s3.so libs/ \
&& cp /usr/lib/x86_64-linux-gnu/libaws-cpp-sdk-core.so  libs/ \
&& cp /opt/qt59/lib/libQt5Core.so.5 libs/ \
&& cp /opt/qt59/lib/libQt5Concurrent.so libs/ \
&& cp /usr/lib/x86_64-linux-gnu/libcrypto.so* libs/ \
&& cp /usr/lib/x86_64-linux-gnu/libicui18n.so* libs/ \
&& cp /usr/lib/x86_64-linux-gnu/libicuuc.so* libs/ \
&& cp /usr/lib/x86_64-linux-gnu/libicudata.so* libs/ \
&& tar czvf s3util_binary.tar.gz s3util libs \
&& mv s3util_binary.tar.gz /work
'
