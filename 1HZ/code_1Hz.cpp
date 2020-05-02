
#include "code_1Hz.h"
#include <sys/utsname.h>
#include <syslog.h>

using namespace cv;
using namespace std;

VideoCapture cap(0);

Mat ppm_frame(480,640,CV_8UC3);
uint8_t *frame_ptr;
Mat frame_jpg(480,640,CV_8UC3);


int abortTest = FALSE;

int dev = 0;
double framerate;
double val;
int capture_cnt= 0;
double time_diff=0;
time_t r_time;
struct tm * cur_t;

double start_array[thread_cnt] = {0,0,0}; //Array to store start time for each thread
double stop_array[thread_cnt] = {0,0,0}; //Array to store the stop time for each thread
double acc_jitter_array[thread_cnt] = {0,0,0}; //Array to store the Accumulated jitter for each thread
double avg_jitter_array[thread_cnt] = {0,0,0};//Array to store the average jitter for each thread
double wcet_array[thread_cnt] = {0,0,0};// Array to store the Worst Case Execution Time for each thread
double avg_diff_array[thread_cnt] = {0,0,0}; // Array to store time difference for each thread.

uint32_t counter_array[thread_cnt] = {0,0,0};
double jitter_calc_array[thread_cnt] = {0,0,0}; //Array to store jitter calculation
double run_time[thread_cnt] = {0,0,0}; //Array to store run time for each thread


sem_t sem_array[thread_cnt]; //Array to store semaphores for each thread
pthread_t thread_array[thread_cnt]; 
pthread_attr_t att_array[thread_cnt];
struct sched_param par_array[thread_cnt]; //Array to store scheduling parameters for each thread
void* (*func_arr[thread_cnt]) (void*) ;

static struct timespec start_t, stop_t, exec_t, current_t;
static uint32_t timer_ctr = 0, start_ctr=0;
static uint8_t cond = TRUE;
static struct timespec cap_strt_t = {0,0};
static struct timespec cap_stp_t = {0,0};
static struct mq_attr frame_mq_attr;
double initial_time;
sem_t ppm_sem, jpg_sem, jpg_fin_sem; //Semaphores for each services s
void display(int disp, uint8_t w, uint8_t* store);

/*****************************************
*Function name : void *image_write(void *threadid)
*Arguments  : threadid
*Return Type : void 
*Description :Write Function: is used for writing the ppm images to the disk
*The imwrite OpenCv function is used for writing the ppm images
******************************************/

void *image_write(void *threadid)
{
	
	uint8_t thread_id=2;
	ostringstream name;
	vector<int> comp_par;
	comp_par.push_back(CV_IMWRITE_PXM_BINARY);
	comp_par.push_back(95);
	struct utsname unameData;
	char PPM_IMAGE[30];
    char JPG_IMAGE[30];
	int Image_Index = 1;
	// 	ostringstream name;

	vector<int> comp_par_1;
	comp_par_1.push_back(CV_IMWRITE_JPEG_QUALITY);
	comp_par_1.push_back(95);	
	CvFont font;
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 0.5, 0.5);
	if (uname(&unameData) != 0) 
  {
		
    perror("uname");
	}
 	while(cond)
	{
 
		sem_wait(&sem_array[thread_id]);
		sem_wait(&ppm_sem);
   		 uint8_t str[10];
		display(counter_array[thread_id],4, str); 
	 	start_array[thread_id] = calc_ms();
		sprintf(PPM_IMAGE, "./frame_%04d.ppm", Image_Index);
		time (&r_time);
 		cur_t = localtime (&r_time);
		putText(ppm_frame,"SURAJ THITE",Point(0,20),FONT_HERSHEY_COMPLEX_SMALL,0.7,Scalar(0,128,255),1);
		putText(ppm_frame,asctime(cur_t),Point(0,950),FONT_HERSHEY_COMPLEX_SMALL,0.7,Scalar(0,128,255),1);
		putText(ppm_frame,unameData.sysname,Point(2,30),FONT_HERSHEY_COMPLEX_SMALL,0.7,Scalar(0,128,255),1);
		putText(ppm_frame,unameData.nodename,Point(60,30),FONT_HERSHEY_COMPLEX_SMALL,0.7,Scalar(0,128,255),1);
		putText(ppm_frame,unameData.release,Point(2,45),FONT_HERSHEY_COMPLEX_SMALL,0.7,Scalar(0,128,255),1);
		putText(ppm_frame,unameData.version,Point(170,45),FONT_HERSHEY_COMPLEX_SMALL,0.7,Scalar(0,128,255),1);
		putText(ppm_frame,unameData.machine,Point(2,60),FONT_HERSHEY_COMPLEX_SMALL,0.7,Scalar(0,128,255),1);
		imwrite(PPM_IMAGE, ppm_frame, comp_par);
		sprintf(JPG_IMAGE, "./frame_%04d.jpg", Image_Index);
 		frame_jpg = imread(PPM_IMAGE,CV_LOAD_IMAGE_COLOR); 
		imwrite(JPG_IMAGE, frame_jpg, comp_par_1);
		Image_Index++;
		jitter_calc(thread_id);
		sem_post(&ppm_sem);
		
  	}
	wcet_array[thread_id]=wcet_array[thread_id]/4;
	print_jitter(thread_id);
	pthread_exit(NULL);
}

