FROM ubuntu:trusty
ENV DEBIAN_FRONTEND=noninteractive
ENV TERM=linux

RUN echo "binfmt_misc /proc/sys/binfmt_misc defaults 0 0" >> $ROOTFS/etc/fstab

RUN sudo apt-get update -y
RUN  sudo apt-get install -y --no-install-recommends \
build-essential g++ python-dev autotools-dev libicu-dev libbz2-dev wget git-core \
libsfml-dev freeglut3-dev libglew-dev llvm-3.4 ca-certificates zlib1g-dev \
clang curl libcurl4-openssl-dev
 
RUN curl -sf -o boost_1_56_0.tar.gz -L http://superb-dca2.dl.sourceforge.net/project/boost/boost/1.56.0/boost_1_56_0.tar.gz
RUN tar xzvf boost_1_56_0.tar.gz

WORKDIR /boost_1_56_0
RUN ./bootstrap.sh --prefix=/usr 
RUN ./b2 install --with-serialization --with-program_options --with-system --with-filesystem

ADD . /keeperrl
WORKDIR /keeperrl

RUN make -j 8 OPT=true RELEASE=true CLANG=true

ENTRYPOINT /keeperrl/keeper