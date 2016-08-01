keeperrl
========

Source code of KeeperRL

Compiling
=========

*Prerequisites*

  * make essentials
  * gcc-4.8.2 OR clang-3.3
  * git
  * libboost 1.56+ with libboost-serialization, libboost-program-options, libboost-filesystem
  * For compiling on OSX you'll also need libboost-system, libboost-thread and libboost-chrono
  * libsdl2-dev, libsdl2-image-dev
  * libopenal-dev
  * libvorbis-dev

Under Ubuntu 14.4 , you could use these to create development enviroment 
```
sudo apt-get install libboost1.56-all-dev libsdl2-dev libsdl2-image-dev libopenal-dev libvorbis-dev llvm-3.4 build-essential

```


In terminal:  
  ```
  git clone https://github.com/miki151/keeperrl.git
  cd keeperrl
  make -j 8 # for slow & debug mode or
  make -j 8 OPT=true RELEASE=true # for release
  # add CLANG=true to compile with clang.
  # add OSX=true to compile on OSX
  ./keeper
  ```

Docker Support
=======
A `Dockerfile` is provided for easy installation of the dependencies

```
  git clone https://github.com/miki151/keeperrl.git
  cd keeperrl
  docker build -t keeperrl ./
```

##Docker Run with GUI Handling
Docker containers are usually headless You can run GUI apps like KeeperRL by passing
environment information about your host machines window system to the container. 
Then when the container is run it will do all the rendering on your host machine.

see: https://blog.jessfraz.com/post/docker-containers-on-the-desktop/ for more info and examples

### Ubuntu 
```
docker run \
       -v /tmp/.X11-unix:/tmp/.X11-unix \
       -e DISPLAY=unix$DISPLAY \
       --device /dev/snd:/dev/snd \
       keeperrl
```
### OSX and boot2docker
OSX Docker Container GUI support is more complicated. boot2docker runs the docker environment in a VM
You need to run socat and host an Xquartz server on your host machine. You can then run the docker container
and have it pipe the X GUI to the OSX host.

see: http://haven.nightlyart.com/trying-gui-apps-with-docker/

```
brew install socat
brew cask install xquartz
open -a XQuartz

##### Run socat
socat TCP-LISTEN:6000,reuseaddr,fork UNIX-CLIENT:\"$DISPLAY\"
```
In a seperate terminal window

```
docker run -e DISPLAY=192.168.59.3:0 keeperrl
```

