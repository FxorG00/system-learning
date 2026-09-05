import numpy as np
import math

def descirbe(name,array):
    print(name)
    print(array.shape)
    print(array.ndim)
    print(array.size)
    print(array.dtype)
    print(array.itemsize)
    print(array.nbytes)

def test1():
    array_1D=np.array([1,2,3,4,5,6],dtype=np.int32)
    array_2D=np.arange(6,dtype=np.float32).reshape(2,3)
    array_3D=np.arange(0,24,dtype=np.float32).reshape(2,3,4)
    descirbe("array 1D",array_1D)
    descirbe("array 2D",array_2D)
    descirbe("array 3D",array_3D)

def test2():
    array_1D=np.array([1,2,3,4,5,6],dtype=np.int32)
    assert array_1D.ndim==len(array_1D.shape)
    assert array_1D.size==math.prod(array_1D.shape)
    assert array_1D.nbytes==array_1D.size*array_1D.itemsize
    
def test3():
    array=np.arange(0,12,1)
    print(array.shape)
    error_seen=False
    try:
        array=array.reshape(5,3)
    except ValueError:
        error_seen=True
    assert error_seen

    original=np.arange(0,6,dtype=np.int32)
    window=original[2:5]
    window[0]=999
    print(original)
    print(window)
    window_copy=original[2:5].copy()
    # 对 [2:5] 这个 slice 深拷贝了一份副本出来
    # 让 window_copy 指向这个副本对象
    window_copy[0]=114514
    print(original)
    print(window_copy)
if __name__=="__main__":
    test1()
    test2()
    test3()