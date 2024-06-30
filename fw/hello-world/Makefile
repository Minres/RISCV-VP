
TARGET  = hello
C_SRCS  = $(wildcard *.c) 
HEADERS = $(wildcard *.h)
CFLAGS += -O0 -g 

BOARD=tgc_vp
LINK_TARGET=link
RISCV_ARCH:=rv32imc
RISCV_ABI:=ilp32
LDFLAGS := -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI)

compiler := $(shell which riscv32-unknown-elf-gcc)
TOOL_DIR=$(dir $(compiler))

BSP_BASE = ../bsp
include $(BSP_BASE)/env/common-gcc.mk
