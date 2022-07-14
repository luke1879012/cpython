# 序
## Python对象在底层都叫啥名字？
```
整数 -> PyLongObject 结构体实例
int -> PyLong_Type (PyTypeObject结构体实例)

字符串 -> PyUnicodeObject 结构体实例
str -> PyUnicode_Type (PyTypeObject结构体实例)

浮点数 -> PyFloatObject 结构体实例
float -> PyFloat_Type (PyTypeObject结构体实例)

复数 -> PyComplexObject 结构体实例
complex -> PyComplex_Type (PyTypeObject结构体实例)

元组 -> PyTupleObject 结构体实例
tuple -> PyTupleObject (PyTypeObject结构体实例)

列表 -> PyListObject 结构体实例
list -> PyList_Type (PyTypeObject结构体实例)

字典 -> PyDictObject 结构体实例
dict -> PyDict_Type (PyTypeObject结构体实例)

集合 -> PySetObject 结构体实例
set -> PySetType (PyTypeObject结构体实例)

不可变集合 -> PyFrozenSetObject 结构体
frozenset -> PyFrozenSet_Type (PyTypeObject结构体实例)

元类: PyType_Type (PyTypeObject结构体实例)
```




## 各个类型的内存大小
* [float](#内存大小)


# 各个类型实现
## float
实例对象: PyFloatObject [跳转](Include\floatobject.h)
```C
typedef struct {
    PyObject_HEAD
    double ob_fval;
} PyFloatObject;
```

类型对象: PyFloat_Type [跳转](Objects\floatobject.c)


### 内存大小
24bit
```
PyObject_HEAD: 16bit
double: 8bit
```

### 生命周期

#### 创建对象
Python/C API:
```
PyFloat_FromDouble
PyFloat_FromString
```

通用: 
```
PyFloat_Type -> float_new -> float_new_impl
-> float_subtype_new
   PyFloat_FromString
   PyNumber_Float -> tp_as_number->nb_float -> __float__
                     tp_as_number->nb_index -> __index__
                     PyFloat_FromDouble
                     PyFloat_FromString
```

#### 销毁对象
```
PyFloat_Type -> float_dealloc
```



### 缓存池
空间换时间
#### 定义
路径: `Objects/floatobject.c` [跳转](Objects/floatobject.c)
```C
#ifndef PyFloat_MAXFREELIST
#define PyFloat_MAXFREELIST    100
#endif
static int numfree = 0;
static PyFloatObject *free_list = NULL;
```

使用内部的ob_type来指向下一个对象

