#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <sys/param.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <mqueue.h>
#include <stdbool.h>
#include <fstream>
#include <sstream>
#include <iomanip>


#define HZ_1 (0) //1Hz of 10 Hz of operation
#define frames_count  1800 //Frames to be captured
#define HRES 640 //Horizontal Resolution
#define VRES 480 //Vertical Resolution
#define MSEC 1000000 
#define NSEC_PER_SEC (1000000000)
#define thread_cnt 4 //Total no of threads to be Implemented

#define TRUE (1)
#define FALSE (0)
#define ERROR (-1)

void *image_write(void *threadid); // Function to write the captured image to flash memory 
void *frame_function(void *threadid); //Function to capture the frame from the logitech camera
void *conv_jpg(void *threadid); //Function to convert the PPM image to jpg image
//void *add_timestamp(void *threadid); //Function to add timestamp to the frame
double calc_ms(void); //Function to calculate milliseconds
void jitter_calc(uint8_t thread_id); //Function to calculate jitter
void print_jitter(uint8_t thread_id); //Function to print jitter
void delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t); //Function to calculate time difference
void threads_init(void); //Function to initialze the threads
void check_time(void); //Function to check time 
void *sequencer(void *threadid); //Sequencer thread
