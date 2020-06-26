BUILD_TYPE = Debug

ifneq ($(shell test -d .git), 0)
GIT_SHORT_HASH:= $(shell git rev-parse --short HEAD)
endif

VERSION_MAJOR = 1
VERSION_MINOR = 3
VERSION_PATCH = 1

#hw/drivers/uwb/uwb_dw1000/src/dw1000_mac.c :
#    ln -s ../../../../decawave-uwb-dw1000/hw/drivers/uwb/uwb_dw1000 hw/drivers/uwb/uwb_dw1000

VERSION = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

COMMON_DEFINITIONS =                                      \
	-DCMAKE_BUILD_TYPE=$(BUILD_TYPE)                      \
	-DVERSION=$(VERSION)                                  \
	-DGIT_SHORT_HASH=$(GIT_SHORT_HASH)

generic:
	rm -R -f build_generic
	mkdir build_generic
	cd build_generic && cmake -G"Unix Makefiles"          \
	$(COMMON_DEFINITIONS)                                 \
	-DCMAKE_TOOLCHAIN_FILE=../toolchain/generic.cmake ..

cortex-m0:
	rm -R -f build_cortex-m0
	mkdir build_cortex-m0
	cd build_cortex-m0 && cmake -G"Unix Makefiles"       \
	$(COMMON_DEFINITIONS)                                \
	-DCMAKE_TOOLCHAIN_FILE=../toolchain/cortex-m0.cmake ..

cortex-m3:
	rm -R -f build_cortex-m3
	mkdir build_cortex-m3
	cd build_cortex-m3 && cmake -G"Unix Makefiles"       \
	$(COMMON_DEFINITIONS)                                \
	-DCMAKE_TOOLCHAIN_FILE=../toolchain/cortex-m3.cmake ..

cortex-m4:
	rm -R -f build_cortex-m4
	mkdir build_cortex-m4
	cd build_cortex-m4 && cmake -G"Unix Makefiles"       \
	$(COMMON_DEFINITIONS)                                \
	-DCMAKE_TOOLCHAIN_FILE=../toolchain/cortex-m4.cmake ..

cortex-m4f:
	rm -R -f build_cortex-m4f
	mkdir build_cortex-m4f
	cd build_cortex-m4f && cmake -G"Unix Makefiles"       \
	$(COMMON_DEFINITIONS)                                \
	-DCMAKE_TOOLCHAIN_FILE=../toolchain/cortex-m4f.cmake ..

cortex-m7:
	rm -R -f build_cortex-m7
	make build_cortex-m7
	cd build_cortex-m7 && cmake -G"Unix Makefiles"       \
	$(COMMON_DEFINITIONS)                                \
	-DCMAKE_TOOLCHAIN_FILE=../toolchain/cortex-m7.cmake ..

xtensa-lx106:
	rm -R -f build_xtensa-lx106
	mkdir build_xtensa-lx106
	cd build_xtensa-lx106 && cmake -G"Unix Makefiles"      	 \
	$(COMMON_DEFINITIONS)                                \
	-DCMAKE_TOOLCHAIN_FILE=../toolchain/xtensa-lx106.cmake ..

##### Cortex-A73
cortex-a73:
	rm -R -f build_cortex-a73
	mkdir build_cortex-a73
	cd build_cortex-a73 && cmake -G"Unix Makefiles"      	 \
	$(COMMON_DEFINITIONS)                                \
	-DCMAKE_TOOLCHAIN_FILE=../toolchain/cortex-a73.cmake ..

arm64:
	rm -R -f build_arm64
	mkdir -p build_arm64
	cd build_arm64 && cmake -G"Unix Makefiles"      	 \
	$(COMMON_DEFINITIONS)                                \
	-DCMAKE_TOOLCHAIN_FILE=../toolchain/cortex-a73.cmake ..

