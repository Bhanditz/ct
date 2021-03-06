/*

 Getting and Compiling the Program:

 curl  https://raw.githubusercontent.com/mchirico/ct/master/src/ct.c  > ct.c
 gcc ct.c -o ct -lpthread



 The MIT License (MIT)
 Copyright (c) 2005 Mike Chirico mchirico@gmail.com
 https://github.com/mchirico/ct


 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.



 Description:

 Simple program for testing a tcp connection, without blocking.


 Example Usage:

 $ ./ct gmail.com 80
 03-16-2013 12:59:32.20282,gmail.com,74.125.226.213,80,Connected,***   Good  ***

 $ ./ct gmail.com 81
 03-16-2013 13:00:10.79637,gmail.com,74.125.226.213,81,No Connection

 Or, you can but this in a quick script. It will never block, so your script
 can go about it's business.

 $ for i in $(seq 79 84); do ./ct gmail.com ${i}; done
 03-16-2013 13:15:35.20628,gmail.com,74.125.226.245,79,No Connection,timeout
 03-16-2013 13:15:36.24639,gmail.com,74.125.226.246,80,Connected,***   Good  ***
 03-16-2013 13:15:37.28678,gmail.com,74.125.226.245,81,No Connection,timeout
 03-16-2013 13:15:38.32927,gmail.com,74.125.226.246,82,No Connection,timeout
 03-16-2013 13:15:39.37058,gmail.com,74.125.226.245,83,No Connection,timeout
 03-16-2013 13:15:40.41139,gmail.com,74.125.226.246,84,No Connection,timeout







 TODO:


 Phase I.

 Getting working prototype up and running fast. Do
 NOT worry about anything but getting this program
 running.

 Phase II.

 Find out what is useful in the running program.
 Get new ideas and quickly implement.

 Phase III.

 Start to fix,re-factor and make more efficient.

 Phase IV.

 Repeat Phase I-IV.


 IDEAS:

 Add getop switches, speed-up the program.




 */


#include <arpa/inet.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

#define SA      struct sockaddr
#define MAXLINE 4096
#define MAXSUB  200
#define MAX_WORKER_THREADS 50
#define MAX_NUM_THREAD_DATABASE MAX_WORKER_THREADS + 1
#define TIMEBUF_SIZE 30
#define LISTENQ         1024

#define _VERSION_ "0.0.4b"
#define MAX_PERMITTED_SCANS 10000

extern int h_errno;


typedef struct str_thdata
{
    int thread_no;
    int sockfd;
    int status;
    struct sockaddr_in servaddr;
    char hname[MAXSUB + 1];
    char port[MAXSUB + 1];
    char ip[MAXSUB + 1];
    char time_buffer[TIMEBUF_SIZE + 1];
    char comment[MAXSUB + 1];

} thdata;



/* BEGIN OF ADDING VECTOR */
#if !defined(__APPLE__)
#include <malloc.h>
#endif


typedef struct
{
    char **key;
    char **val;
    int argc;
} Key_val;

typedef struct
{
    char **key;
    Key_val **val;
    int argc;
} Vec;

Vec *vecAdd (Vec * c, const char *key, Key_val * val);
Key_val *keyAdd (Key_val * c, const char *key, const char *val);
void pr (Key_val * c);
void prV (Vec * c);
void myfree (Key_val * c);
void myfreeV (Vec * c);
char *find (Key_val * c, const char *s);
Key_val *findK (Vec * c, const char *s);
int pvLength (Vec * c);
int modify (Key_val * c, const char *s, const char *new_val);
Key_val *getK (Vec * c, int index);
char *getV (Vec * c, int index);
char *getKkey (Key_val * c, int index);

/*
 Call this from parse hosts

 */
