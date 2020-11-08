#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
#include <thread>
#include <sys/epoll.h>

#define CLOCKID CLOCK_REALTIME
#define SIG SIGALRM

// global variables for signal handling
HistogramCollection hc;
bool want_file = false;
__int64_t filelength = 1;  // prevent division by 0
__int64_t remainlength = 0;

using namespace std;

void patient_thread_function(int n, int pno, BoundedBuffer* request_buffer){
    datamsg *x = new datamsg(pno, 0.0, 1);
    for (int i = 0; i < n; i++){
        request_buffer->push( (char*) x, sizeof(datamsg) );
        x->seconds +=  0.004;
    }
    delete x;
}

void file_thread_function(string filename,  FIFORequestChannel* chan, int buffercapacity, BoundedBuffer* request_buffer){
    filemsg* f = new filemsg(0, 0);
    int to_alloc = sizeof(filemsg) + filename.length() + 1;
    char* buf = new char[1024];
    memcpy(buf, f, sizeof(filemsg));
    memcpy(buf+sizeof(filemsg), filename.c_str(), filename.length()+1);
    delete f;

    chan->cwrite(buf, to_alloc);
    chan->cread(&filelength, sizeof(__int64_t));

    string pathname = "./received/" + filename;

    filemsg* fm = new filemsg(0, 0);
    int fd = open(pathname.c_str(), O_CREAT|O_TRUNC, S_IRWXU);
    close(fd);
    remainlength = filelength;

    while(remainlength > 0){
        fm->length = min (remainlength, (__int64_t) buffercapacity);
        memcpy(buf, fm, sizeof(filemsg));
        memcpy(buf+sizeof(filemsg), filename.c_str(), filename.length()+1);
        request_buffer->push(buf , to_alloc);
        fm->offset += fm->length;
        remainlength -= fm->length;
    }
    delete fm;
    delete [] buf;
}

void worker_thread_function(FIFORequestChannel* chan, BoundedBuffer* request_buffer, HistogramCollection* hc, int buffercapacity) {
    char* buf = new char[1024];
    char* recvbuf = new char[buffercapacity];
    while (true){
        request_buffer->pop(buf, sizeof(buf));
        MESSAGE_TYPE* m = (MESSAGE_TYPE*) buf;
        if (*m == DATA_MSG){
            datamsg *d = (datamsg*) buf;
            chan->cwrite(d, sizeof(datamsg));
            double result;
            chan->cread(&result, sizeof(double));
            int pno = d->person;
            hc->update(pno, result);
        } else if (*m == QUIT_MSG){
            if (sizeof(MESSAGE_TYPE) > buffercapacity){
                cout << "Buffer Capacity is large not enough." << endl;
                return;
            }
            chan->cwrite(m , sizeof(MESSAGE_TYPE));
            delete chan;
            break;
        } else if (*m == FILE_MSG){
            filemsg *fm = (filemsg*) buf;
            string filename = (char*) (fm + 1);
            int sz = sizeof(filemsg) + filename.size() + 1;
            chan->cwrite(buf, sz);
            chan->cread(recvbuf, buffercapacity);

            //  write to file
            string pathname = "./received/" + filename;
            int fd = open(pathname.c_str(), O_WRONLY, S_IRWXU);
            lseek(fd, fm->offset, SEEK_SET);
            write(fd, recvbuf, fm->length);
            close(fd);
        }
    }
    delete [] buf; 
    delete [] recvbuf;
}