/*****************************************
*Function name : void *frame_function(void *threadid)
*Arguments  : threadid
*Return Type : void 
*Description :Frame function: The function used to capture frames
*from the camera
******************************************/

void *frame_function(void *threadid)
{
	
  	uint8_t thread_id=1;	
	
	system("uname -a > system.out");
 	while(cond)
  	{
		//Wait for semaphore
    	sem_wait(&sem_array[thread_id]);
	  	start_array[thread_id] = calc_ms(); //Store the start time
		clock_gettime(CLOCK_REALTIME, &cap_strt_t); //Get the REALTIME
		cap >> ppm_frame; //Get the next frame
		sem_post(&ppm_sem);
		clock_gettime(CLOCK_REALTIME, &cap_stp_t);
		time_diff= ((cap_stp_t.tv_sec - cap_strt_t.tv_sec)*1000000000 + (cap_stp_t.tv_nsec - cap_strt_t.tv_nsec)); //To get the difference between capture stop time and capture start time
		syslog(LOG_DEBUG,"\n\r frame capture time is: %0.8lf ns\n", time_diff);
		frame_ptr = (uint8_t*) ppm_frame.data;
		jitter_calc(thread_id);
	  }
	print_jitter(thread_id); //Print the jitter for this thread
	pthread_exit(NULL); //Exit the pthread
}

/*****************************************
*Function name : void jitter_calc(uint8_t thread_id)
*Arguments : thread_id 
*Return Type : void 
*Description : Jitter calculations: Function for calculating 
*the jitter: the avg difference array, 
*the jitter calculation, the accumulated jitter, 
*the average jitter and the
*accumulated jitter.
*The jitter is calculated for every thread
******************************************/

