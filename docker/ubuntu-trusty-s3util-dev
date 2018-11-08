FROM ubuntu:trusty
ENV PATH="/opt/qt59/bin:${PATH}"
RUN apt-get update \
    && apt-get install software-properties-common python-software-properties -y \
    && sudo add-apt-repository --yes ppa:beineri/opt-qt594-trusty \
    && sudo add-apt-repository --yes ppa:george-edison55/cmake-3.x \
    && apt-get update \
    && apt-get remove cmake cmake-data -y \
    && apt-get install git qt59base build-essential libcurl4-openssl-dev libssl-dev uuid-dev zlib1g-dev libpulse-dev cmake -y \
    && mkdir aws_build && cd aws_build && git clone --depth=1 https://github.com/aws/aws-sdk-cpp.git && cd aws-sdk-cpp && sed -i '/ENABLE_TESTING/s/ON/OFF/g' CMakeLists.txt && mkdir build && cd build && cmake -DBUILD_ONLY="s3" -DCMAKE_INSTALL_PREFIX="/usr" .. && make && sudo make install
