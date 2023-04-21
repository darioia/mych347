\rm -f CH347PAR_Demo myCH347PAR_Demo_hidapi myCH347_SPI_Demo_nohidapi myCH347_SPI_Demo_hiddev

CFLAG=" -g -Wall -I./include -I/usr/include/hidapi"
LDFLAG=" -lhidapi-hidraw -lch347  -L./lib"

gcc $CFLAG $LDFLAG ./src/CH347PAR_Demo.c           -o CH347PAR_Demo
gcc $CFLAG $LDFLAG ./src/myCH347PAR_Demo_hidapi.c  -o myCH347PAR_Demo_hidapi
gcc $CFLAG $LDFLAG ./src/myCH347_SPI_Demo_hiddev.c -o myCH347_SPI_Demo_hiddev