void jitter_calc(uint8_t thread_id)
{
	
cout<<"\n\rTHREAD"<<thread_id+0; 
	syslog(LOG_DEBUG,"\n\rFRAME: %d\n",counter_array[thread_id]);
//printf("\n\rFRAME: %d\n",counter_array[thread_id]);
	syslog(LOG_DEBUG,"\n\rSTART TIME IN MILLI-SECONDS IS: %0.8lf \n", start_array[thread_id]);
//printf("\n\rSTART TIME IN MILLI-SECONDS IS: %0.8lf \n", start_array[thread_id]); 
 	stop_array[thread_id] = calc_ms();
	syslog(LOG_DEBUG,"\n\rSTOP TIME IN MILLI-SECONDS IS  is: %0.8lf\n", stop_array[thread_id]);
//printf("\n\rSTOP TIME IN MILLI-SECONDS IS  is: %0.8lf\n", stop_array[thread_id]);
  	run_time[thread_id] = stop_array[thread_id] - start_array[thread_id];
	syslog(LOG_DEBUG,"\n\rTHE RUN TIME IN MILLI-SECONDS IS: %0.8lf\n", run_time[thread_id]);
printf("\n\rTHE RUN TIME IN MILLI-SECONDS IS: %0.8lf\n", run_time[thread_id]);
	if(run_time[thread_id] > wcet_array[thread_id])
	{
		wcet_array[thread_id] = run_time[thread_id]; //Worst case jitter
		syslog(LOG_DEBUG,"THE WORST CASE EXECTUTION TIME FOR THREAD %d is %d",thread_id,wcet_array[thread_id]);
		//cout<<"\n\rTHE WORST CASE EXECTUTION TIME FOR THREAD IS"<<thread_id+0<<" ="<<wcet_array[thread_id];
	}
	if(counter_array[thread_id] == 0)
	{
      	avg_diff_array[thread_id] = run_time[thread_id]; //To get the average difference array
		syslog(LOG_DEBUG,"THE AVERAGE TIME DIFFERENCE ARRAY : %0.8lf ms", avg_diff_array[thread_id]);
	//printf("\n\rTHE AVERAGE TIME DIFFERENCE ARRAY : %0.8lf ms", avg_diff_array[thread_id]);
    	}
	else if(counter_array[thread_id] > 0)
	{
		jitter_calc_array[thread_id] = run_time[thread_id] - avg_diff_array[thread_id];
		avg_diff_array[thread_id] = (avg_diff_array[thread_id] * (counter_array[thread_id]-1) + run_time[thread_id])/(counter_array[thread_id]);
		syslog(LOG_DEBUG,"THE AVERAGE TIME DIFFERENCE ARRAY : %0.8lf ms", avg_diff_array[thread_id]);
		printf("\n\rTHE CALCUALTED JITTER IS: %0.8lf ms\n", jitter_calc_array[thread_id]); //To get the calculated jitter
		acc_jitter_array[thread_id] += jitter_calc_array[thread_id]; //The accumulated jitter is calculated
	}
	counter_array[thread_id]++;
}

/*****************************************
*Function name : double calc_ms(void)
*Arguments : void 
*Return Type : void 
*Description : calc_ms: Function to calculate ms val
*It uses clock get real time to calculate 
*time in ms.
*
******************************************/

double calc_ms(void)
{
	struct timespec scene = {0,0};
	clock_gettime(CLOCK_REALTIME, &scene);
	return ((scene.tv_sec*1000)+scene.tv_nsec/MSEC);
}

/*****************************************
*Function name : void print_jitter(uint8_t thread_id)
*Arguments : thread_id 
*Return Type : void 
*Description : Function for printing the final jitter values
******************************************/

void print_jitter(uint8_t thread_id)
{
	cout<<"\n\rThe accumulated jitter for thread "<<thread_id+0<<" ="<<acc_jitter_array[thread_id]; 
	cout<<"\n\r The worst execution time for thread"<<thread_id+0<<" ="<<wcet_array[thread_id];
	avg_jitter_array[thread_id]=acc_jitter_array[thread_id]/frames_count;
	cout<<"\n\rThe average jitter for thread in ms thread"<<thread_id+0<<" ="<<avg_jitter_array[thread_id]; 

}

/*****************************************
*Function name : void delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
*Arguments : start time , stop time and time difference 
*Return Type : void 
*Description : Function: delta_t for calculating the difference between two times
*It has three timespec structures as the parameters to the function.
******************************************/

void delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
  int dt_sec=stop->tv_sec - start->tv_sec;
  int dt_nsec=stop->tv_nsec - start->tv_nsec;

  if(dt_sec >= 0)
  {
    if(dt_nsec >= 0)
    {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
    }
    else
    {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
    }
  }
  else
  {
    if(dt_nsec >= 0)
    {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
    }
    else
    {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
    }
  }
  return;
}

/*****************************************
*Function name : void threads_init(void)
*Arguments: void
*Return Type : void 
*Description :*Function: For creating the threads, 
*initializing the semaphores and joining the 
*threads
******************************************/

