#!/bin/sh
#
# Copyright (C) 2011, Oracle and/or its affiliates. All rights reserved.
#
# Incremental build
#

rm incmake.output.v2
mv incmake.output incmake.output.v2
make 1> incmake.output

