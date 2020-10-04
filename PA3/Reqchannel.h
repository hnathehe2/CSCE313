
#ifndef _Reqchannel_H_
#define _Reqchannel_H_

#include "common.h"

class RequestChannel
{
public:
	enum Side {SERVER_SIDE, CLIENT_SIDE};
	enum Mode {READ_MODE, WRITE_MODE};
	
protected:
	// common for all channels
	string my_name;
	Side my_side;
	int wfd;
	int rfd;
	string stream1, stream2;
	virtual int open_ipc (string _pipe_name, int mode){;}
	
public:
	RequestChannel(const string _name, const Side _side): my_name(_name), my_side(_side){}
	
	virtual ~RequestChannel(){}
	
	virtual int cread (void* msgbuf, int bufcapacity) = 0;
	
	virtual int cwrite(void *msgbuf , int msglen) = 0;
	
	string name() {
		return my_name;
	}
};

#endif
