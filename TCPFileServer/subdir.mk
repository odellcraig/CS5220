################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Client.cpp \
../Encryption.cpp \
../SendRecv.cpp \
../Server.cpp \
../Socket.cpp \
../Util.cpp \
../main.cpp 

OBJS += \
./Client.o \
./Encryption.o \
./SendRecv.o \
./Server.o \
./Socket.o \
./Util.o \
./main.o 

CPP_DEPS += \
./Client.d \
./Encryption.d \
./SendRecv.d \
./Server.d \
./Socket.d \
./Util.d \
./main.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