Key_val *
parse_ports (char *s, char **array, Key_val * k)
{

    char *str1, *str2, *token, *subtoken;
    char *saveptr1, *saveptr2;
    char *sep1 = ",";
    char *sep2 = "-";

    int count = 0;
    int count2 = 0;
    char last[50];
    int j;

    for (j = 1, str1 = s;; j++, str1 = NULL)
    {
        token = strtok_r (str1, sep1, &saveptr1);
        if (token == NULL)
            break;
        //printf("%d: %s\n", j, token);
        count2 = 0;
        for (str2 = token;; str2 = NULL)
        {
            subtoken = strtok_r (str2, sep2, &saveptr2);
            if (subtoken == NULL)
                break;
            ++count2;
            if (count2 == 2)
            {
                //printf("We have double last=%s current=%s\n", last, subtoken);
                int beg = atoi (last);
                int end = atoi (subtoken);
                if (beg >= 1 && end <= 60000)
                    if (beg < end)
                    {
                        int t;
                        for (t = beg + 1; t < end; ++t)
                        {
                            //printf("G %d\n", t);
                            char ts[15];
                            snprintf (ts, 15, "%d", t);
                            k = keyAdd (k, ts, "0");
                            ++count;
                        }
                        //printf("\n");

                    }
            }
            snprintf (last, 50, "%s", subtoken);
            char ts[15];
            snprintf (ts, 15, "%s", last);
            if (atoi (ts) < 0 || atoi (ts) > 60000)
            {
                exit (-1);
            }
            k = keyAdd (k, ts, "0");
            ++count;
        }
    }
    if (k == NULL)
        printf ("ODD should not be null. ERR:01\n");
    return k;
}



Vec *
parse_hosts (char *s, char **array, Vec * v, const char *argv2)
{

    Key_val *k = NULL;
    char **tarray = NULL;

    char ports[1000];


    char *str1, *str2, *token, *subtoken;
    char *saveptr1, *saveptr2;
    char *sep1 = ",";
    char *sep2 = "|";

    int count = 0;
    int count2 = 0;
    char last[50];
    int j;

    for (j = 1, str1 = s;; j++, str1 = NULL)
    {
        token = strtok_r (str1, sep1, &saveptr1);
        if (token == NULL)
            break;
        //printf("%d: %s\n", j, token);
        count2 = 0;
        for (str2 = token;; str2 = NULL)
        {
            subtoken = strtok_r (str2, sep2, &saveptr2);
            if (subtoken == NULL)
                break;
            ++count2;
            if (count2 == 2)
            {
                printf ("We have double last=%s current=%s\n", last, subtoken);
            }
            snprintf (last, 50, "%s", subtoken);
            // This is hostname
            char ts[45];
            snprintf (ts, 45, "%s", last);
            k = NULL;
            strcpy (ports, argv2);
            k = parse_ports (ports, tarray, k);
            v = vecAdd (v, ts, k);
            ++count;
        }
    }

    return v;
}




Vec *
vecAdd (Vec * c, const char *key, Key_val * val)
{

    char *s = NULL;
    Key_val *v = NULL;
    char **t = NULL;
    Key_val **tC = NULL;

    s = (char *) malloc (sizeof (char) * (strlen (key) + 1));
    if (s == NULL)
        return NULL;

    v = val;

    strcpy (s, key);

    if (c == NULL)
    {
        c = (Vec *) malloc (sizeof (Vec));
        if (c == NULL)
            return NULL;
        c->key = NULL;
        c->val = NULL;
        c->argc = 0;
    }
    c->argc = c->argc + 1;
    t = (char **) realloc (c->key,
                           sizeof (char *) * (long unsigned int) c->argc);
    if (t == NULL)
        return NULL;

    t[c->argc - 1] = s;
    c->key = t;

    tC = realloc (c->val, sizeof (Key_val *) * (long unsigned int) c->argc);
    if (tC == NULL)
        return NULL;
    tC[c->argc - 1] = v;
    c->val = tC;

    return c;
}

Key_val *
keyAdd (Key_val * c, const char *key, const char *val)
{

    char *s = NULL;
    char *v = NULL;
    char **t = NULL;

    s = (char *) malloc (sizeof (char) * (strlen (key) + 1));
    if (s == NULL)
        return NULL;
    v = (char *) malloc (sizeof (char) * (strlen (val) + 1));
    if (v == NULL)
        return NULL;

    strcpy (s, key);
    strcpy (v, val);

    if (c == NULL)
    {
        c = (Key_val *) malloc (sizeof (Key_val));
        if (c == NULL)
            return NULL;
        c->key = NULL;
        c->val = NULL;
        c->argc = 0;
    }
    c->argc = c->argc + 1;
    t = realloc (c->key, sizeof (char *) * (long unsigned int) c->argc);
    if (t == NULL)
        return NULL;

    t[c->argc - 1] = s;
    c->key = t;

    t = realloc (c->val, sizeof (char *) * (long unsigned int) c->argc);
    if (t == NULL)
        return NULL;
    t[c->argc - 1] = v;
    c->val = t;

    return c;
}

