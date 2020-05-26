#include "E906ET_netclient.h"
E906ET_NETCLIENT::E906ET_NETCLIENT()
{//constructor -- initialize all relevant variables
  writeIt=1; 
  printIt=1; 
  control[0]=1;
  control[1]=0;
  control[2]=0;
  control[3]=0;
  iterations=1;
  event_counter=0;
  freq=0.0; freq_tot=0; freq_avg=0.0;
  out.open("raw_ET_netclient_dump.out");
}

E906ET_NETCLIENT::~E906ET_NETCLIENT()
{//destructor -- kill all pointers, etc
  out.close();
}

ULong_t *E906ET_NETCLIENT::E906ET_netclient_returnarray(Int_t eventnum)
{
  ++event_counter;
  pdata = NULL;
  /* get pointer to data */
  et_event_getdata(evs[eventnum], (void **) &pdata);
  pInt = (unsigned long int *) pdata;
  /* do we need to swap data ? */
  et_event_needtoswap(evs[eventnum], &swap);
  if (swap == ET_SWAP) 
  {
    /* swap data  here */;
    *pInt = ntohl(*pInt);
  }

  return pInt;
}

int E906ET_NETCLIENT::E906ET_netclient_return_nevents(Int_t eventcounter)
{
  /*Get nevents from ET.  DO NOT call this as a limit of a loop, as nevents
    is call-by-reference.
    */

  if(eventcounter > pulse_frequency*4.5)
  {
    err = et_events_get(id, my_att, evs, ET_SLEEP, NULL, 1, &nevents);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_events_get\n");
      exit(1);
    }
  }
  else
  {  
    err = et_events_get(id, my_att, evs, ET_SLEEP, NULL, 1000, &nevents);
    if(0==nevents)
    {
      cout << eventcounter << endl;
      sleep(1);
    }
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_events_get\n");
      exit(1);
    }
  }
  return nevents;

}

void E906ET_NETCLIENT::E906ET_netclient_postprocess()
{
  err = et_events_put(id, my_att, evs, nevents);
  if (err < ET_OK) 
  {
    printf("et_netclient: error calling etr_events_put\n");
    exit(1);
  }

}

int E906ET_NETCLIENT::E906ET_netclient_alive()
{
  return et_alive(id);
}

void E906ET_NETCLIENT::E906ET_netclient_close()
{
  cout << "Close Netclient" << endl;
  err = et_station_detach(id, my_att);
  if (err == ET_OK)
    printf("et_netclient: detached from stat %d\n", my_stat);
  else 
  {
    printf("et_netclient: error detaching (%d)\n", err);
    exit(1);
  }

  err = et_station_remove(id, my_stat);
  if (err == ET_OK)
    printf("et_netclient: removed stat %d\n", my_stat);
  else
    printf("et_netclient: error removing stat (%d)\n", err);

  err = et_close(id);
  if (err == ET_OK)
    printf("et_netclient: ET closed\n");
  else
  {
    printf("et_netclient: error closing (%d)\n", err);
    exit(1);
  }

}

