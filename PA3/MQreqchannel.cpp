#include "common.h"
#include "MQreqchannel.h"
#include <mqueue.h>
using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/



MQRequestChannel::MQRequestChannel(const string _name, const Side _side) : RequestChannel (_name, _side){
	stream1 = "/message_queue_" + my_name + "1";
	stream2 = "/message_queue_" + my_name + "2";
		
	if (_side == SERVER_SIDE){
		wfd = open_ipc(stream1, O_RDWR | O_CREAT);
		rfd = open_ipc(stream2, O_RDWR | O_CREAT);
	}
	else{
		rfd = open_ipc(stream1, O_RDWR | O_CREAT);
		wfd = open_ipc(stream2, O_RDWR | O_CREAT);
	}
	
}

MQRequestChannel::~MQRequestChannel(){ 
	mq_close(wfd);
	mq_close(rfd);

	mq_unlink(stream1.c_str());
	mq_unlink(stream2.c_str());
}

int MQRequestChannel::open_ipc(string _pipe_name, int mode) {
	int fd = (int) mq_open(_pipe_name.c_str(), O_RDWR | O_CREAT, 0600, 0);
	if (fd < 0){
		EXITONERROR(_pipe_name);
	}
	return fd;
}

int MQRequestChannel::cread(void* msgbuf, int bufcapacity){
	return mq_receive(rfd, (char *) msgbuf, 8192, NULL); // NULL will be replaced with msg priority
	// hard code bufcapacity is 8KB
}

int MQRequestChannel::cwrite(void* msgbuf, int len){ 
	return mq_send(wfd,(char *) msgbuf, len, NULL); // NULL will be replaced with msg priority
}

