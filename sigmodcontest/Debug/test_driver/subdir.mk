################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../test_driver/test.o 

CPP_SRCS += \
../test_driver/test.cpp 

OBJS += \
./test_driver/test.o 

CPP_DEPS += \
./test_driver/test.d 


# Each subdirectory must supply rules for building sources it contributes
test_driver/%.o: ../test_driver/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D__GXX_EXPERIMENTAL_CXX0X__ -I/usr/include/c++/4.7.2 -I/usr/include/c++/4.7.2/x86_64-linux-gnu -O3 -g3 -Wall -fexceptions -c -fmessage-length=0 -std=c++11 -lpthread -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@"  "$<"
	@echo 'Finished building: $<'
	@echo ' '


