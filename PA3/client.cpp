#include "common.h"
#include <sys/wait.h>
#include "FIFOreqchannel.h"
#include "MQreqchannel.h"
#include "SHMreqchannel.h"
#include <fstream>
#include <iostream>

using namespace std;


int main(int argc, char *argv[]){
    
    int c;
    int buffercap = MAX_MESSAGE;
    int p = 0, ecg = 1;
    double t = -1.0;
    bool isnewchan = false;
    bool isfiletransfer = false;
    string filename;
    string ipc_method = "f";
    int n_channels = 1;

    while ((c = getopt (argc, argv, "p:t:e:m:f:c:i:")) != -1){
        switch (c){
            case 'p':
                p = atoi (optarg);
                break;
            case 't':
                t = atof (optarg);
                break;
            case 'e':
                ecg = atoi (optarg);
                break;
            case 'm':
                buffercap = atoi (optarg);
                break;
            case 'c':
                isnewchan = true;
                n_channels = atoi(optarg);
                break;
            case 'f':
                isfiletransfer = true;
                filename = optarg;
                break;
            case 'i':
                ipc_method = optarg;
                break;
        }
    }
            
            
            
    // RequestChannel* control_chan = NULL;
    // if (ipc_method == "f") {
    //     control_chan = new FIFORequestChannel ("control", RequestChannel::CLIENT_SIDE);
    // }
    // else if (ipc_method == "q") {
    //     control_chan = new MQRequestChannel("control", RequestChannel::CLIENT_SIDE);
    // }
    // else if (ipc_method == "m") {
    //     control_chan = new SHMRequestChannel("control", RequestChannel::CLIENT_SIDE, buffercap);
    // }
// for (int ii = 0; ii < n_channels; ii++) {
// fork part
    
    
    if (fork()==0){ // child 
        char* args [] = {"./server", "-m", (char *) to_string(buffercap).c_str(), "-i", (char *) ipc_method.c_str(), NULL};
        if (execvp (args [0], args) < 0){
            perror ("exec failed");
            exit (0);
        }
    }
    vector<RequestChannel*> store_channels(n_channels, nullptr);
    // vector<RequestChannel*> store_control(n_channels, nullptr);
    RequestChannel* control_chan = NULL;
    if (ipc_method == "f") {
        control_chan = new FIFORequestChannel ("control", RequestChannel::CLIENT_SIDE);
    }
    else if (ipc_method == "q") {
        control_chan = new MQRequestChannel("control", RequestChannel::CLIENT_SIDE);
    }
    else if (ipc_method == "m") {
        control_chan = new SHMRequestChannel("control", RequestChannel::CLIENT_SIDE, buffercap);
    }
    
    string outfilepath = string("received/") + filename;
    FILE* outfile = fopen (outfilepath.c_str(), "wb");

    for (int ii=0; ii<n_channels; ii++) {
        RequestChannel* chan = control_chan;
        if (isnewchan){
            cout << "Using the new channel everything following" << endl;
            MESSAGE_TYPE m = NEWCHANNEL_MSG;
            control_chan->cwrite (&m, sizeof (m));
            char newchanname [100];
            control_chan->cread (newchanname, sizeof (newchanname));
            if (ipc_method == "f") {
                chan = new FIFORequestChannel (newchanname, RequestChannel::CLIENT_SIDE);
            }
            else if (ipc_method == "q") {
                chan = new MQRequestChannel (newchanname, RequestChannel::CLIENT_SIDE);
            }
            else if (ipc_method == "m") {
                chan = new SHMRequestChannel(newchanname, RequestChannel::CLIENT_SIDE, buffercap);
            }

            cout << "New channel by the name " << newchanname << " is created" << endl;
            cout << "All further communication will happen through it instead of the main channel" << endl;
        }
        store_channels[ii] = chan;
        // store_control[ii] = control_chan;

        if (!isfiletransfer){   // requesting data msgs
            if (t >= 0){    // 1 data point
                datamsg d (p, t, ecg);
                chan->cwrite (&d, sizeof (d));
                double ecgvalue;
                chan->cread (&ecgvalue, sizeof (double));
                cout << "Ecg " << ecg << " value for patient "<< p << " at time " << t << " is: " << ecgvalue << endl;
            } else {          // bulk (i.e., 1K) data requests 
                int start_point = round((1000*1.0)/n_channels*ii);
                int end_point = round((1000*1.0)/n_channels*(ii+1));
                double ts = start_point*0.004;  
                datamsg d (p, ts, ecg);
                for (int i = start_point; i < end_point; i++) {  //edit here
                    double ecgvalue;
                    chan->cwrite (&d, sizeof (d));
                    chan->cread (&ecgvalue, sizeof (double));
                    cout << d.seconds << "  -->  " << ecgvalue << endl;
                    d.seconds += 0.004; //increment the timestamp by 4ms
                }
            }
        }
        else if (isfiletransfer){
            filemsg f (0,0);  // special first message to get file size
            int to_alloc = sizeof (filemsg) + filename.size() + 1; // extra byte for NULL
            char* buf = new char [to_alloc];
            memcpy (buf, &f, sizeof(filemsg));
            memcpy (buf + sizeof(filemsg), filename.c_str(), filename.size()+1);
            strcpy (buf + sizeof (filemsg), filename.c_str());
            chan->cwrite (buf, to_alloc);
            __int64_t filesize;
            chan->cread (&filesize, sizeof (__int64_t));
            cout << "File size: " << filesize << endl;

            //int transfers = ceil (1.0 * filesize / MAX_MESSAGE);
            filemsg* fm = (filemsg*) buf;
            
            int start_point = round((filesize*1.0)/n_channels*ii);
            int end_point = round((filesize*1.0)/n_channels*(ii+1));
            __int64_t rem = end_point - start_point;

            
            fm->offset = start_point;
            
            char* recv_buffer = new char [MAX_MESSAGE];
            while (rem > 0){
                fm->length = (int) min (rem, (__int64_t) MAX_MESSAGE);
                chan->cwrite (buf, to_alloc);
                chan->cread (recv_buffer, MAX_MESSAGE);
                fwrite (recv_buffer, 1, fm->length, outfile);
                rem -= fm->length;
                fm->offset += fm->length;
            }
            
            delete recv_buffer;
            delete buf;
            cout << "File transfer completed" << endl;
        }
    }
    fclose (outfile);
    //wait(0);
    MESSAGE_TYPE q = QUIT_MSG;
    for (int ii = 0; ii < n_channels; ii++) {
        store_channels[ii]->cwrite(&q,sizeof(MESSAGE_TYPE));
        delete store_channels[ii];
    }
    store_channels.clear();
    
    control_chan->cwrite(&q,sizeof(MESSAGE_TYPE));
    delete control_chan;
    wait(0);
    
// wait for the child process running server
// this will allow the server to properly do clean up
// if wait is not used, the server may sometimes crash
      
}