void
pr (Key_val * c)
{
    int i;

    if (c == NULL)
        return;
    for (i = 0; i < c->argc; ++i)
        printf ("%s->%s\n", c->key[i], c->val[i]);

    return;
}



char *
getV (Vec * c, int index)
{

    if (index >= c->argc || index < 0)
        return NULL;

    return c->key[index];
}

char *
getKkey (Key_val * c, int index)
{

    if (index >= c->argc || index < 0)
        return NULL;

    return c->key[index];
}



void
prV (Vec * c)
{
    int i;

    if (c == NULL)
        return;
    for (i = 0; i < c->argc; ++i)
    {
        printf ("[%s]=>\n", c->key[i]);
        pr (c->val[i]);
        printf ("\n\n");

    }
    return;
}

int
pvLength (Vec * c)
{
    return c->argc;
}

void
myfree (Key_val * c)
{
    if (c == NULL)
        return;

    int i;
    for (i = 0; i < c->argc; ++i)
    {
        free (c->key[i]);
        free (c->val[i]);
    }
    free (c->key);
    free (c->val);
    free (c);

}

void
myfreeV (Vec * c)
{
    if (c == NULL)
        return;

    int i;
    for (i = 0; i < c->argc; ++i)
    {
        free (c->key[i]);
        myfree (c->val[i]);
    }
    free (c->key);
    free (c->val);
    free (c);

}

char *
find (Key_val * c, const char *s)
{
    int i;
    for (i = 0; i < c->argc; ++i)
    {
        if (strcmp (c->key[i], s) == 0)
            return c->val[i];
    }

    return NULL;
}


/*  Add the key value combination, if we do not
    find it.
 */
int
modify (Key_val * c, const char *s, const char *new_val)
{
    int i;
    char **t = NULL;
    char *tt = NULL;
    for (i = 0; i < c->argc; ++i)
    {
        if (strcmp (c->key[i], s) == 0)
        {
            tt = realloc (c->val[i], sizeof (char *) * (strlen (new_val)));
            strcpy (tt, new_val);
            c->val[i] = tt;
            return 1;
        }
    }

    c->argc = c->argc + 1;
    t = (char **) realloc (c->key, sizeof (char *) *
                           (long unsigned int) c->argc);
    tt = realloc (c->val[i], sizeof (char *) * (strlen (new_val)));
    strcpy (tt, new_val);
    c->val[i] = tt;

    c = keyAdd (c, s, new_val);

    return 0;
}



/*
 Find a particular key_val in a vector given
 a vector key.
 */
Key_val *
findK (Vec * c, const char *s)
{
    int i;
    for (i = 0; i < c->argc; ++i)
        if (strcmp (c->key[i], s) == 0)
            return c->val[i];

    return NULL;
}

Key_val *
getK (Vec * c, int i)
{

    if (i >= 0 && i < c->argc)
        return c->val[i];

    return NULL;
}

/* END OF ADDING VECTOR */



FILE *
Popen (const char *command, const char *mode)
{
    FILE *fp;

    if ((fp = popen (command, mode)) == NULL)
        fprintf (stderr, "popen error");
    return (fp);
}

int
Pclose (FILE * fp)
{
    int n;

    if ((n = pclose (fp)) == -1)
        fprintf (stderr, "pclose error");
    return (n);
}

char *
Fgets (char *ptr, int n, FILE * stream)
{
    char *rptr;

    if ((rptr = fgets (ptr, n, stream)) == NULL && ferror (stream))
        fprintf (stderr, "fgets error");

    return (rptr);
}

void
Fputs (const char *ptr, FILE * stream)
{
    if (fputs (ptr, stream) == EOF)
        fprintf (stderr, "fputs error");
}



extern int errno;
ssize_t
process (int sockfd, char *cmd)
{
    char sendline[MAXLINE + 1], recvline[MAXLINE + 1];
    ssize_t n;
    FILE *fp;
    char buf[MAXLINE + 1];

    //fp = Popen("date", "r");
    fp = Popen (cmd, "r");

    while (Fgets (buf, MAXLINE, fp) != NULL)
    {
        Fputs (buf, stdout);
        snprintf (sendline, MAXSUB, "%s\n", buf);
        write (sockfd, sendline, strlen (sendline));
        n = read (sockfd, recvline, MAXLINE);
        recvline[n] = '\0';
        printf ("%s", recvline);
    }

    Pclose (fp);
    return n;

}