NDK_PATH = ../../../
cortex-a73-apk:
	rm -R -f build_cortex-a73-apk
	mkdir build_cortex-a73-apk
	cd build_cortex-a73-apk && cmake -G"Unix Makefiles"      	 \
	$(COMMON_DEFINITIONS)                                \
    -DCMAKE_TOOLCHAIN_FILE=$(NDK_PATH)/android-ndk-r20/build/cmake/android.toolchain.cmake \
	-DANDROID_NDK=$(NDK_PATH)/android-ndk-r20 -DCMAKE_BUILD_TYPE=Release \
	-DANDROID_NATIVE_API_LEVEL=android-21 -DANDROID_ABI="arm64-v8a" \
	-DANDROID_APK_BUILD=1 \
	-- VERBOSE=1 --no-print-directory ..

cortex-a73-aosp:
	rm -R -f build_cortex-a73-aosp
	mkdir build_cortex-a73-aosp
	cd build_cortex-a73-aosp && cmake -G"Unix Makefiles"      	 \
	$(COMMON_DEFINITIONS)                                \
    -DCMAKE_TOOLCHAIN_FILE=$(NDK_PATH)/android-ndk-r20/build/cmake/android.toolchain.cmake \
	-DANDROID_NDK=$(NDK_PATH)/android-ndk-r20 -DCMAKE_BUILD_TYPE=Release \
	-DANDROID_NATIVE_API_LEVEL=android-21 -DAOSP=1 -DANDROID_ABI="arm64-v8a" \
	-- VERBOSE=1 --no-print-directory ..

kernel-prereq:
	@$(info Building generic build in preparation for kernel modules)
	@mkdir -p build_generic
	@mkdir -p repos
	@$(info Setting up symlinks to drivers)
	rm -f repos/decawave-uwb-dw*
	ln -s ../../decawave-uwb-dw1000 repos/decawave-uwb-dw1000
	ln -s ../../decawave-uwb-dw3000-c0 repos/decawave-uwb-dw3000-c0
	rm -f hw/drivers/uwb/uwb_dw*
	ln -s ../../../../decawave-uwb-dw1000/hw/drivers/uwb/uwb_dw1000 hw/drivers/uwb/uwb_dw1000
	ln -s ../../../../decawave-uwb-dw3000-c0/hw/drivers/uwb/uwb_dw3000-c0 hw/drivers/uwb/uwb_dw3000-c0
	rm -f lib/cir/cir_dw*
	ln -s ../../../decawave-uwb-dw1000/lib/cir/cir_dw1000 lib/cir/cir_dw1000
	ln -s ../../../decawave-uwb-dw3000-c0/lib/cir/cir_dw3000-c0 lib/cir/cir_dw3000-c0
	cd build_generic && cmake -G"Unix Makefiles" $(COMMON_DEFINITIONS)  \
		-DCMAKE_TOOLCHAIN_FILE=../toolchain/generic.cmake ..
	make -C build_generic

cortex-a73-kernel: kernel-prereq
	@$(info Building kernel module for KERNEL_SRC='$(KERNEL_SRC)' (if empty, please supply))
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KERNEL_SRC) M=$(PWD) \
		CONFIG_UWB_DW1000=y CONFIG_UWB_DW3000=y CONFIG_UWB_CORE=m CONFIG_UWB_HAL=m modules
	@echo "\nKernel module(s) generated:"
	@find . -name "*.ko"

xcode:
	rm -R -f xcode
	mkdir xcode
	cd xcode && cmake -G"Xcode"       \
	$(COMMON_DEFINITIONS) ..

eclipse:
	rm -R -f build 
	mkdir build 
	cd build && cmake -G"Eclipse CDT4 - Unix Makefiles"       \
	$(COMMON_DEFINITIONS) ..

visual:
	rm -R -f visualstudio:wq 
	mkdir visualstudio 
	cd visualstudio && cmake -G”Visual Studio 10 Win64"       \
	$(COMMON_DEFINITIONS) ..

lib_only:
	rm -R -f build_lib_only
	mkdir build_lib_only
	cd build_lib_only && cmake $(COMMON_DEFINITIONS) -DLIB_ONLY=TRUE ..

all: generic cortex-m0 cortex-m3 cortex-m4 lib_only 

clean:
	rm -R -f bin
	rm -R -f targets
	rm -R -f include
	rm -R -f build_*
	rm -R -f ext_images
	rm -R -f xcode

unpack_images:
	rm -R -f ext_images
	7z x ext_images.7z
