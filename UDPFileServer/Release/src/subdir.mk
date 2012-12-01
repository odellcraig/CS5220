################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ARQBase.cpp \
../src/Client.cpp \
../src/SelectiveRepeat.cpp \
../src/Server.cpp \
../src/StopAndWait.cpp \
../src/UDPFileServer.cpp \
../src/UDPSocket.cpp \
../src/Util.cpp 

OBJS += \
./src/ARQBase.o \
./src/Client.o \
./src/SelectiveRepeat.o \
./src/Server.o \
./src/StopAndWait.o \
./src/UDPFileServer.o \
./src/UDPSocket.o \
./src/Util.o 

CPP_DEPS += \
./src/ARQBase.d \
./src/Client.d \
./src/SelectiveRepeat.d \
./src/Server.d \
./src/StopAndWait.d \
./src/UDPFileServer.d \
./src/UDPSocket.d \
./src/Util.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


