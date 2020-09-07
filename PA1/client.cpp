/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <fstream>
#include <iostream>
#include <sys/wait.h>
using namespace std;



int main(int argc, char *argv[]){
    int buffercapacity = MAX_MESSAGE;
    int opt; 

    int execute_option = 0;
    /* 
        0 no execution
        1 for data
        2 for file
        3 for 1000 data
        4 for create new channel
    */
    int identity = fork(), status;
    if (identity == 0) {
        cout << identity << " parent call client" << endl;
        FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
    
        int person_no; // for patient number
        double time_request; //patient time request
        int ecg_chan; //1 or 2, ecg channel
        string file_name; //for file requesting

        while((opt = getopt(argc, argv, "cp:t:e:f:m:")) != -1)  
        {  
            switch(opt)  
            {
                case 'p':
                    if (execute_option == 0)
                        execute_option = 3;
                    person_no = atoi(optarg);
                    break;
                case 't':
                    time_request = stod(optarg);
                    execute_option = 1;
                    break;
                case 'e':
                    ecg_chan = atoi(optarg);
                    break;    
                case 'f':
                    execute_option = 2;
                    file_name = optarg;
                    break;
                case 'm':
                    buffercapacity = atoi(optarg);
                    break;
                case 'c':
                    execute_option = 4;
                    break;
                case ':':  
                    cout << "option needs a value\n" << endl;  
                    break;  
                case '?':  
                    cout << "unknown option: " << optopt << endl; 
                    break;  
            }  
        }

        if (execute_option == 1) {
            //sending data message
            datamsg d(person_no, time_request, ecg_chan);
            chan.cwrite(&d, sizeof (datamsg));

            //receiving data message
            double result;
            chan.cread(&result, sizeof(double));
            cout << "Result is: " << result << endl;
        }
        else if (execute_option == 2) {

            double time_request = 0.00;
            struct timeval start, end;
            gettimeofday(&start, NULL);

            //sending file message
            int size_total = sizeof(filemsg) + file_name.size() + 1;
            filemsg f(0, 0);
            char* buf = new char [size_total]; //need 1 extra offbyte
            memcpy(buf, &f, sizeof(filemsg));
            memcpy(buf + sizeof(filemsg), file_name.c_str(), file_name.size()+1);
            //can also do this with strcpy
            // strcpy (buf + sizeof (filemsg), filename.c_str());
            chan.cwrite(buf, sizeof(filemsg) + file_name.size() + 1);
            
            //receiving file message for file size
            __int64_t filelen;
            chan.cread(&filelen, sizeof(__int64_t));

            // open file
            string output_filepath = string("received/") + file_name;
            FILE* fp = fopen(output_filepath.c_str(), "wb");
            __int64_t off_set = 0;
            
            while (off_set < filelen) {
                filemsg* fm = (filemsg*) buf;
                fm->offset = off_set;
                int buf_length;
                if (filelen - off_set < buffercapacity) {   
                    buf_length = filelen-off_set;
                }
                else {
                    buf_length = buffercapacity;    
                }
                fm->length = buf_length;
                
                off_set += buffercapacity;

                chan.cwrite (buf, size_total);
                char ret_buf [buf_length];
                chan.cread(ret_buf, sizeof(ret_buf));
                fwrite(ret_buf, 1, buf_length, fp);
            }
            fclose(fp);

            gettimeofday(&end, NULL); 
            double time_taken; 
            time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
            time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6; 
            cout << "Time taken by program is : " << fixed << time_taken << setprecision(6); 
            cout << " sec" << endl;
        }
        else if (execute_option == 3) {
            
            double store[1000];

            double time_request = 0.00;
            struct timeval start, end;
            gettimeofday(&start, NULL); 

            for (int i=0; i<1000; i++) {
                datamsg d(person_no, time_request, ecg_chan);
                chan.cwrite(&d, sizeof (datamsg));
                time_request += 0.004;
                chan.cread(&store[i], sizeof(double));
            }

            gettimeofday(&end, NULL); 
            double time_taken; 
            time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
            time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6; 
            cout << "Time taken by program is : " << fixed << time_taken << setprecision(6); 
            cout << " sec" << endl;
            
            string output_filepath = string("received/") + "first_1000_patient" + to_string(person_no)+ "_" + to_string(ecg_chan) + ".csv";
            ofstream fp(output_filepath);
            for (auto i:store)
                fp << to_string(i) << "\n";
            fp.close();
        }
        else if (execute_option == 4) {
            MESSAGE_TYPE new_channel = NEWCHANNEL_MSG;
            chan.cwrite(&new_channel, sizeof(MESSAGE_TYPE));
            char channel_name[100];
            chan.cread(channel_name, 100);

            FIFORequestChannel chan_new(channel_name, FIFORequestChannel::CLIENT_SIDE);

            datamsg d(15, 0.008, 1);
            chan_new.cwrite(&d, sizeof(datamsg));
            
            double result;
            chan_new.cread(&result, sizeof(double));
            cout << " Patient 15 Time 0.008 ECG 1 is " << result << endl;

            MESSAGE_TYPE m = QUIT_MSG;
            chan_new.cwrite (&m, sizeof (MESSAGE_TYPE));
        }

        // closing the channel    
        MESSAGE_TYPE m = QUIT_MSG;
        chan.cwrite (&m, sizeof (MESSAGE_TYPE));
    }
    else {
        char* arg[argc-1];
        for (int i=1; i<argc; i++) {
            arg[i-1] = argv[i];
        }
        cout << identity << " child call server" << endl;
        execv("./server", arg);
        cout << "create server success" << endl;
        wait(&status);
    }

    return 0;
}
