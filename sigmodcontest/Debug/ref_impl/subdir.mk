################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../ref_impl/core.o 

CPP_SRCS += \
../ref_impl/core.cpp 

OBJS += \
./ref_impl/core.o 

CPP_DEPS += \
./ref_impl/core.d 


# Each subdirectory must supply rules for building sources it contributes
ref_impl/core.o: ../ref_impl/core.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	gcc -D__GXX_EXPERIMENTAL_CXX0X__ -I/usr/include/c++/4.7.2 -I/usr/include/c++/4.7.2/x86_64-linux-gnu -O3 -g3 -Wall -fexceptions -c -fmessage-length=0 -std=c++11 -lpthread -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"ref_impl/core.d" -o "$@"  "$<"
	@echo 'Finished building: $<'
	@echo ' '


