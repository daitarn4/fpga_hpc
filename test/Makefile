# variables
SRC := "./src"
DST := "./bin"
TMP := "./tmp"
TARGET_BOARD := "p520_hpc_sg280l" 
TARGET_KERNEL := "kernel"
TARGET_HOST := "host"
OPENCL_LIB := "/opt/intel/intelFPGA_pro/18.1/hld/host/include/" 
KNAME ?=

# actions
# run application
run: 
	@cd bin/ && ./host $(KNAME)

# compile all the kernel(s) and the host code
all: kernels host

# full compilation of the FPGA kernel 
kernels:
	@echo "[inf] compiling the FPGA kernel(s) <"$(TARGET_KERNEL)".cl>"
	@echo "[inf] target fpga accelerator is " $(TARGET_BOARD)
	@cd $(TMP) &&\
	aoc ../$(SRC)/$(TARGET_KERNEL).cl -o ../$(TMP)/$(TARGET_KERNEL).aocx --board $(TARGET_BOARD) --report
	@echo "[inf] moving output files to the destination path"
	@mv $(TMP)/$(TARGET_KERNEL).aocx $(DST)/$(TARGET_KERNEL).aocx
	@mv $(TMP)/$(TARGET_KERNEL).aoco $(DST)/$(TARGET_KERNEL).aoco
	@echo "[inf] action completed"

# compile the host code
host: 
	@echo "[inf] compiling the host code <"$(TARGET_HOST)".c>" 
	@gcc $(SRC)/$(TARGET_HOST).c -o $(DST)/$(TARGET_HOST) -I$(OPENCL_LIB) -lOpenCL
