# Supersonic

Copyright 2012 Google Inc. All Rights Reserved.

## Build

1. Clone this repo and dependencies.

    ```Bash
    git clone https://github.com/davidli2010/supersonic.git
    git clone https://github.com/davidli2010/supersonic_depends.git
    cd supersonic
    git submodule update --init --recursive
    ```

2. Build dependencies.

    Unzip all the dependencies and build one by one.
    
    Assume we install all the dependencies in `/home/<user>/supersonic-deps`.
    
    - boost
    
        ```Bash
        ./bootstrap.sh
        ./b2
        ./b2 install --prefix=/home/<user>/supersonic-deps
        ```

    - gflags
    
        ```Bash
        ./configure --prefix=/home/<user>/supersonic-deps
        make install -j 4
        ```
      
    - glog
        
        ```Bash
        ./configure --prefix=/home/<user>/supersonic-deps
        make install -j 4
        ```
      
    - protobuf
    
        ```Bash
        ./configure --prefix=/home/<user>/supersonic-deps
        make install -j 4
        ```
      
    - re2
    
        Modify the `Makefile`:
        
        ```Bash
        prefix=/home/<user>/supersonic-deps
        ```
        
        then build it:
        
        ```Bash
        make install -j 4
        ```

3. Build supersonic:

    ```Bash
    cd supersonic
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/home/<user>/supersonic/build/install -DCMAKE_INCLUDE_PATH=/home/<user>/supersonic-deps/include -DCMAKE_LIBRARY_PATH=/home/<user>/supersonic-deps/lib -DProtobuf_PROTOC_EXECUTABLE=/home/<user>/supersonic-deps/bin/protoc
    make install -j 4
    ```
