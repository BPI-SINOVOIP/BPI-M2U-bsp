CC:= arm-none-linux-gnueabi-gcc
LINUX_DIR = $(shell pwd)/../../../../../include/:$(shell pwd)/../../../../../arch/arm/include
ARCH_DIR=../../../../../arch/arm/include/
CFLAGS := -I $(LINUX_DIR)
TARGET := csi_test

.PHONY: all clean

all: $(TARGET) 

csi_test: csi_test.c 
	$(CC) $(CFLAGS) -static  $^  -o  $@

clean:
	rm -rf $(TARGET)  