//03 - 16 - 2013 16: 10:43.203604
int
getTime (char *buffer)
{
    char tbuff[30];
    struct timeval tv;

    time_t curtime;


    gettimeofday (&tv, NULL);
    curtime = tv.tv_sec;
    strftime (tbuff, 30, "%m-%d-%Y %T.", localtime (&curtime));
#if defined(__APPLE__)
    snprintf (buffer, 30, "%s%d", tbuff, tv.tv_usec);
#else
    snprintf (buffer, 30, "%s%ld", tbuff, tv.tv_usec);
#endif


    return (int) strlen (buffer);

}



void
prTime ()
{
    char buffer[30];
    struct timeval tv;

    time_t curtime;

    gettimeofday (&tv, NULL);
    curtime = tv.tv_sec;

    strftime (buffer, 30, "%m-%d-%Y %T.", localtime (&curtime));
#if defined(__APPLE__)
    printf ("%s%d", buffer, tv.tv_usec);
#else
    printf ("%s%ld", buffer, tv.tv_usec);
#endif
    
    return;

}



void *
quickConnect (void *ptr)
{


    thdata *data;
    data = (thdata *) ptr;
    getTime (data->time_buffer);

    if (connect
            (data->sockfd, (SA *) & (data->servaddr), sizeof (data->servaddr)) == 0)
    {
        data->status = 1;
    }
    else
    {
        data->status = 0;
        //exit(1);
    }

    return NULL;
}


void
manageInput (int argc, char **argv, char *hname, char *port)
{
    if (argc < 3)
    {
        printf ("Need hostname  port\n");
        printf ("./client 127.0.0.1  10001\n");
        exit (0);
    }
    snprintf (hname, MAXSUB, "%s", (char *) argv[1]);
    snprintf (port, MAXSUB, "%s", (char *) argv[2]);

    return;
}