void threads_init(void)
{
	uint8_t i=0; 
	uint8_t max_priority = sched_get_priority_max(SCHED_FIFO); 
	
	printf("\nCreating Threads");
	for(i=0;i<thread_cnt;i++)
	{
		pthread_attr_init(&att_array[i]);
		pthread_attr_setinheritsched(&att_array[i],PTHREAD_EXPLICIT_SCHED);
		pthread_attr_setschedpolicy(&att_array[i],SCHED_FIFO);
		par_array[i].sched_priority=max_priority-i;
		
				
		if (sem_init (&sem_array[i], 0, 0))
		{
			cout<<"\n\rFailed to initialize semaphore for thread"<<i;
			exit (-1);
		}
		if(sem_init(&ppm_sem, 0,1))
		{
			cout<<"\n\rFailed to initialize semaphore for thread";
			exit(-1);
		}

		cout<<"\nSemaphore "<<i+0<<" initialized"; 
		cout<<"\nPPM semaphore initialized";
    	pthread_attr_setschedparam(&att_array[i], &par_array[i]);
		if(pthread_create(&thread_array[i], &att_array[i], func_arr[i], (void *)0) !=0)
		{
			perror("ERROR; pthread_create:");
			exit(-1);
		}
		cout<<"\nthread "<<i+0<<" created";
	}
	
	sem_post(&sem_array[1]);
	cout<<"\n\rjoining Threads";
	for(i=0;i<thread_cnt;i++)
	{
  		pthread_join(thread_array[i],NULL);
	}
}

void check_time(void)
{
	if(++timer_ctr==frames_count)
	{
		cond = FALSE;
	}
}

/*****************************************
*Function name :  void *sequencer(void *threadid)
*Arguments  : threadid
*Return Type : void 
*Description :*Sequencer function to send the threads at proper rates
* The semaphores are posted for each thread based on the
*sequencer rates
******************************************/

void *sequencer(void *threadid)
{
	uint8_t thread_id = 0,i=0;	
	#ifdef HZ_1
	struct timespec delay_time = {1, 0};
	#else 
	struct timespec delay_time = {0, 25000000}; 
	#endif
	struct timespec remaining_time;
	uint32_t time_old = 1;
	unsigned long long seqCnt =0; 
	double residual; int rc; int delay_cnt =0;
	clock_gettime(CLOCK_REALTIME, &current_t);
	printf("\n\rThe current time is %d sec and %d nsec\n", current_t.tv_sec, current_t.tv_nsec);
	#ifdef HZ_1
	do 
	{       
		//sem_wait(&sem_array[0]);
		clock_gettime(CLOCK_REALTIME, &current_t);
		delay_cnt=0; residual=0.0;
		{
			start_ctr++;
			time_old = stop_t.tv_sec;
			do
			{
				rc=nanosleep(&delay_time, &remaining_time);
				if(rc==EINTR)
				{
					residual = remaining_time.tv_sec + ((double)remaining_time.tv_nsec/(double) NSEC_PER_SEC);
					delay_cnt++;
				}
				else if(rc < 0)
				{
					perror(" Sequencer nanosleep");
					exit(-1);
				}
			}while((residual > 0.0) && (delay_cnt < 100));
			seqCnt++;
			clock_gettime(CLOCK_REALTIME, &current_t);
			if(delay_cnt > 1)
			printf("%d seq_loop\n", delay_cnt);
			if((seqCnt % 1) == 0)
			{
				check_time();
				sem_post(&sem_array[1]);
				
			}
			if((seqCnt % 1) == 0)
			{
				
				sem_post(&sem_array[2]);
				
			}
		}
		sem_post(&sem_array[0]);

	}while(cond);
 #else 
	do 
	{       
		//sem_wait(&sem_array[0]);
		clock_gettime(CLOCK_REALTIME, &current_t);
		delay_cnt=0; residual=0.0;
		{
			start_ctr++;
			time_old = stop_t.tv_sec;
			do
			{
				rc=nanosleep(&delay_time, &remaining_time);
				if(rc==EINTR)
				{
					residual = remaining_time.tv_sec + ((double)remaining_time.tv_nsec/(double) NSEC_PER_SEC);
					delay_cnt++;
				}
				else if(rc < 0)
				{
					perror(" Sequencer nanosleep");
					exit(-1);
				}
			}while((residual > 0.0) && (delay_cnt < 100));
			seqCnt++;
			clock_gettime(CLOCK_REALTIME, &current_t);
				if(delay_cnt > 1)
			printf("%d seq_loop\n", delay_cnt);
			if((seqCnt % 3) == 0)
			{
				check_time();
				sem_post(&sem_array[1]);
				
			}
			if((seqCnt % 3) == 0)
			{
				
				sem_post(&sem_array[2]);
				
			}

		}
		sem_post(&sem_array[0]);

	}while(cond);
	#endif

	for(i=1;i<thread_cnt;i++)
	{
		sem_post(&sem_array[i]);
		
	}
	pthread_exit(NULL);
}