int E906ET_NETCLIENT::E906ET_netclient_error_reporting()
{

  if (1) 
  {
    /***********************************************************/
    err = et_events_get(id, my_att, evs, ET_SLEEP, NULL, 10, &nread);
    if (err < ET_OK) 
    {
      if (err == ET_ERROR_TIMEOUT) 
      {
        printf("et_netclient: timeout calling et_events_get\n");
      }
      else 
      {
        printf("et_netclient: error calling et_events_get, %d\n", err);
        exit(1);
      }
    }
    else 
    {
      if (printIt) 
      {
        sleep(3);
        for (j=0; j < nread; j++) 
        {
          et_event_getdata(evs[j], (void **) &pdata);
          printf(" evs[%d] = %s\n", j, pdata);
        }
      }
      err = et_events_dump(id, my_att, evs, nread);
      if (err < ET_OK) 
      {
        printf("et_netclient: error calling et_events_put, %d\n", err);
        exit(1);
      }
      printf("et_netclient: got & DUMPED chunk of events in SLEEP wait\n");
    }


    /***********************************************************/
    err = et_events_get(id, my_att, evs, ET_TIMED, &twait, 10, &nread);
    if (err < ET_OK) 
    {
      if (err == ET_ERROR_TIMEOUT) 
        printf("et_netclient: timeout calling et_events_get\n");
      else 
      {
        printf("et_netclient: error calling et_events_get, %d\n", err);
        exit(1);
      }
    }
    else 
    {
      if (printIt) 
      {
        sleep(3);
        for (j=0; j < nread; j++) 
        {
          et_event_getdata(evs[j], (void **) &pdata);
          printf(" evs[%d] = %s\n", j, pdata);
        }
      }
      err = et_events_put(id, my_att, evs, nread);
      if (err < ET_OK) 
      {
        printf("et_netclient: error calling et_events_put, %d\n", err);
        exit(1);
      }
      printf("et_netclient: got & put chunk events in TIMED wait (1sec)\n");
    }


    /***********************************************************/
    err = et_events_get(id, my_att, evs, ET_ASYNC, NULL, 10, &nread);
    if (err < ET_OK) 
    {
      if (err == ET_ERROR_BUSY) 
        printf("et_netclient: busy calling et_events_get\n");
      else if (err == ET_ERROR_EMPTY)
        printf("et_netclient: empty calling et_events_get\n");
      else 
      {
        printf("et_netclient: error calling et_events_get, %d\n", err);
        exit(1);
      }
    }
    else 
    {
      if (printIt) 
      {
        sleep(3);
        for (j=0; j < nread; j++) 
        {
          et_event_getdata(evs[j], (void **) &pdata);
          printf(" evs[%d] = %s\n", j, pdata);
        }
      }
      err = et_events_put(id, my_att, evs, nread);
      if (err < ET_OK) 
      {
        printf("et_netclient: error calling et_events_put, %d\n", err);
        exit(1);
      }
      printf("et_netclient: got & put chunk events in ASYNC wait\n");
    }

    /***********************************************************/
    err = et_events_get(id, my_att, evs, ET_SLEEP|ET_MODIFY, NULL, 10, &nread);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_events_get, %d\n", err);
      exit(1);
    }
    if (printIt) 
    {
      sleep(3);
      for (j=0; j < nread; j++) 
      {
        et_event_getdata(evs[j], (void **) &pdata);
        printf(" evs[%d] = %s\n", j, pdata);
      }
    }
    err = et_events_put(id, my_att, evs, nread);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_events_put, %d\n", err);
      exit(1);
    }
    printf("et_netclient: got & put chunk of events & modified (SLEEP)\n");

    /***********************************************************/
    err = et_events_get(id, my_att, evs, ET_TIMED|ET_MODIFY, &twait, 10, &nread);
    if (err < ET_OK) 
    {
      if (err == ET_ERROR_TIMEOUT)
        printf("et_netclient: timeout calling et_events_get\n");
      else 
      {
        printf("et_netclient: error calling et_events_get, %d\n", err);
        exit(1);
      }
    }
    else 
    {
      if (printIt) 
      {
        sleep(3);
        for (j=0; j < nread; j++) 
        {
          et_event_getdata(evs[j], (void **) &pdata);
          printf(" evs[%d] = %s\n", j, pdata);
        }
      }
      err = et_events_put(id, my_att, evs, nread);
      if (err < ET_OK) 
      {
        printf("et_netclient: error calling et_events_put, %d\n", err);
        exit(1);
      }
      printf("et_netclient: got & put a chunk of events & modified (TIMED)\n");
    }    


    /***********************************************************/
    err = et_events_get(id, my_att, evs, ET_ASYNC|ET_MODIFY, NULL, 10, &nread);
    if (err < ET_OK) 
    {
      if (err == ET_ERROR_BUSY)
        printf("et_netclient: busy calling et_events_get\n");
      else if (err == ET_ERROR_EMPTY)
        printf("et_netclient: empty calling et_events_get\n");
      else 
      {
        printf("et_netclient: error calling et_events_get, %d\n", err);
        exit(1);
      }
    }
    else 
    {
      if (printIt) 
      {
        sleep(3);
        for (j=0; j < nread; j++) 
        {
          et_event_getdata(evs[j], (void **) &pdata);
          printf(" evs[%d] = %s\n", j, pdata );
        }
      }
      err = et_events_put(id, my_att, evs, nread);
      if (err < ET_OK) 
      {
        printf("et_netclient: error calling et_events_put, %d\n", err);
        exit(1);
      }
      printf("et_netclient: got & put a chunk of events & modified (ASYNC)\n");
    }    

    /***********************************************************/
    err = et_event_new(id, my_att, &evs[0], ET_SLEEP, NULL, 16);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_event_new, %d\n", err);
      exit(1);
    }
    if (writeIt) 
    {
      et_event_setcontrol(evs[0], control, ET_STATION_SELECT_INTS);
      et_event_getdata(evs[0], (void **) &pdata);
      strcpy(pdata, "SLEEP");
      et_event_setlength(evs[0], strlen("SLEEP")+1);
    }
    err = et_event_put(id, my_att, evs[0]);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_event_put, %d\n", err);
      exit(1);
    }
    printf("et_netclient: got & put 1 NEW event (SLEEP)\n");



    /***********************************************************/
    err = et_event_new(id, my_att, &evs[0], ET_SLEEP, NULL, 16);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_event_new, %d\n", err);
      exit(1);
    }
    if (writeIt) 
    {
      et_event_setcontrol(evs[0], control, ET_STATION_SELECT_INTS);
      et_event_getdata(evs[0], (void **) &pdata);
      strcpy(pdata, "SLEEP");
      et_event_setlength(evs[0], strlen("SLEEP")+1);
    }
    err = et_event_dump(id, my_att, evs[0]);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_event_put, %d\n", err);
      exit(1);
    }
    printf("et_netclient: got & DUMPED 1 NEW event (SLEEP)\n");

    /***********************************************************/
    err = et_event_new(id, my_att, &evs[0], ET_TIMED, &twait, 16);
    if (err < ET_OK) 
    {
      if (err == ET_ERROR_TIMEOUT) 
        printf("et_netclient: timeout calling et_event_new\n");
      else 
      {
        printf("et_netclient: error calling et_event_new, %d\n", err);
        exit(1);
      }
    }
    else 
    {
      if (writeIt) 
      {
        et_event_setcontrol(evs[0], control, ET_STATION_SELECT_INTS);
        et_event_getdata(evs[0], (void **) &pdata);
        strcpy(pdata, "TIMED");
        et_event_setlength(evs[0], strlen("TIMED")+1);
      }
      err = et_event_put(id, my_att, evs[0]);
      if (err < ET_OK) 
      {
        printf("et_netclient: error calling et_event_put, %d\n", err);
        exit(1);
      }
      printf("et_netclient: got & put 1 NEW event (TIMED)\n");
    }   


    /***********************************************************/
    err = et_event_new(id, my_att, &evs[0], ET_ASYNC, &twait, 16);
    if (err < ET_OK) 
    {
      if (err == ET_ERROR_BUSY)
        printf("et_netclient: busy calling et_event_new\n");
      else if (err == ET_ERROR_EMPTY) 
      {
        printf("et_netclient: empty calling et_event_new\n");
        fflush(stdout);
      }
      else 
      {
        printf("et_netclient: error calling et_event_new, %d\n", err);
        exit(1);
      }
    }
    else 
    {
      if (writeIt) 
      {
        et_event_setcontrol(evs[0], control, ET_STATION_SELECT_INTS);
        et_event_getdata(evs[0], (void **) &pdata);
        strcpy(pdata, "ASYNC");
        et_event_setlength(evs[0], strlen("ASYNC")+1);
      }
      err = et_event_put(id, my_att, evs[0]);
      if (err < ET_OK) 
      {
        printf("et_netclient: error calling et_event_put, %d\n", err);
        exit(1);
      }
      printf("et_netclient: got & put 1 NEW event (ASYNC)\n");
    }


    /***********************************************************/
    err = et_events_new(id, my_att, evs, ET_SLEEP, NULL, 16, 10, &nread);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_events_new, %d\n", err);
      exit(1);
    }
    if (writeIt) 
    {
      for (j=0; j < nread; j++) 
      {
        et_event_setcontrol(evs[j], control, ET_STATION_SELECT_INTS);
        et_event_getdata(evs[j], (void **) &pdata);
        strcpy(pdata, "SLEEP");
        et_event_setlength(evs[j], strlen("SLEEP")+1);
      }
    }
    err = et_events_put(id, my_att, evs, nread);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_events_put, %d\n", err);
      exit(1);
    }
    printf("et_netclient: got & put chunk of NEW events in (SLEEP)\n");


    /***********************************************************/
    err = et_events_new(id, my_att, evs, ET_SLEEP, NULL, 16, 10, &nread);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_events_new, %d\n", err);
      exit(1);
    }
    if (writeIt) 
    {
      for (j=0; j < nread; j++) 
      {
        et_event_setcontrol(evs[j], control, ET_STATION_SELECT_INTS);
        et_event_getdata(evs[j], (void **) &pdata);
        strcpy(pdata, "SLEEP");
        et_event_setlength(evs[j], strlen("SLEEP")+1);
      }
    }
    err = et_events_dump(id, my_att, evs, nread);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_events_put, %d\n", err);
      exit(1);
    }
    printf("et_netclient: got & DUMPED chunk of NEW events in (SLEEP)\n");


    /***********************************************************/
    err = et_events_new(id, my_att, evs, ET_TIMED, &twait, 16, 10, &nread);
    if (err < ET_OK) 
    {
      if (err == ET_ERROR_TIMEOUT)
        printf("et_netclient: timeout calling et_events_new\n");
      else 
      {
        printf("et_netclient: error calling et_events_new, %d\n", err);
        exit(1);
      }
    }
    else
    {
      if (writeIt) 
      {
        for (j=0; j < nread; j++) 
        {
          et_event_setcontrol(evs[j], control, ET_STATION_SELECT_INTS);
          et_event_getdata(evs[j], (void **) &pdata);
          strcpy(pdata, "TIMED");
          et_event_setlength(evs[j], strlen("TIMED")+1);
        }
      }
      err = et_events_put(id, my_att, evs, nread);
      if (err < ET_OK) 
      {
        printf("et_netclient: error calling et_events_put, %d\n", err);
        exit(1);
      }
      printf("et_netclient: got & put chunk of NEW events in (TIMED)\n");
    }


    /***********************************************************/
    err = et_events_new(id, my_att, evs, ET_ASYNC, &twait, 16, 10, &nread);
    if (err < ET_OK) 
    {
      if (err == ET_ERROR_BUSY) 
        printf("et_netclient: busy calling et_events_new\n");
      else if (err == ET_ERROR_EMPTY)
        printf("et_netclient: empty calling et_events_new\n");
      else 
      {
        printf("et_netclient: error calling et_events_new, %d\n", err);
        exit(1);
      }
    }
    else 
    {
      if (writeIt) 
      {
        for (j=0; j < nread; j++) 
        {
          et_event_setcontrol(evs[j], control, ET_STATION_SELECT_INTS);
          et_event_getdata(evs[j], (void **) &pdata);
          strcpy(pdata, "ASYNC");
          et_event_setlength(evs[j], strlen("ASYNC")+1);
        }
      }
      err = et_events_put(id, my_att, evs, nread);
      if (err < ET_OK) 
      {
        printf("et_netclient: error calling et_events_put, %d\n", err);
        exit(1);
      }
      printf("et_netclient: got & put chunk of NEW events in (ASYNC)\n");
    }


    /***********************************************************/
    err = et_event_get(id, my_att, &evs[0], ET_SLEEP, NULL);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_event_get, %d\n", err);
      exit(1);
    }
    if (printIt) 
    {
      sleep(3);
      et_event_getdata(evs[0], (void **) &pdata);
      printf(" evs[0] = %s\n", pdata);
    }
    err = et_event_put(id, my_att, evs[0]);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_event_put, %d\n", err);
      exit(1);
    }
    printf("et_netclient: got & put 1 event singly (SLEEP)\n");


    /***********************************************************/
    err = et_event_get(id, my_att, &evs[0], ET_SLEEP, NULL);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_event_get, %d\n", err);
      exit(1);
    }
    if (printIt) 
    {
      sleep(3);
      et_event_getdata(evs[0], (void **) &pdata);
      printf(" evs[0] = %s\n", pdata);
    }
    err = et_event_dump(id, my_att, evs[0]);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_event_put, %d\n", err);
      exit(1);
    }
    printf("et_netclient: got & DUMPED 1 event singly (SLEEP)\n");


    /***********************************************************/
    err = et_event_get(id, my_att, &evs[0], ET_TIMED, &twait);
    if (err < ET_OK) 
    {
      if (err == ET_ERROR_TIMEOUT)
        printf("et_netclient: timeout calling et_event_get\n");
      else 
      {
        printf("et_netclient: error calling et_event_get, %d\n", err);
        exit(1);
      }
    }
    else 
    {
      if (printIt) 
      {
        sleep(3);
        et_event_getdata(evs[0], (void **) &pdata);
        printf(" evs[0] = %s\n", pdata);
      }
      err = et_event_put(id, my_att, evs[0]);
      if (err < ET_OK) 
      {
        printf("et_netclient: error calling et_event_put, %d\n", err);
        exit(1);
      }
      printf("et_netclient: got & put 1 event singly (TIMED)\n");
    }


    /***********************************************************/
    err = et_event_get(id, my_att, &evs[0], ET_ASYNC, &twait);
    if (err < ET_OK) 
    {
      if (err == ET_ERROR_BUSY)
        printf("et_netclient: busy calling et_event_get\n");
      else if (err == ET_ERROR_EMPTY)
        printf("et_netclient: empty calling et_event_get\n");
      else 
      {
        printf("et_netclient: error calling et_event_get, %d\n", err);
        exit(1);
      }
    }
    else
    {
      if (printIt)
      {
        sleep(3);
        et_event_getdata(evs[0], (void **) &pdata);
        printf(" evs[0] = %s\n", pdata);
      }
      err = et_event_put(id, my_att, evs[0]);
      if (err < ET_OK) 
      {
        printf("et_netclient: error calling et_event_put, %d\n", err);
        exit(1);
      }
      printf("et_netclient: got & put 1 event singly (ASYNC)\n");
    }


    /***********************************************************/
    err = et_event_get(id, my_att, &evs[0], ET_SLEEP|ET_MODIFY, NULL);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_event_get, %d\n", err);
      exit(1);
    }
    if (printIt)
    {
      sleep(3);
      et_event_getdata(evs[0], (void **) &pdata);
      printf(" evs[0] = %s\n", pdata);
    }
    err = et_event_put(id, my_att, evs[0]);
    if (err < ET_OK) 
    {
      printf("et_netclient: error calling et_event_put, %d\n", err);
      exit(1);
    }
    printf("et_netclient: got & put 1 event singly & modified (SLEEP)\n");


    /***********************************************************/
    err = et_event_get(id, my_att, &evs[0], ET_TIMED|ET_MODIFY, &twait);
    if (err < ET_OK) 
    {
      if (err == ET_ERROR_TIMEOUT) 
        printf("et_netclient: timeout calling et_event_get\n");
      else 
      {
        printf("et_netclient: error calling et_event_get, %d\n", err);
        exit(1);
      }
    }
    else 
    {
      if (printIt) 
      {
        sleep(3);
        et_event_getdata(evs[0], (void **) &pdata);
        printf(" evs[0] = %s\n", pdata);
      }
      err = et_event_put(id, my_att, evs[0]);
      if (err < ET_OK) 
      {
        printf("et_netclient: error calling et_event_put, %d\n", err);
        exit(1);
      }
      printf("et_netclient: got & put 1 event singly & modified (TIMED)\n");
    }


    /***********************************************************/
    err = et_event_get(id, my_att, &evs[0], ET_ASYNC|ET_MODIFY, &twait);
    if (err < ET_OK) 
    {
      if (err == ET_ERROR_BUSY)
        printf("et_netclient: busy calling et_event_get\n");
      else if (err == ET_ERROR_EMPTY)
        printf("et_netclient: empty calling et_event_get\n");
      else 
      {
        printf("et_netclient: error calling et_event_get, %d\n", err);
        exit(1);
      }
    }
    else 
    {
      if (printIt) 
      {
        sleep(3);
        et_event_getdata(evs[0], (void **) &pdata);
        printf(" evs[0] = %s\n", pdata);
      }
      err = et_event_put(id, my_att, evs[0]);
      if (err < ET_OK) 
      {
        printf("et_netclient: error calling et_event_put, %d\n", err);
        exit(1);
      }
      printf("et_netclient: got & put 1 event singly & modified (ASYNC)\n");   
    }


    /***********************************************************/
    if ((err=et_alive(id)) == 1)
      printf("et_netclient: ET is alive\n");
    else if (err == ET_ERROR_WRITE) 
    {
      printf("et_netclient: et_alive write error\n");
      exit(1);
    }
    else if (err == ET_ERROR_READ) 
    {
      printf("et_netclient: et_alive read error\n");
      exit(1);
    }
    else 
    {
      printf("et_netclient: ET is dead\n");
      exit(1);
    }


    if ((err = et_wait_for_alive(id)) == ET_OK)
      printf("et_netclient: waited for alive\n");
    else if (err == ET_ERROR_WRITE) 
    {
      printf("et_netclient: et_wait_for_alive write error\n");
      exit(1);
    }
    else if (err == ET_ERROR_READ) 
    {
      printf("et_netclient: et_wait_for_alive read error\n");
      exit(1);
    }


    err = et_station_isattached(id, my_stat, my_att);
    if (err == 1)
      printf("et_netclient: this attachment (%d) is attached to stat %d\n", my_att, my_stat);
    else if (err == 0)
      printf("et_netclient: this attachment is NOT attached\n");
    else 
    {
      printf("et_netclient: et_station_isattached error %d\n", err);
      exit(1);
    }


    err = et_station_exists(id, &my_stat, station);
    if (err == 1)
      printf("et_netclient: %s (id=%d) exists\n", station, my_stat);
    else if (err == 0)
      printf("et_netclient: %s does NOT exist\n", station);
    else 
    {
      printf("et_netclient: et_station_exists error %d\n", err);
      exit(1);
    }


    err = et_station_name_to_id(id, &my_stat, station);
    if (err == ET_OK)
      printf("et_netclient: %s has id = %d\n", station, my_stat);
    else 
    {
      printf("et_netclient: et_station_name_to_id error %d\n", err);
      exit(1);
    }

  } /* if(0) */


  /* station parameters */
  if (1) 
  {
    int  words[ET_STATION_SELECT_INTS], val;
    char lib[ET_FILENAME_LENGTH];

    if ( (err = et_station_getselectwords(id, my_stat, words)) != ET_OK)
      printf("et_netclient: et_station_getselectwords BAD\n");
    else
      printf("et_netclient: et_station_getselectwords %d,%d,%d,%d\n",
          words[0], words[1], words[2], words[3]);

    if ( (err = et_station_getlib(id, my_stat, lib)) != ET_OK)
      printf("et_netclient: et_station_getlib BAD\n");
    else
      printf("et_netclient: et_station_getlib (%s)\n", lib);

    if ( (err = et_station_getfunction(id, my_stat, lib)) != ET_OK)
      printf("et_netclient: et_station_getfunction BAD\n");
    else
      printf("et_netclient: et_station_getfunction (%s)\n", lib);

    if ( (err = et_station_getattachments(id, my_stat, &val)) != ET_OK)
      printf("et_netclient: et_station_getattachments BAD\n");
    else
      printf("et_netclient: et_station_getattachments (%d)\n", val);

    if ( (err = et_station_getstatus(id, my_stat, &val)) != ET_OK)
      printf("et_netclient: et_station_getstatus BAD\n");
    else
      printf("et_netclient: et_station_getstatus (%d)\n", val);

    if ( (err = et_station_getinputcount(id, my_stat, &val)) != ET_OK)
      printf("et_netclient: et_station_getinputcount BAD\n");
    else
      printf("et_netclient: et_station_getinputcount (%d)\n", val);

    if ( (err = et_station_getoutputcount(id, my_stat, &val)) != ET_OK)
      printf("et_netclient: et_station_getoutputcount BAD\n");
    else
      printf("et_netclient: et_station_getoutputcount (%d)\n", val);

    if ( (err = et_station_getblock(id, my_stat, &val)) != ET_OK)
      printf("et_netclient: et_station_getblock BAD\n");
    else
      printf("et_netclient: et_station_getblock (%d)\n", val);

    if ( (err = et_station_getuser(id, my_stat, &val)) != ET_OK)
      printf("et_netclient: et_station_getuser BAD\n");
    else
      printf("et_netclient: et_station_getuser (%d)\n", val);

    if ( (err = et_station_getrestore(id, my_stat, &val)) != ET_OK)
      printf("et_netclient: et_station_getrestore BAD\n");
    else
      printf("et_netclient: et_station_getrestore (%d)\n", val);

    if ( (err = et_station_getprescale(id, my_stat, &val)) != ET_OK)
      printf("et_netclient: et_station_getprescale BAD\n");
    else
      printf("et_netclient: et_station_getprescale (%d)\n", val);

    if ( (err = et_station_getcue(id, my_stat, &val)) != ET_OK)
      printf("et_netclient: et_station_getcue BAD\n");
    else
      printf("et_netclient: et_station_getcue (%d)\n", val);

    if ( (err = et_station_getselect(id, my_stat, &val)) != ET_OK)
      printf("et_netclient: et_station_getselect BAD\n");
    else
      printf("et_netclient: et_station_getselect (%d)\n", val);
  }

  /* system parameters */
  if (1) 
  {
    int  val;
    pid_t pid;

    if ( (err = et_system_gettemps(id, &val)) != ET_OK)
      printf("et_netclient: et_system_gettemps BAD\n");
    else
      printf("et_netclient: et_system_gettemps (%d)\n", val);

    if ( (err = et_system_gettempsmax(id, &val)) != ET_OK)
      printf("et_netclient: et_system_gettempsmax BAD\n");
    else
      printf("et_netclient: et_system_gettempsmax (%d)\n", val);

    if ( (err = et_system_getstations(id, &val)) != ET_OK)
      printf("et_netclient: et_system_getstations BAD\n");
    else
      printf("et_netclient: et_system_getstations (%d)\n", val);

    if ( (err = et_system_getstationsmax(id, &val)) != ET_OK)
      printf("et_netclient: et_system_getstationsmax BAD\n");
    else
      printf("et_netclient: et_system_getstationsmax (%d)\n", val);

    if ( (err = et_system_getprocs(id, &val)) != ET_OK)
      printf("et_netclient: et_system_getprocs BAD\n");
    else
      printf("et_netclient: et_system_getprocs (%d)\n", val);

    if ( (err = et_system_getprocsmax(id, &val)) != ET_OK)
      printf("et_netclient: et_system_getprocsmax BAD\n");
    else
      printf("et_netclient: et_system_getprocsmax (%d)\n", val);

    if ( (err = et_system_getattachments(id, &val)) != ET_OK)
      printf("et_netclient: et_system_getattachments BAD\n");
    else
      printf("et_netclient: et_system_getattachments (%d)\n", val);

    if ( (err = et_system_getattsmax(id, &val)) != ET_OK)
      printf("et_netclient: et_system_getattsmax BAD\n");
    else
      printf("et_netclient: et_system_getattsmax (%d)\n", val);

    if ( (err = et_system_getheartbeat(id, &val)) != ET_OK)
      printf("et_netclient: et_system_getheartbeat BAD\n");
    else
      printf("et_netclient: et_system_getheartbeat (%d)\n", val);

    if ( (err = et_system_getpid(id, &pid)) != ET_OK)
      printf("et_netclient: et_system_getpid BAD\n");
    else
      printf("et_netclient: et_system_getpid (%d)\n", pid);
  }

  err = et_station_detach(id, my_att);
  if (err == ET_OK)
    printf("et_netclient: detached from stat %d\n", my_stat);
  else 
  {
    printf("et_netclient: error detaching (%d)\n", err);
    exit(1);
  }

  err = et_station_remove(id, my_stat);
  if (err == ET_OK)
    printf("et_netclient: removed stat %d\n", my_stat);
  else
    printf("et_netclient: error removing stat (%d)\n", err);

  err = et_close(id);
  if (err == ET_OK)
    printf("et_netclient: ET closed\n");
  else 
  {
    printf("et_netclient: error closing (%d)\n", err);
    exit(1);
  }
  return err;
}

