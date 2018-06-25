#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <unistd.h>

using namespace cv;
using namespace std;


int t_capture(queue <Mat> * q, mutex * m){
	
	VideoCapture capture(0);

	if(!capture.isOpened()) return -1;

    Mat frame;

	int counter = 0;
    
	while(true){

		counter += 1;

		if (counter % 2 == 0)

			continue;
        
		capture >> frame;

		if(frame.empty()){
			
			cout << "empty" << endl;
		}
		
		resize(frame, frame, Size(256, 256));
		//imshow("frame", frame);
		//
		m->lock();
		//
		q->push(frame);

		m->unlock();

		//if(waitKey(30) >= 0) break;
		//

    }
    
	return 0;

}
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

                cout << "this size < 0 " << endl;

            else

                rest -= current;

        }

        frame = imdecode(Mat(encode), 1);    
        
        m->lock();

        q->emplace(frame);

        m->unlock();
    }   

}

int t_send(int s, queue <Mat> * q, mutex * m){
	
	while(true){

		if(q->empty()){
			
			//cout << "client send empty" << endl;

			//unsigned int microseconds = 1;
			
			//usleep(microseconds);
			
			continue;

		}

		else{
			
			//cout << "client send not empty " << endl;
		}
			
		vector <uchar> encode;

		m->lock();

		Mat frame = q->front();

		q->pop();

		m->unlock();
		
		imencode(".jpg", frame, encode);

		//cout << encode.size() << endl;

		string codes = to_string(encode.size());

		if (codes.size() < 5){

			int pad = 5 - codes.size();

			char c = '0';

			codes.insert(0, pad, c);

		}

		assert(codes.size() == 5);

		send(s, codes.data(), codes.size(), 0);

		int send_size = send(s, encode.data(), encode.size(), 0);

		//cout << "encode size : " << encode.size() << endl;

		//cout << "send size : " << send_size << endl;

	}

}

int main(){

	int sockfd = 0;
    
	sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1)
    
		printf("Fail to create a socket.");
    
	struct sockaddr_in info;
    
	bzero(&info,sizeof(info));
    
	info.sin_family = PF_INET;
    
	info.sin_addr.s_addr = inet_addr("118.166.70.114");
    
	info.sin_port = htons(38010);
	
	int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    
	if(err == -1)
		
		printf("Connection error");

    char message[] = {"Hi there"};
    
	char receiveMessage[100] = {};

	send(sockfd, message, sizeof(message), 0);
	
	recv(sockfd, receiveMessage, sizeof(receiveMessage), 0);

	cout << receiveMessage << endl;
	
  	vector <thread> ts;

	queue <Mat> qoriginal, qprocess;

	mutex loriginal, lprocess;
	
	ts.emplace_back(thread(t_capture, & qoriginal, & loriginal));

	ts.emplace_back(thread(t_send, sockfd, & qoriginal, & loriginal));
	
	ts.emplace_back(thread(t_recv, sockfd, & qprocess, & lprocess));

	while(true){

		//cout << "display" << endl;

		if(qprocess.empty())

			continue;

		cout << "dsawa" << endl;

		lprocess.lock();

		Mat frame = qprocess.front();

		qprocess.pop();

		lprocess.unlock();

		imshow("Capture", frame);

		waitKey(5);

	}
	
	for(auto &t : ts)
		
		t.join();

    printf("%s",receiveMessage);
    
	printf("close Socket\n");
    
	close(sockfd);
    
	return 0;
}



