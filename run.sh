PORT_NUM=2860
UDP_DIR=./UDPFileServer/Release
TCP_DIR=./TCPFileServer/Release
LOG_DIR=./logs
LOSS_PCT=5


# Build both binaries
echo "Building both TCP and UDP File Servers..."
cd $TCP_DIR && make
cd ../../
cd $UDP_DIR && make
cd ../../



# Test 1 - TCP
echo
echo "Testing TCP file server."
cd $TCP_DIR && ./TCPFileServer -s -p $PORT_NUM 2>&1 | tee -a ../../tcpTest/tcpServer.log &
cd tcpTest && ../TCPFileServer/Release/TCPFileServer -c -p $PORT_NUM localhost file.pdf 2>&1 | tee -a tcpClient.log
md5sum ../TCPFileServer/Release/file.pdf >MD5SUMS
md5sum ./file.pdf >> MD5SUMS
md5sum -c --status MD5SUMS && (echo "file.pdf MD5 verified"; echo "Test Passed") || (echo "file.pdf MD5 incorrect!"; echo "Test Failed!")
cd ../
killall TCPFileServer
echo





echo
echo "Testing UDP file server with Selective Repeat ARQ."
rm -f udpTest/*SelectiveRepeat*; rm -f udpTest/file.pdf
cd $UDP_DIR && ./UDPFileServer -s -p $PORT_NUM -d $LOSS_PCT -t 3 -o ../../udpTest/udpServerSelectiveRepeatTrace.txt 2>&1 | tee -a ../../udpTest/udpServerSelectiveRepeat.log &
cd udpTest && ../UDPFileServer/Release/UDPFileServer -c -p $PORT_NUM -d $LOSS_PCT -t 3 -o udpClientSelectiveRepeatTrace.txt localhost file.pdf 2>&1 | tee -a udpClientSelectiveRepeat.log
md5sum ../UDPFileServer/Release/file.pdf >MD5SUMS
md5sum ./file.pdf >> MD5SUMS
md5sum -c --status MD5SUMS && (echo "file.pdf MD5 verified"; echo "Test Passed") || (echo "file.pdf MD5 incorrect!"; echo "Test Failed!")
cd ../
killall UDPFileServer
echo


exit
echo
echo "Testing UDP file server with Stop and Wait ARQ."
rm -f udpTest/*StopAndWait*; rm -f udpTest/file.pdf
cd $UDP_DIR && ./UDPFileServer -s -p $PORT_NUM -d $LOSS_PCT -t 1 -o ../../udpTest/udpServerStopAndWaitTrace.txt 2>&1 | tee -a ../../udpTest/udpServerStopAndWait.log &
cd udpTest && ../UDPFileServer/Release/UDPFileServer -c -p $PORT_NUM -d $LOSS_PCT -t 1 -o udpClientStopAndWaitTrace.txt localhost file.pdf 2>&1 | tee -a udpClientStopAndWait.log
md5sum ../UDPFileServer/Release/file.pdf >MD5SUMS
md5sum ./file.pdf >> MD5SUMS
md5sum -c --status MD5SUMS && (echo "file.pdf MD5 verified"; echo "Test Passed") || (echo "file.pdf MD5 incorrect!"; echo "Test Failed!")
cd ../
killall UDPFileServer
echo
