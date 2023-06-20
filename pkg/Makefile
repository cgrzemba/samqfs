# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at pkg/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END

DESTDIR := $(if $(DESTDIR),$(DESTDIR),./root_i386)

FLAVOR.DBG = --debug_build
FLAVOR = $(FLAVOR.$(DEBUG:yes=DBG))

INSTALL.64 = --64bit
INSTALL_ISA = $(INSTALL.$(AMD64:yes=64))

OS_RELEASE := $(shell  uname -v | gawk '{ split($$0,a,"[.-]"); print a[1]"-"a[2]}')
REPO ?= ../repo/i386-$(OS_RELEASE)

install:
	./install-sam.py --destdir=$(DESTDIR) --version=$(SAMQFS_VERSION) $(FLAVOR) $(INSTALL_ISA)

pkg:
	./install-sam.py --destdir=$(DESTDIR) --version=$(SAMQFS_VERSION) $(FLAVOR) $(INSTALL_ISA) --publish --repro=$(REPO)

.PHONY: clean
clean:
	rm -rf $(DESTDIR)/*
	rm *.cache