void E906ET_NETCLIENT::E906ET_netclient_config(const char *host)
{
  sprintf( station, host );
  twait.tv_sec  = 1;
  twait.tv_nsec = 0;
  cout << "Station name is " << station << endl;
  printf("ET open config...\n");
  et_open_config_init(&openconfig);

  //the next lines is generated automatically by the Makefile.  Do NOT change it.
  char hostname[]="e906sc3.sq.pri"; //Makefile.auto.1
  et_open_config_sethost(openconfig, hostname);

  et_open_config_setmode(openconfig, ET_HOST_AS_REMOTE);
  if (et_open(&id, "/tmp/et_sys_Sea2sc", openconfig) != ET_OK) 
  {
    printf("et_netclient: cannot open ET system\n");
    exit(1);
  }

  et_open_config_destroy(openconfig);

  printf("Station definition...\n");
  /* define a station */
  et_station_config_init(&sconfig);
  et_station_config_setuser(sconfig, ET_STATION_USER_MULTI);
  et_station_config_setrestore(sconfig, ET_STATION_RESTORE_OUT);
  et_station_config_setprescale(sconfig, 1);
  et_station_config_setcue(sconfig, 2000);
  et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
  et_station_config_setblock(sconfig, ET_STATION_BLOCKING);


  printf("Create Station...\n");
  /* create a station */
  if ((status = et_station_create(id, &my_stat, station, sconfig)) < ET_OK) 
  {
    if (status == ET_ERROR_EXISTS) 
    {
      /* my_stat contains pointer to existing station */;
      printf("et_netclient: station already exists\n");
    }
    else if (status == ET_ERROR_TOOMANY) 
    {
      printf("et_netclient: too many stations created\n");
      exit(1);
    }
    else if (status == ET_ERROR_REMOTE) 
    {
      printf("et_netclient: memory or improper arg problems\n");
      exit(1);
    }
    else if (status == ET_ERROR_READ) 
    {
      printf("et_netclient: network reading problem\n");
      exit(1);
    }
    else if (status == ET_ERROR_WRITE) 
    {
      printf("et_netclient: network writing problem\n");
      exit(1);
    }
    else 
    {
      printf("et_netclient: error in station creation\n");
      exit(1);
    }
  }
  et_station_config_destroy(sconfig);

  /* create an attachment */
  if (et_station_attach(id, my_stat, &my_att) < 0) 
  {
    printf("et_netclient: error in station attach\n");
    exit(1);
  }

  neventstotal=0;
  gettimeofday(&t1, NULL);
}
