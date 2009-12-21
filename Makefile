# $Id$

VID = 1.0
PRODUCT = 1

SUPPORTED_VERSIONS = 64

MOD_TARGETS = v473.out

include ${PRODUCTS_INCDIR}frontend-2.3.mk

v473.o cube.o mooc_class.o : v473.h

v473.out : v473.o cube.o mooc_class.o ${PRODUCTS_LIBDIR}libvwpp-0.1.a
	${make-mod}