/*****************************************
*Function name :  void display(int disp, uint8_t w, uint8_t* store)
*Arguments  : disp,w,store
*Return Type : void 
*Description :*This functions pads the image with zeroes 
******************************************/
void display(int disp, uint8_t w, uint8_t* store)
{

	uint8_t character[10];
	int var=0, temp=0;
	uint8_t* store_string = character;
	for(temp=w; temp>1; temp--)
	{
		switch(temp)
			{
				case 2: 
					{
						var = 9;
						if(disp<= var)
						{
							*(store++) = '0';
						}
						break;
					}

				case 3:
					{
						var = 99;
						if(disp<= var)
						{
							*(store++) = '0';
						}
						break;
					}
				case 4:
					{
						var = 999;
						if(disp<= var)
						{
							*(store++) = '0';
						}
						break;
					}
				}
		}
		temp = 0;
		do
		{
			*(++store_string) = '0' + disp%10;
			disp/=10;
			temp++;
		}while(disp>0);
		for(;temp>0;temp--)
		{
			*(store++) = *(store_string--);
		}
		*(store) = 0;
			
						
	}


/*****************************************
*Function name :int main(int argc, char *argv[])
*Arguments  : int argc, char *argv[]
*Return Type : int 
*Description :main function where the execution of program begins
******************************************/

int main(int argc, char *argv[])
{
	clock_gettime(CLOCK_REALTIME, &start_t); /*To find the start time for the code*/
	printf("\n********THE START TIME IS  %d seconds AND %d nanoseconds*********\n", start_t.tv_sec, start_t.tv_nsec);
	openlog(NULL,0,LOG_USER);
	if(argc > 1)
	{
		sscanf(argv[1], "%d", &dev);
	}
	
	cap.set(CV_CAP_PROP_FRAME_WIDTH, HRES); //Set the horizontal resoulution
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, VRES);//Set the vertical resolution
	cap.set(CV_CAP_PROP_FPS,1800.0); //Set the camera rate to 2000
	cap.open(dev); //Start the Camera 
	printf("fps %lf\n", cap.get(CV_CAP_PROP_FPS));
	func_arr[0] = sequencer; //Sequencer thread
 	func_arr[1] = frame_function; //Frame Thread
  	func_arr[2] = image_write; //Write thread
	syslog(LOG_USER,"Initialiazing Threads\n");
 	threads_init(); 
	cap.release(); //Release the camera dev driver
	clock_gettime(CLOCK_REALTIME, &stop_t); //Get real time clock
	printf("\nTHE STOP TIME FOR CODE IS  %d seconds AND %d nanoseconds\n",stop_t.tv_sec, stop_t.tv_nsec); 
	delta_t(&stop_t, &start_t, &exec_t); //Calculate the execution time for the frames
	cout<<"\n\r THE EXECUTION TIME FOR ENTIRE CODE IS "<<exec_t.tv_sec<<" seconds "<<exec_t.tv_nsec<<" nano seconds."; 
	printf("\n*********COMPLETED EXECUTION**********\n");
}

	
