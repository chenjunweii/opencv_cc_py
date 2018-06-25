import numpy as np
cimport numpy as np  # for np.ndarray
from libc.string cimport memcpy
from libc.stdio cimport printf
from opencv_mat cimport *
import cv2
import time
# inspired and adapted from http://makerwannabe.blogspot.ch/2013/09/calling-opencv-functions-via-cython.html

cdef public Mat np2Mat3D(np.ndarray ary):
    assert ary.ndim==3 and ary.shape[2]==3, "ASSERT::3channel RGB only!!"
    ary = np.dstack((ary[...,2], ary[...,1], ary[...,0])) #RGB -> BGR

    cdef np.ndarray[np.uint8_t, ndim=3, mode ='c'] np_buff = np.ascontiguousarray(ary, dtype=np.uint8)
    cdef unsigned int* im_buff = <unsigned int*> np_buff.data
    cdef int r = ary.shape[0]
    cdef int c = ary.shape[1]
    cdef Mat m
    m.create(r, c, CV_8UC3)
    memcpy(m.data, im_buff, r*c*3)
    return m

cdef public Mat np2Mat2D(np.ndarray ary):
    assert ary.ndim==2 , "ASSERT::1 channel grayscale only!!"

    cdef np.ndarray[np.uint8_t, ndim=2, mode ='c'] np_buff = np.ascontiguousarray(ary, dtype=np.uint8)
    cdef unsigned int* im_buff = <unsigned int*> np_buff.data
    cdef int r = ary.shape[0]
    cdef int c = ary.shape[1]
    cdef Mat m
    m.create(r, c, CV_8UC1)
    memcpy(m.data, im_buff, r*c)
    return m


cdef public Mat np2Mat(np.ndarray ary):
    if ary.ndim == 2:
        return np2Mat2D(ary)
    elif ary.ndim == 3:
        return np2Mat3D(ary)


cdef public np.ndarray Mat2np(Mat m):
    # Create buffer to transfer data from m.data
    # Define the size / len of data
    cdef size_t len = m.rows * m.cols * m.channels() * sizeof(CV_8UC3)
    cdef Py_buffer buf_info
    # Fill buffer
    PyBuffer_FillInfo(&buf_info, NULL, m.data, len, 1, PyBUF_FULL_RO)
    # Get Pyobject from buffer data
    Pydata = PyMemoryView_FromBuffer(&buf_info)
    # Create ndarray with data
    shape_array = (m.rows, m.cols, m.channels())
    ary = np.ndarray(shape=shape_array, buffer = Pydata, order = 'c', dtype = np.uint8)
    # BGR -> RGB
    ary = np.dstack((ary[...,2], ary[...,1], ary[...,0]))
    # Convert to numpy array
    pyarr = np.asarray(ary)
    return pyarr


import sys
sys.path.insert(0, 'fast-style-transfer/')
sys.path.insert(0, 'fast-style-transfer/src')
from evaluate import ffwd_stream
import cv2
import tensorflow as tf
import numpy as np
import os


cpdef public object INIT_TENSORFLOW():

    os.environ["CUDA_VISIBLE_DEVICES"] = "0"

    g = tf.Graph()
      
    soft_config = tf.ConfigProto(allow_soft_placement=True) 

    soft_config.gpu_options.allow_growth = True

    sess = tf.Session(config=tf.ConfigProto(allow_soft_placement=True,
                                        log_device_placement=True))

    stream = ffwd_stream(sess, 'fast-style-transfer/checkpoint/la_muse.ckpt')

    return stream

cpdef public np.ndarray TRANSFORM(object stream, np.ndarray frame):
    
    #frame = cv2.resize(frame, (128, 128))

    #RGB = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

    RGB = np.expand_dims(frame, 0)

    #start = time.now()
    
    stylized = np.clip(stream.transform(RGB), 0, 255).astype(np.uint8)

    #print('elapse : ', time.now() - start)

    #BGR = cv2.cvtColor(stylized[0], cv2.COLOR_RGB2BGR)
    
    #stylized = cv2.resize(stylized[0], (256, 256))

    return stylized[0]

cpdef public np.ndarray flip(np.ndarray frame):

    return cv2.flip(frame, 1)

def np2Mat2np(nparray):
    
    cdef Mat m
    # Convert numpy array to cv::Mat
    m = np2Mat(nparray)
    # Convert cv::Mat to numpy array
    pyarr = Mat2np(m)
    return pyarr


cdef class PyMat:
    cdef Mat mat

    def __cinit__(self, np_mat):
        self.mat = np2Mat(np_mat)

    def get_mat(self):
        return Mat2np(self.mat)
