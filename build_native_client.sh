#!/bin/bash

# How to use this script:
# 1. Adjust the variables in the following section.
# 2. Run "./build_native_client.sh" to make a clean PNaCl build of UAE.
# 3. Now edit, run "make", and run "./build_native_client.sh run" repeatedly.

###########################################################################
# Set up. Adjust these as necessary.
###########################################################################

# Path to the NaCl SDK to use. Should be pepper_31 or later.
export NACL_SDK_ROOT=${HOME}/work/nacl_sdk/pepper_31

# Check out naclports and point this to the folder with 'src/'.
export NACL_PORTS_ROOT=${HOME}/work/naclports

# Staging dir on local web server.
export WEB_SERVER_DESTINATION_DIR=${HOME}/work/puae/staging

# URL to access the staging dir via the web server.
export WEB_SERVER="http://localhost:8080"

# The OS you're building this on.
export OS=mac # Should be mac, linux, or win.

# Set the Chrome/Chromium to use.
export CHROME_EXE='/Applications/Google Chrome Canary.app/Contents/MacOS/Google Chrome Canary'

# Any extra CPPFLAGS.
export NACL_CPPFLAGS="-DDEBUG"

# Choose SDL or Pepper for graphics, sound, and input.
# TODO(cstefansen): Fix and check in SDL build.
#export UAE_CONFIGURE_FLAGS_NACL="--enable-drvsnd \
#    --with-sdl --with-sdl-gfx --with-sdl-gl --with-sdl-sound"
export UAE_CONFIGURE_FLAGS_NACL="--enable-drvsnd --with-pepper"


###########################################################################
# You shouldn't need to change anything below this point.
###########################################################################

export UAE_ROOT=`cd \`dirname $0\` && pwd`


############################################################################
# Do a clean build for PNaCl.
############################################################################

if [ "$1" = "pnacl" ] || [ -z "$1" ]; then
    export NACL_TOOLCHAIN_ROOT=${NACL_SDK_ROOT}/toolchain/${OS}_pnacl
    export CC=${NACL_TOOLCHAIN_ROOT}/bin/pnacl-clang++
    export CXX=$CC
    export RANLIB=${NACL_TOOLCHAIN_ROOT}/bin/pnacl-ranlib
    export AR=${NACL_TOOLCHAIN_ROOT}/bin/pnacl-ar
    export NACL_INCLUDE=${NACL_SDK_ROOT}/include
    export NACL_LIB=${NACL_SDK_ROOT}/lib/pnacl/Release

    export CPPFLAGS="-I${NACL_INCLUDE} ${NACL_CPPFLAGS}"
    export CFLAGS="-O2 -g -Wno-unused-parameter -Wno-missing-field-initializers"
    export CXXFLAGS="-O2 -g -Wno-unused-parameter -Wno-missing-field-initializers"
    export LDFLAGS="-L${NACL_LIB}"

    # Build zlib.
    cd ${NACL_PORTS_ROOT}/src/libraries/zlib
    # TODO(cstefansen): Had to comment out test stage in this script:
    NACL_ARCH=pnacl NACL_GLIBC=0 ./nacl-zlib.sh

    # Prepare PUAE build
    cd ${UAE_ROOT}
    make clean
    ./bootstrap.sh
    ./configure --host=le32-unknown-nacl ${UAE_CONFIGURE_FLAGS_NACL}
    make
fi # "pnacl"


###########################################################################
# Incremental
###########################################################################

if [ "$1" = "incremental" ]; then
    make
fi


############################################################################
# Build for desktop.
# This is left in for convenience to aid debugging/profiling.
############################################################################

if [ "$1" = "desktop" ]; then
    CPPFLAGS="-m32 -g"
    LDFLAGS="-m32 -g"
    cd ${UAE_ROOT}
    make clean
    ./bootstrap.sh
    ./configure --disable-ui --disable-jit \
        --with-sdl --with-sdl-gfx --without-sdl-gl --with-sdl-sound \
        --disable-autoconfig
    if [ "$?" -ne "0" ]; then
        echo "./configure failed for desktop UAE."
        exit 1
    fi
    make
    if [ "$?" -ne "0" ]; then
        echo "make failed for desktop UAE."
        exit 1
    fi
fi # "desktop"


###########################################################################
# Running/debugging
###########################################################################

if [ "$1" = "run" ] || [ "$1" = "debug" ]; then
    # TODO(cstefansen): Put the pnacl-finalize step in a make rule instead.
    ${NACL_SDK_ROOT}/toolchain/${OS}_pnacl/bin/pnacl-finalize src/uae \
        -o ${WEB_SERVER_DESTINATION_DIR}/uae.pexe

    # Copy stuff to web server.
    echo "Copying to UAE and roms to web server directory."
    cp src/gui-html/uae.nmf ${WEB_SERVER_DESTINATION_DIR}/uae.nmf

    cp src/gui-html/img/amiga500_128x128.png ${WEB_SERVER_DESTINATION_DIR}/
    cp src/gui-html/img/boingball_16x16.png ${WEB_SERVER_DESTINATION_DIR}/
    cp src/gui-html/img/loading.gif ${WEB_SERVER_DESTINATION_DIR}/
    cp src/gui-html/uae.html src/gui-html/uae.js \
        ${WEB_SERVER_DESTINATION_DIR}/
    cp src/gui-html/default.uaerc ${WEB_SERVER_DESTINATION_DIR}/

    # Make sure the files are readable.
    chmod -R 0755 ${WEB_SERVER_DESTINATION_DIR}

    # Have Chrome navigate to the app.
    CHROME_APP_LOCATION="$WEB_SERVER/uae.html"

    # Chrome flags.
    # Add the flag --disable-gpu-vsync when measuring performance.
    # Add the flag --disable-gpu to simulate system without OpenGLES support
    CHROME_FLAGS='--user-data-dir=../chrome-profile \
         --no-first-run --show-fps-counter'
#    	--disable-gpu --enable-nacl --incognito 

    # Clean the user profile.
#    rm -fR ../chrome-profile

    if [ "$1" = "debug" ]; then
        CHROME_FLAGS+=' --enable-nacl-debug'
        echo "*** Starting Chrome in NaCl debug mode. ***"
        echo "Start nacl-gdb and connect with 'target remote localhost 4010'."
    else
        ${NACL_SDK_ROOT}/toolchain/${OS}_pnacl/bin/pnacl-strip \
            ${WEB_SERVER_DESTINATION_DIR}/uae.pexe
    fi
    echo "${CHROME_EXE}" ${CHROME_FLAGS} ${CHROME_APP_LOCATION}
    "${CHROME_EXE}" ${CHROME_FLAGS} ${CHROME_APP_LOCATION}
fi # "run" || "debug"
