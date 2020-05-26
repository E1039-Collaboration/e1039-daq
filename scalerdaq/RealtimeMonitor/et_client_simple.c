/*----------------------------------------------------------------------------*
 *  Copyright (c) 1998        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12H        *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:                                                               *
 *      Simple ET system client.                                              *
 *      Creates or attaches to a station and consumes events until killed.    *
 *                                                                            *
 *    WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! *
 *      Alot of the error handling for a robust program has been left out of  *
 *      this simple demo. See et_client.c for a detailed example.             *
 *                                                                            *
 *----------------------------------------------------------------------------*/
 
#include <stdio.h>
#include <stdlib.h>


#include "et.h"

#define NUM_EVENTS      100
#define REPORT_INTERVAL 200000

/* Globals */
static char     *g_progname = "et_client_simple";
int counter = 0;

/******************************************************************************/

void analyze(int n_events,   et_event  *pArrayEv[]) {

  static int       print_event_info = 1;
  int              i,k;
  int              priority, length;
  void             *pData = NULL;
  unsigned long int         *kazdata = NULL;
  FILE *filehandle;
  filehandle = fopen("missed_event_check.out","a"); //APPENDS TO EXISTING FILE
  
  if (print_event_info) {
    //   printf("        Address        Priority          Length\n");
    for (i = 0; i < n_events; i++) {
      
      fprintf(filehandle,"%d %d  \n",counter,i);

      if (et_event_getdata(pArrayEv[i], &pData) == ET_OK) {
	//	et_event_getdata(pArrayEv[i], &kazdata);
	//	et_event_getdata(pArrayEv[i], (void **) &kazdata);
        if (et_event_getpriority(pArrayEv[i], &priority) == ET_OK) {
          if (et_event_getlength(pArrayEv[i], &length)== ET_OK) {

	    //	    printf("        %8X              %2d        %8d\n",(unsigned long int) pData, priority, length);
	    kazdata = (unsigned long int *) pData;

	    for(k=0;k<length;k++){
	      //	      printf("%d  %d  %d \n",i,k,kazdata[k]);//print data ala coda format in event#, word#, data
	      //	      fprintf(filehandle,"%d  %d  %lu \n",i,k,kazdata[k]);

	    }
	    //	    fprintf(filehandle,"12345  67890  24680 \n");//end of event
	    //	    fprintf(filehandle,"%d \n",kazdata[4]);//output event number to file to see if we missed any

          }
        }
      }
      counter++;
    }
    //    printf("        Address        Priority          Length\n");
  }
  fclose(filehandle);
}

int main(int argc,char **argv) {  

  char           *et_filename = NULL;
  char           *et_stnname  = NULL;

  et_sys_id       et_id;
  et_att_id       stn_door;
  et_stat_id      stn;

  et_event        Ev;
  et_event*       pSingleEv = &Ev;

  struct timespec timeout;

  et_event       *pArrayEv[NUM_EVENTS];

  int             status;
  int             n_events = 0, events_read=0;
 
  printf("Starting Program\n");

  if (argc != 3) {
    printf("Usage: %s <et_filename> <station_name>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  et_filename = argv[1];
  et_stnname  = argv[2];

  if (et_open(&et_id, et_filename, NULL) != ET_OK) {
    printf("%s: et_open problems\n", g_progname);
    exit(EXIT_FAILURE);
  }

  status = et_station_create(et_id, &stn, et_stnname, NULL);
  if (status < ET_OK) {
    printf("%s: error in station creation\n", g_progname);
    exit(EXIT_FAILURE);
  }

  if (et_station_attach(et_id, stn, &stn_door) < 0) {
    printf("%s: error in station attach\n", g_progname);
    exit(EXIT_FAILURE);
  }
  //  printf("debug1\n");
  /*****************************************/
  /* Single timed call to get events.      */
  /*****************************************/
  timeout.tv_sec  = 1;
  timeout.tv_nsec = 300*1000*1000; /* = 300 ms */

  status = et_event_get(et_id, stn_door, &pSingleEv, ET_TIMED, &timeout);

  if (status != ET_OK) {
    if (status == ET_ERROR_TIMEOUT) {
      printf("et_event_get() timed out after %ld sec, %ld nsec.\n",
             timeout.tv_sec, timeout.tv_nsec);
    } else {
      printf("et_event_get() error. Exiting.\n");
      exit(EXIT_FAILURE);
    }
  } else {
    //    printf("good\n");
    analyze( 1, &pSingleEv);
    status = et_event_put(et_id, stn_door, pSingleEv);
  }


  /**********************************************/
  /* Higher performance et_events_get loop:     */
  /*   get events, analyze, put events .        */
  /**********************************************/
  while (et_alive(et_id)) {

    status = et_events_get(et_id, stn_door, pArrayEv, 
                           ET_SLEEP, NULL, NUM_EVENTS, &n_events);
    if (status < ET_OK) {
      printf("%s: et_events_get error = %d.\n", g_progname, status);
      exit(EXIT_FAILURE);
    }

    analyze(n_events, pArrayEv);

    status = et_events_put(et_id, stn_door, pArrayEv, n_events);
    if (status != ET_OK) {
      printf("%s: put error\n", g_progname);
      exit(EXIT_FAILURE);
    }

    events_read += n_events;
    if (events_read >= REPORT_INTERVAL) {
      events_read = 0;
      printf(" et_client: %d events\n", REPORT_INTERVAL);
    }

    if ( ! et_alive(et_id)) {
      et_wait_for_alive(et_id);
    }
  }
    
  exit(EXIT_SUCCESS);
}

