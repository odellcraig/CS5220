################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Client.cpp \
../src/Encryption.cpp \
../src/SendRecv.cpp \
../src/Server.cpp \
../src/Socket.cpp \
../src/Util.cpp \
../src/main.cpp 

OBJS += \
./src/Client.o \
./src/Encryption.o \
./src/SendRecv.o \
./src/Server.o \
./src/Socket.o \
./src/Util.o \
./src/main.o 

CPP_DEPS += \
./src/Client.d \
./src/Encryption.d \
./src/SendRecv.d \
./src/Server.d \
./src/Socket.d \
./src/Util.d \
./src/main.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


