#!/bin/bash

# -------------------------------------------------------
# this is the directory we plan on building everything to
# -------------------------------------------------------
BUILD_DIR=audio-build
cd "../$BUILD_DIR"

# -------------------------------------------------------
# now, build
# -------------------------------------------------------
make
echo ""
