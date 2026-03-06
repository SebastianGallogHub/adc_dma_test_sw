CC = gcc
CFLAGS = -Wall -O2

USB_LIBS = -lusb-1.0 -lpthread
SERIAL_LIBS = -lpthread

OBJDIR = build
OUTDIR = output

USB_TARGET = $(OUTDIR)/tar_usb_control
SERIAL_TARGET = $(OUTDIR)/tar_serial_control
BINCSV_TARGET = $(OUTDIR)/bin_to_csv

USB_SRC = tar_usb_control.c usb.c binToCSV.c
SERIAL_SRC = tar_serial_control.c serial_port.c binToCSV.c

USB_OBJ = $(OBJDIR)/tar_usb_control.o $(OBJDIR)/usb.o $(OBJDIR)/binToCSV.o
SERIAL_OBJ = $(OBJDIR)/tar_serial_control.o $(OBJDIR)/serial_port.o $(OBJDIR)/binToCSV.o

all: usb serial bincsv

usb: $(USB_OBJ)
	mkdir -p output
	$(CC) $(USB_OBJ) -o $(USB_TARGET) $(USB_LIBS)

serial: $(SERIAL_OBJ)
	mkdir -p output
	$(CC) $(SERIAL_OBJ) -o $(SERIAL_TARGET) $(SERIAL_LIBS)

bincsv:
	mkdir -p output
	$(CC) $(CFLAGS) -DBUILD_BINTOCSV_MAIN binToCSV.c -o $(BINCSV_TARGET)

$(OBJDIR)/%.o: %.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(USB_TARGET) $(SERIAL_TARGET)