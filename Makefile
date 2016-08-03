# $Id$

VID = 1.5
PRODUCT = 1

SUPPORTED_VERSIONS = 55 64 67

MOD_TARGETS = v473.out v473-dan.out
MOD_64_TARGETS = v473-cube.out
MOD_67_TARGETS = v473-cube.out

include ${PRODUCTS_INCDIR}frontend-2.4.mk

v473.o cube.o mooc_class.o test_v473.o : v473.h

v473.out : v473.o mooc_class.o ${PRODUCTS_LIBDIR}libvwpp-1.0.a
	${make-mod}

v473-cube.out : cube.o ${PRODUCTS_LIBDIR}libvwpp-1.0.a
	${make-mod}

v473-dan.out : test_v473.o ${PRODUCTS_LIBDIR}libvwpp-1.0.a
	${make-mod}