int
setupConnection (char *hname, char *port, thdata * data)
{
    int sockfd;
    struct sockaddr_in servaddr;

    char **pptr;
    char str[50];
    char ip[50];

    ip[0] = '\0';
    struct hostent *hptr;
    if ((hptr = gethostbyname (hname)) == NULL)
    {
        fprintf (stderr, " gethostbyname error for host: %s: %s\n",
                 hname, hstrerror (h_errno));
        // You do not want to exit. Want to check if IP. If not move on.
        //  exit (1);
        return 1;
    }
    //prTime();

    if (hptr->h_addrtype == AF_INET && (pptr = hptr->h_addr_list) != NULL)
    {
        snprintf (ip, 50, "%s",
                  inet_ntop (hptr->h_addrtype, *pptr, str, sizeof (str)));

    }
    else
    {
        fprintf (stderr, "Error call inet_ntop \n");
    }


    sockfd = socket (AF_INET, SOCK_STREAM, 0);
    bzero (&servaddr, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons (atoi (port));
    inet_pton (AF_INET, str, &servaddr.sin_addr);


    data->thread_no = 1;
    data->sockfd = sockfd;
    data->servaddr = servaddr;
    data->status = -1;
    strcpy (data->hname, hname);
    strcpy (data->ip, ip);
    strcpy (data->port, port);
    //printf("Setup connection port %s  host=%s\n", data->port, data->hname);

    return 0;
}

void
prData (thdata * data)
{

    if (data->status == -1)
    {
        printf ("%s,%s,%s,%s,%d,No Connection,timeout\n",
                data->time_buffer, data->hname, data->ip, data->port,
                data->status);
    }
    if (data->status == 0)
    {
        printf ("%s,%s,%s,%s,%d,No Connection\n",
                data->time_buffer, data->hname, data->ip, data->port,
                data->status);
    }
    if (data->status == 1)
    {
        printf ("%s,%s,%s,%s,%d,Connected,*** GOOD ***\n",
                data->time_buffer, data->hname, data->ip, data->port,
                data->status);
    }
}



/*
 This will need to be cleaned up.



 */
int
process_loop (int argc, char **argv)
{


    pthread_t thread[MAX_WORKER_THREADS];	/* thread variables */
    thdata data[MAX_NUM_THREAD_DATABASE];	/* structs to be passed to
                                             threads */


    //char hname[MAXSUB + 1];
    //char port[MAXSUB + 1];
    int  recovery_test=0;
    int tflag[MAX_WORKER_THREADS + 1];


    int i;

    for (i = 0; i < MAX_WORKER_THREADS; ++i)
        tflag[i] = -2;

    char tp[20 + 1];
    int sig;
    int loops;
    int err;
    int stat_good=0;

    char c[1000];
    //    char ports[1000];
    char **array=NULL;



    Key_val *k = NULL;
    Vec *v = NULL;

    strcpy (c, argv[1]);

    v = parse_hosts (c, array, v, argv[2]);
    k = getK (v, 0);

    loops = v->argc * k->argc;


    if (loops >= MAX_PERMITTED_SCANS) {
        myfreeV (v);
        fprintf(stderr,"Too many scans - try less. Something under (hosts*ports) < %d\n",MAX_PERMITTED_SCANS);
        exit(-1);
    }


    int vi, ki;
    int thread_index;

    i = 0;
    for (vi = 0; vi < (v->argc); ++vi)
        for (ki = 0; ki < (k->argc); ++ki)
        {
            thread_index = i % MAX_WORKER_THREADS;
            k = getK (v, vi);
            snprintf (tp, 20, "%s", getKkey (k, ki));
            //printf("i=%d  getV(v,vi)=%s  tp=%s\n", i, getV(v, vi), tp);
            if ( setupConnection (getV (v, vi), tp, &data[thread_index]) != 0 )
            {
                //printf("Going to continue\n");
                continue;
            }
            err = pthread_create (&thread[thread_index], NULL,
                                  (void *) &quickConnect,
                                  (void *) &data[thread_index]);
            if (err != 0)
            {
                fprintf (stderr, "Issue with pthread_create in process_loop\n");
                tflag[thread_index] = -1;
            }
            else
            {
                tflag[thread_index] = 1;
            }

            /* We need to recover threads */
            //if (i > 0 && ( thread_index  == 0))   //too late
            if (i > 0 && ( (i+1) % MAX_WORKER_THREADS  == 0))   //too late
            {
                recovery_test=1;
                if (data[thread_index].status != 1)
                {
                    // printf ("Yep. We had to sleep 0\n");
                    sleep (1);
                }

                int j = 0;
                for (j = 0; j < MAX_WORKER_THREADS; ++j)
                {
                    if (tflag[j] == 1)
                    {
                        sig = pthread_cancel (thread[j]);
                        if (sig != 0)
                        {
                            tflag[j] = -1;
                        }
                        else
                        {
                            tflag[j] = 0;
                        }
                        if (tflag[j] != -2)
                            sig = pthread_join (thread[j], 0);
                        if (sig != 0)
                        {
                            fprintf (stderr,
                                     "Error thread may have terminated.\n");
                        }
                        else
                        {
                            tflag[j] = -2;
                        }
                        tflag[j] = -2;

                        close (data[j].sockfd);

                        prData (&data[j]);
                    }

                }
            }

            i++;
            //Refactor.  Sloppy with the i
        }

    /* We may have extra (loops % (MAX_WORKER_THREADS) is loops > MAX_ */

    sleep (1);
    int j = 0;
    for (j = 0; j < MAX_WORKER_THREADS; ++j)
    {

        if (tflag[j] == 1)
        {
            sig = pthread_cancel (thread[j]);
            if (sig != 0)
            {
                //fprintf(stderr, "Error thread may have terminated.\n");
                tflag[j] = -1;
            }
            else
            {
                tflag[j] = 0;
            }

            sig = pthread_join (thread[j], 0);
            if (sig != 0)
            {
                //fprintf(stderr, "Error thread may have terminated.\n");
            }
            tflag[j] = -1;
            close (data[j].sockfd);
            prData (&data[j]);
            if(data[j].status == 1)
                ++stat_good;
        }
    }


    myfreeV (v);
    return stat_good;
}


int
main (int argc, char **argv)
{


    if (argc != 3)
    {
        fprintf (stderr,
                 "\n%s\nversion %s\n\nUsage:\n  %s host1,host2  port1,port2,port-port\n\nExample:\n  %s gmail.com,google.com 80,440-444\n\n",
                 "Source: https://github.com/mchirico/ct", _VERSION_,
                 argv[0], argv[0]);
        fprintf(stderr,"\nSteps to get and build latest verson:\n%s\n%s",
                "  curl  https://raw.githubusercontent.com/mchirico/ct/master/src/ct.c  > ct.c",
                "  gcc ct.c -o ct -lpthread\n\n");
        exit (EXIT_FAILURE);
    }

    return process_loop (argc, argv);


}

