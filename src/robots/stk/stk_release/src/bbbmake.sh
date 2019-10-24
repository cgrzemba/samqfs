#!/bin/sh
#
# Copyright (C) 2011, Oracle and/or its affiliates. All rights reserved.
#
# Big Bang Build
#

rm bbbmake.output.v2
mv bbbmake.output bbbmake.output.v2
make sterile 1> bbbmake.output 2> bbbmake.output
make 1>> bbbmake.output

