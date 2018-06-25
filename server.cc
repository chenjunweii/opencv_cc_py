#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <mutex>
#include <queue>

#include <Python.h>
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <boost/python.hpp>
#include <boost/python/list.hpp>

#include "process.hh"

using namespace std;
using namespace cv;

int t_recv(int s, queue <Mat> * q, mutex * m){ 

    int status = 0;
	
	int rest = 0;
		
	int buf = 2048;

	int current = 0;

	Mat frame(256, 256, CV_8UC3);
    
    while(true){

		char size_c[5];

		status = recv(s, &size_c, 5, 0);

        if(status != 5)

            cout << "[!] Error when recving data => " << status << endl;

		int total = stoi(string(size_c));

		rest = total;

        vector <uchar> encode (total);
		
		while (rest > 0){

			if (rest > buf)

				current = recv(s, &encode[total - rest], buf, 0);

			else

				current = recv(s, &encode[total - rest], rest, 0);

			if (current < 0)

				cout << "[!] Receive Error" << endl;

			else

				rest -= current;
			
		}

        frame = imdecode(Mat(encode), 1);    
		
        m->lock();

        q->push(frame);

        m->unlock();
    }   

}

int t_send(int s, queue <Mat> * q, mutex * m){

	while(true){
        
		if(q->empty())

            continue;

		cout << "[*] Sending Back" << endl;

        vector <uchar> encode;

        m->lock();

        Mat frame = q->front();

        q->pop();

        m->unlock();

        imencode(".jpg", frame, encode);

        string codes = to_string(encode.size());

		if (codes.size() < 5){

             int pad = 5 - codes.size();

             char c = '0';

             codes.insert(0, pad, c);

         }

		assert(codes.size() == 5);


        send(s, codes.data(), codes.size(), 0);

        int send_size = send(s, encode.data(), encode.size(), 0);

		if(send_size != encode.size()){

			cout << "[!] Send Size Error .." << endl;

			break;

		}

    }
	
}


int main(int argc , char *argv[]){

	init_python();

	auto stream = init_tensorflow();
    
	char inputBuffer[256] = {};
   
	char message[] = {"Hi,this is server.\n"};
    
	int sockfd = 0, client = 0;
    
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	const int reuse = 1;

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
   		
		printf("setsockopt(SO_REUSEADDR) failed");
    
	if (sockfd == -1){
    
		printf("Fail to create a socket.");
    }

    struct sockaddr_in serverInfo, clientInfo;
    
	socklen_t addrlen = sizeof(clientInfo);
    
	bzero(&serverInfo,sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    
	serverInfo.sin_addr.s_addr = INADDR_ANY;
    
	serverInfo.sin_port = htons(38010);
    
	bind(sockfd, (struct sockaddr *) &serverInfo, sizeof(serverInfo));
    
	listen(sockfd, 5);

	cout << "[!] Server is Ready" << endl;
	
	client = accept(sockfd, (struct sockaddr*) &clientInfo, &addrlen);

	cout << "[*] Client is connected ... " << endl;

	send(client, message, sizeof(message),0);
        
	recv(client, inputBuffer, sizeof(inputBuffer),0);
    
	vector <thread> ts;

	queue <Mat> qoriginal, qprocess;

	mutex moriginal, mprocess;

	ts.emplace_back(thread(t_recv, client, & qoriginal, & moriginal));
	
	ts.emplace_back(thread(t_send, client, & qprocess, & mprocess));

		
	while(true){

		//cout << "q size : " << qoriginal.size() << endl;

		if(qoriginal.empty()){

			continue;
		
		}

		//cout << "[!] Processing" << endl;

		moriginal.lock();

		Mat frame = qoriginal.front();

		qoriginal.pop();

		moriginal.unlock();

		//Mat gray(256, 256, CV_8UC3);

		//cvtColor(frame, frame, CV_BGR2GRAY);
		frame = process(stream, frame);

		mprocess.lock();

		qprocess.push(frame);

		mprocess.unlock();

	}

	Py_Finalize();

	for (auto &t : ts)

		t.join();
        
	return 0;
}
