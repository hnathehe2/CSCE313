#ifndef BoundedBuffer_h
#define BoundedBuffer_h

#include <stdio.h>
#include <queue>
#include <string>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
using namespace std;

class BoundedBuffer
{
private:
	int cap; // max number of items in the buffer
	queue<vector<char>> q;	/* the queue of items in the buffer. Note
	that each item a sequence of characters that is best represented by a vector<char> for 2 reasons:
	1. An STL std::string cannot keep binary/non-printables
	2. The other alternative is keeping a char* for the sequence and an integer length (i.e., the items can be of variable length).
	While this would work, it is clearly more tedious */

	// add necessary synchronization variables and data structures 
    mutex m;
    condition_variable data_available;
    condition_variable slot_available;

public:
	BoundedBuffer(int _cap){
        cap = _cap;
	}
	~BoundedBuffer(){

	}

	void push(char* data, int len){
		//1. Wait until there is room in the queue (i.e., queue lengh is less than cap)
		//2. Convert the incoming byte sequence given by data and len into a vector<char>
		//3. Then push the vector at the end of the queue

		unique_lock<mutex> ul (m);
        slot_available.wait(ul, [this] {return q.size()< cap; });

		vector<char> data_to_push(data, data+len);
		q.push(data_to_push);

		ul.unlock();
		data_available.notify_one();
	}

	int pop(char* buf, int bufcap){
		//1. Wait until the queue has at least 1 item
		//2. pop the front item of the queue. The popped item is a vector<char>
		//3. Convert the popped vector<char> into a char*, copy that into buf, make sure that vector<char>'s length is <= bufcap
		//4. Return the vector's length to the caller so that he knows many bytes were popped

		unique_lock<mutex> ul (m);
        data_available.wait(ul, [this] {return q.size() > 0;});

        vector<char> data_read = q.front();
        q.pop();
        ul.unlock();

        memcpy(buf, data_read.data(), data_read.size());
        slot_available.notify_one();

        return data_read.size();
	}
};

#endif /* BoundedBuffer_ */
