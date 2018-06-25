#include <Python.h>
#include <opencv2/opencv.hpp>
#include <string.h>
#include <stdio.h>
#include "numpy/arrayobject.h"
#include "numpy/ufuncobject.h"
#include "core/core.hpp"
#include "ios"
#include "new"
#include "stdexcept"
#include "typeinfo"
#include "opencv_mat.h"

using namespace std;
using namespace cv;

Mat process(PyObject * net, Mat frame){

    PyArrayObject * np = __pyx_f_10opencv_mat_Mat2np(frame);

	PyArrayObject * process_np = TRANSFORM(net, np, 0);

	//PyArrayObject * process_np = flip(np, 0);
	
    Mat process_frame = __pyx_f_10opencv_mat_np2Mat(process_np);

	return process_frame;

}

void init_python(){

	PyImport_AppendInittab("opencv_mat", PyInit_opencv_mat);
	
	Py_Initialize();
	
	PyImport_ImportModule("opencv_mat");
}

PyObject * init_tensorflow(){

	auto stream = INIT_TENSORFLOW(0);

	return stream;
}


