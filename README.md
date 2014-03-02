keeperrl
========

Source code of KeeperRL

Compiling
=========

-Prerequisites

  * make essentials
  * git
  * libsfml-dev 2+ (Ubuntu ppa that contains libsfml 2: http://www.playdeb.net/updates/Ubuntu/13.10#how_to_install )
  * freeglut-dev
  * libglew-dev (if you're still getting undefined symbols about GLEW, try compiling SFML from source)



In terminal:  
  ```
  git clone https://github.com/miki151/keeperrl.git
  cd keeperrl
  make -j 8 # for slow & debug mode or
  make -j 8 OPT=true # for release
  ./keeper
  ```
