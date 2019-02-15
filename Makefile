CC=gcc
export SHAREDFLAGS=-O3
CFLAGS=-I./rtk2/ `pkg-config gtk+-2.0 glib --cflags` -Wall -g `xml2-config --cflags`  $(SHAREDFLAGS)
LDFLAGS=-L./rtk2 -lrtk -ljpeg -lm `pkg-config gtk+-2.0 glib --libs` `xml2-config --libs` $(SHAREDFLAGS)

all: lrt lbt parse_log_file laser_scan_panel

laser_scan_panel : laser_scan_panel.o log_configuration.o line_based_read.o main_wnd.o librtk.a background_img.h utils.o wnd_annos.o wnd_logs.o laser_read.o interval.o wnd_prog.o tracking_mod.o temp_corr_calc.o clustering.o tracking.o
	$(CC) -o laser_scan_panel laser_scan_panel.o log_configuration.o line_based_read.o main_wnd.o utils.o wnd_annos.o wnd_logs.o laser_read.o interval.o wnd_prog.o tracking_mod.o temp_corr_calc.o clustering.o tracking.o $(LDFLAGS)

librtk.a:
	make -C rtk2 
	cp rtk2/librtk.a .

parse_log_file : parse_log_file.o line_based_read.o librtk.a 
	$(CC) -o parse_log_file parse_log_file.o line_based_read.o $(LDFLAGS)

lbt : line_based_test.o line_based_read.o  librtk.a 
	$(CC) -o lbt line_based_test.o line_based_read.o $(LDFLAGS)

lrt : laser_reader_test.o laser_read.o line_based_read.o  librtk.a 
	$(CC) -o lrt laser_reader_test.o laser_read.o line_based_read.o $(LDFLAGS)

it : interval.o it.o
	$(CC) -o it interval.o it.o

line_based_test.o :  line_based_read.o line_based_test.c
	$(CC) $(CFLAGS) -c line_based_test.c

line_based_read.o : line_based_read.h line_based_read.c
	$(CC) $(CFLAGS) -c line_based_read.c

laser_read.o : laser_read.h laser_read.c line_based_read.h
	$(CC) $(CFLAGS) -c laser_read.c

parse_log_file.o : parse_log_file.h parse_log_file.c line_based_read.h
	$(CC) $(CFLAGS) -c parse_log_file.c

laser_scan_panel.o : laser_scan_panel.c  log_configuration.h main_wnd.h
	$(CC) $(CFLAGS) -c laser_scan_panel.c 

log_configuration.o : log_configuration.c
	$(CC) $(CFLAGS) -c log_configuration.c

main_wnd.o : main_wnd.c main_wnd.h wnd_annos.h log_configuration.h tracking.h
	$(CC) $(CFLAGS) -c main_wnd.c

wnd_annos.o : wnd_annos.c 
	$(CC) $(CFLAGS) -c wnd_annos.c

wnd_logs.o : wnd_logs.c 
	$(CC) $(CFLAGS) -c wnd_logs.c

wnd_prog.o : wnd_prog.c 
	$(CC) $(CFLAGS) -c wnd_prog.c

utils.o : utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

it.o : it.c
	$(CC) $(CFLAGS) -c it.c

interval.o : interval.c interval.h
	$(CC) $(CFLAGS) -c interval.c

tracking_mod.o : tracking_mod.h  tracking_mod.c tracking_common.h
	$(CC) $(CFLAGS) -c tracking_mod.c

temp_corr_calc.o : temp_corr_calc.h temp_corr_calc.c tracking_common.h
	$(CC) $(CFLAGS) -c temp_corr_calc.c

tracking.o : tracking.h tracking.c tracking_common.h
	$(CC) $(CFLAGS) -c tracking.c

clustering.o : clustering.h clustering.c tracking_common.h
	$(CC) $(CFLAGS) -c clustering.c

clean :
	make -C rtk2 clean
	rm -f librtk.a 
	rm -f *.o

