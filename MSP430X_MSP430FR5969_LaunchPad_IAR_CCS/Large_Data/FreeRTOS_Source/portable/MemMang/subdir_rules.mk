################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
FreeRTOS_Source/portable/MemMang/heap_4.obj: /Users/moteenshah/Downloads/FreeRTOSv202212.01/FreeRTOS/Source/portable/MemMang/heap_4.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccs1250/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --abi=eabi --data_model=large -O0 --use_hw_mpy=F5 --include_path="/Applications/ti/ccs1250/ccs/ccs_base/msp430/include" --include_path="/Users/moteenshah/Downloads/FreeRTOSv202212.01/FreeRTOS/Demo/MSP430X_MSP430FR5969_LaunchPad_IAR_CCS" --include_path="/Users/moteenshah/Downloads/FreeRTOSv202212.01/FreeRTOS/Source/include" --include_path="/Users/moteenshah/Downloads/FreeRTOSv202212.01/FreeRTOS/Source/portable/CCS/MSP430X" --include_path="/Users/moteenshah/Downloads/FreeRTOSv202212.01/FreeRTOS-Plus/Source/FreeRTOS-Plus-CLI" --include_path="/Users/moteenshah/Downloads/FreeRTOSv202212.01/FreeRTOS/Demo/Common/include" --include_path="/Applications/ti/ccs1250/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --include_path="/Users/moteenshah/Downloads/FreeRTOSv202212.01/FreeRTOS/Demo/MSP430X_MSP430FR5969_LaunchPad_IAR_CCS/driverlib/MSP430FR5xx_6xx" -g --c89 --relaxed_ansi --define=__MSP430FR5969__ --display_error_number --diag_warning=225 --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="FreeRTOS_Source/portable/MemMang/$(basename $(<F)).d_raw" --obj_directory="FreeRTOS_Source/portable/MemMang" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