//thread evp(event_polling_function, n, p, w, m, wchans, &request_buffer, &hc);
//void event_polling_function(FIFORequestChannel* chan, BoundedBuffer* request_buffer, HistogramCollection* hc, int buffercapacity) {
void event_polling_function(int n, int p, int w, int m, FIFORequestChannel** wchans, BoundedBuffer* request_buffer, HistogramCollection* hc, int buffercapacity) {
    char* buf = new char[1024];
    char* recvbuf = new char[buffercapacity];
    struct epoll_event ev;
    struct epoll_event events[w];

    // create an epoll list
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        EXITONERROR("epoll_create1");
    }

    unordered_map<int, int> fd_to_index;
    vector<vector<char>> state(w);

    // primming + adding each rfd to the list
    int n_sent = 0;
    int n_recv = 0;
    for (int i=0; i<w; i++) {
        int sz = request_buffer->pop(buf, 1024);
        
        // need a condition to get out if w > p * n for example
        // example quit break (watch Farah video)
        // or set w = max(n*p, w)

        wchans[i]->cwrite(buf, sz);
        state[i] = vector<char>(buf, buf+sz); //record state i
        n_sent++;

        int rfd = wchans[i]->getrfd();
        fcntl(rfd, F_SETFL, O_NONBLOCK);
        // fcntl(wchas[i]->getwrd(), F_SETFL, O_NONBLOCK);

        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = rfd;
        fd_to_index[rfd] = i;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, rfd, &ev) == -1)
            EXITONERROR("epoll_ctl: listen_sock");
    }

    bool quit_recv = false;

    while (true) {
        if (quit_recv && n_recv == n_sent) {
            break; // when receive == send
        }

        int nfds = epoll_wait (epollfd, events, w, -1);
        if (nfds == -1) {
            EXITONERROR("epoll_wait");
        }
        for (int i=0; i<nfds; i++) {
            int wfd = events[i].data.fd; //ready rfd
            int index = fd_to_index[wfd]; // map rfd to index
            n_recv++;
                        
            vector<char> req = state[index];
            char* request = req.data();

            // processing the response
            MESSAGE_TYPE* m = (MESSAGE_TYPE *) request;
            if (*m==DATA_MSG) {
                double result;
                int resp_sz = wchans[index]->cread(&result, sizeof(double));
                if (resp_sz ==0) {
                    quit_recv = true;
                }
                hc->update(((datamsg *) request)->person, (double) result);
            }
            else if (*m == FILE_MSG) {
                
                // implement file MSG
                filemsg* fm = (filemsg*) request;
                

                string filename = (char*) (fm + 1);
                int sz = sizeof(filemsg) + filename.size() + 1;
                
                int resp_sz =  wchans[index]->cread(recvbuf, buffercapacity);
                if (resp_sz ==0) {
                    quit_recv = true;
                }
                //  write to file
                string pathname = "./received/" + filename;
                int fd = open(pathname.c_str(), O_WRONLY, S_IRWXU);
                lseek(fd, fm->offset, SEEK_SET);
                write(fd, recvbuf, fm->length);
                close(fd);
            }

            if (!quit_recv) {
                int req_sz = request_buffer->pop (buf, sizeof(buf));
                if (*(MESSAGE_TYPE*) buf == QUIT_MSG) {
                    quit_recv = true;
                }
                else {
                    wchans[index]->cwrite(buf, req_sz);
                    state[index] = vector<char> (buf, buf+req_sz);
                    n_sent++;
                }
            }
        }
    }
    delete [] buf; 
    delete [] recvbuf;
    return;
}


FIFORequestChannel* create_new_channel(FIFORequestChannel* chan){
    MESSAGE_TYPE nc = NEWCHANNEL_MSG;
    chan->cwrite(&nc, sizeof(MESSAGE_TYPE));

    char nch_name[30];
    chan->cread(nch_name, 30);
    FIFORequestChannel* new_chan = new FIFORequestChannel(nch_name, FIFORequestChannel::CLIENT_SIDE);
    return new_chan;
}

