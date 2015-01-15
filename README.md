keeperrl
========

Source code of KeeperRL

Compiling
=========

*Prerequisites*

  * make essentials
  * gcc-4.8.2 OR clang-3.3
  * git
  * libboost 1.56+ with libboost-serialization, libboost-program-optoins
  * For compiling on OSX you'll also need libboost-system and libboost-thread
  * libsfml-dev 2+ (Ubuntu ppa that contains libsfml 2: http://www.playdeb.net/updates/Ubuntu/13.10#how_to_install )
  * freeglut-dev
  * libglew-dev (if you're still getting undefined symbols about GLEW, try compiling SFML from source)
  * angelscript (http://www.angelcode.com/angelscript/)


Under Ubuntu 14.4 , you could use these to create development enviroment 
```
sudo apt-get install libboost1.56-all-dev libsfml-dev freeglut3-dev libglew-dev llvm-3.4 build-essential

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
