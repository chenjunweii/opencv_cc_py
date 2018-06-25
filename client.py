import cv2
import numpy as np
#import threading
#import gevent

# patches stdlib (including socket and ssl modules) to cooperate with other greenlets

import socket               # 导入 socket 模块
#from queue import Queue
import sys
import time

#from multiprocessing import Process, Queue, Lock
from threading import Thread as Process
from threading import Lock
from queue import Queue

#from gevent import monkey
#monkey.patch_all()
#monkey.patch_socket()

def capture(s, q, lock):

    webcam = cv2.VideoCapture('me2.mp4')

    counter = 0

    prev = 0

    current = 0

    while True:

        if counter % 2 == 0:

            ret, frame = webcam.read()

            #print(frame.shape)

            frame = cv2.resize(frame, (256, 256))

            (h, w) = frame.shape[:2]
            
            center = (w/2, h/2)

            M = cv2.getRotationMatrix2D(center, 90, 1.0)
            
            frame = cv2.warpAffine(frame, M, (w, h))
            
            encode = encoder(frame)

            send_size = s.send(encode)

            """
            
            lock.acquire()

            q.put(frame)

            lock.release()

            """

            #cv2.imshow('capture', frame)

            #if cv2.waitKey(1) & 0xFF == ord('q'):
                
            #    break
    
            #"""
        
        counter += 1

def send(s, q, lock):

    while True:

        if q.empty():

            continue

        lock.acquire()
        
        frame = q.get()

        lock.release()

        start = time.time()

        encode = encoder(frame)

        to_send_size = sys.getsizeof(encode)

        actual_send_size = s.send(encode)

        to_send_size -= actual_send_size

    s.close()


def encoder(frame):

    encode = cv2.imencode('.jpg', frame)[1]

    encode = np.array(encode)  
    
    encode = encode.tostring()

    send_size = (str(sys.getsizeof(encode)).rjust(10, '0')).encode()

    size_of_send_size = sys.getsizeof(send_size)
    
    return (send_size + encode)

def decoder(string):
      
    nd = np.fromstring(string, np.uint8)  
    
    decode = cv2.imdecode(nd, cv2.IMREAD_COLOR)

    return decode

def recv(s, q, lock):
 
    size_len = 10
    
    byte_header = sys.getsizeof(bytes())

    while True:

        recv = s.recv(10)

        recv_len = len(recv) # only first time get bytes size, because wa only send one byte header

        size = recv[ : size_len]

        frame_size = int(size.decode())

        frame_len = frame_size - byte_header

        rest_len = frame_len - recv_len + size_len

        rest = s.recv(rest_len)

        recv += rest

        rest_len -= len(rest)

        assert(rest_len >= 0)
        
        while rest_len > 0:

            rest = s.recv(rest_len)

            recv += rest

            rest_len -= len(rest)
                         
        encode = recv[size_len : ]

        lock.acquire()

        q.put(encode)

        lock.release()

def display(post, post_lock):

    while True:

        if post.empty():
            
            time.sleep(0.1)

            continue

        post_lock.acquire()

        encode = post.get()
        
        post_lock.release()
        
        frame = decoder(encode)
        
        cv2.imshow('frame', frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            
            break
    
def main():

    host = 'heidiz.ddns.net'

    port = 38010                # 设置端口好
    
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    s.connect((host, port))

    recv_connection = s.recv(4096)

    print(recv_connection.decode())

    pre = Queue(); post = Queue()

    pre_lock = Lock(); post_lock = Lock()
    #"""
    thread = dict()

    thread['capture'] = Process(target = capture, args = (s, pre, pre_lock))
    
    #thread['send'] = Process(target = send, args = (s, pre, pre_lock))
    
    thread['recv'] = Process(target = recv, args = (s, post, post_lock))

    """
    thread = [None] * 4 
    
    thread[0] = gevent.spawn(capture, s, pre, pre_lock)
    
    thread[1] = gevent.spawn(send, s, pre, pre_lock)
    
    thread[2] = gevent.spawn(recv, s, post, post_lock)
    
    thread[3] = gevent.spawn(display, post, post_lock)

    gevent.joinall(thread)
    """
    #thread['display'] = threading.Thread(target = display, args = (post, post_lock))

    start = [t[1].start() for t in thread.items()]
    
    while True:

        if post.empty():

            time.sleep(1)

            continue

        post_lock.acquire()

        encode = post.get()
        
        post_lock.release()
        
        frame = decoder(encode)
        
        cv2.imshow('frame', frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            
            break
    
    join = [t[1].join() for t in thread.items()]

main()