void sigalrm_handler(int signo, siginfo_t *si, void *uc){
    //system("clear");
    if (!want_file){
        //hc.print();
        //cout << endl;
    }
    else{
        cout << "WRITING TO FILE---> " << (double) (filelength - remainlength) / (double) filelength * 100.0  << "% COMPLETED."<< endl;
    }
}
int main(int argc, char *argv[])
{
    int n = 1000;    //default number of requests per "patient"
    int p = 10;     // number of patients [1,15]
    int w = 50;    //default number of worker threads
    int b = 20; 	// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the message buffer
	string filename = "";
    srand(time_t(NULL));
    int opt = -1;
    while ((opt = getopt(argc, argv, "n:p:w:b:m:f:")) != -1) {
        switch (opt) {
            case 'n':
                n = atoi(optarg);
                break;
            case 'p':
                p = atoi (optarg);
                break;
            case 'w':
                w = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 'm':
                m = atoi( optarg );
                break;
            case 'f':
                filename = optarg;
                want_file = true;
                break;
            case '?':
                cout << "ERROR\n";
                break;
        }
    }

    // Check buffercapacity, ensure that all types of requests are able to pass through
    int to_check = max (sizeof(NEWCHANNEL_MSG), sizeof(QUIT_MSG));
    to_check = max ((int) sizeof(datamsg), to_check);
    if (want_file){
        to_check = max ((int) (sizeof(filemsg) + filename.length() + 1), to_check);
    }
    if (to_check > m) {
        cout << "Buffer Capacity is large not enough." << endl;
        exit(0);
    }

    // **** BONUS CREDIT
    // establish handler
    struct sigaction act;
    act.sa_sigaction = sigalrm_handler;
    act.sa_flags = SA_SIGINFO;
    if (sigaction(SIG, &act, NULL) != 0){
        perror("SIGACTION() FAILED");
        exit(0);
    }

    // create timer
    struct sigevent sev;
    timer_t timerid;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIG;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCKID, NULL, &timerid) != 0){
        perror("TIMER_CREATE() FAILED");
        exit(0);
    }

    // set timer
    struct itimerspec its;
    its.it_interval.tv_sec = 2;  //  it_interval specifies period between successive timer expirations
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 2;  //  it_value indicates time left to the next time expiration
    its.it_value.tv_nsec = 0;

    if (timer_settime(timerid, 0, &its, NULL) != 0){
        perror("TIMER_SETTIME() FAILED");
        exit(0);
    }

    // server as child process
    int pid = fork();
    if (pid == 0) {
        if (execl("server", "server", "-m", to_string(m).c_str(), NULL)){
            perror("EXECL() FAILED");
            exit(0);
        }
    }

	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
    // Only add histogram to histogram collection if user requests data
    if (!want_file) {
        for (int i = 0; i < p; i++) {
            Histogram *h = new Histogram(10, -2.0, 2.0);
            hc.add(h);
            w = min(p*n, w);
        }
    }
    

	FIFORequestChannel** wchans = new FIFORequestChannel* [w];
	for (int i = 0; i < w; i++){
	    wchans[i] = create_new_channel(chan);
	}

    struct timeval start, end;
    gettimeofday (&start, 0);

    /* Start all threads here */
    thread patients[p];
    thread file_thread;
    if (!want_file){
        for (int i = 0; i < p; i++){
            patients[i] = thread(patient_thread_function, n, i+1, &request_buffer);
        }
    } else {
        file_thread = thread(file_thread_function, filename, chan, m, &request_buffer);
    }

    // thread evp(event_polling_function, n, p, w, m, wchans, &request_buffer, &hc);
    thread evp(event_polling_function, n, p, w, m, wchans, &request_buffer, &hc, b);
    //this is new code
	
    /* Join all threads here */
	if (!want_file) {
        for (int i = 0; i < p; i++) {
            patients[i].join();
        }
	} else {
	    file_thread.join();
	}
    cout << "Producers are finished" << endl;


    MESSAGE_TYPE q = QUIT_MSG;
    request_buffer.push((char*) &q, sizeof(q));
    evp.join();

    for (int i=0; i<w; i++) {
        wchans[i]->cwrite(&q, sizeof(MESSAGE_TYPE));
        delete wchans[i];
    }
    delete[] wchans;

    cout << "Consumers are finished" << endl;

    //when all jobs are done, delete timer
    timer_delete(timerid);

    gettimeofday (&end, 0);

    // print the results
    if (!want_file){
        hc.print ();
    }
    else{
        cout << "Finish writing into received/" << filename << endl;
    }
    
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    cout << "All Done!!!" << endl;
    chan->cwrite(&q, sizeof(MESSAGE_TYPE));
    delete chan;
}
