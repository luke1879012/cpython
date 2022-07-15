# 序

## 源码结构
* `Include`: 包含了Cpython所提供的所有头文件，主要包含了一些实例对象在底层的定义
* `Lib`: Python自带的所有标准库，基本都是纯Python写的
* `Modules`: 包含了所有C语言编写的模块。针对速度要求非常严格的模块
* `Parser`: 包含了解释器中的`Scanner`和`Parser`部分(词法分析和语法分析)。
* `Objects`: 包含了所有Python的内置类型对象的实现，以及其实例对象相关操作的实现。
* `Python`: 虚拟机的实现相关，Python运行的核心



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
使用`sys.getsizeof`获取，Python中对象的大小，是根据底层的结构体计算出来的
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

24字节
```
PyObject_HEAD: 16字节
double: 8字节
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



### 行为

float_as_number [跳转](Objects/floatobject.c)



## int

实例对象: PyLongObject [跳转](Include\longintrepr.h)
```C
struct _longobject {
    PyObject_VAR_HEAD
    digit ob_digit[1];
};
```

类型对象: 



### 不会溢出的实现
```
PyLongObject {
   ob_refcnt:
   ob_type:
   ob_size: 实现正负数，以及ob_digit数组的长度
   ob_digit: 32位无符号的整数(实际使用30位)
}
```
为什么解释器只使用30位？
> 1.加法进位有关，只能30位的话，相加之后还是可以用32位整数保存(溢出的话会先强制转换为64位整数再进行计算，影响效率)

> 2.为了方便计算乘法，需要多保留1位用于计算溢出。这样当两个整数相乘的时候，可以直接按digit计算，并且由于兼顾了"溢出位"，可以把结果直接保存在一个寄存器中，以获得最佳性能。



### 内存大小

```
ob_refcnt: 8字节
ob_type: 8字节
ob_size: 8字节
ob_digit: 4字节 * ob_size(绝对值)
```
例子：
```python
import numpy as np
a = 88888888888
print(np.log2(a + 1))  # 36.371284042320475
# 所以至少需要37位。一个digit是30位，所以需要两个digit
print(a // 2 ** 30)  # 82
print(a - 82 * 2 ** 30)  # 842059320
# 所以整数88888888888在底层对应的ob_digit数组为[842059320, 82]
```



### 生命周期

#### 创建对象
Python/C API: 
```
PyLong_FromLong
PyLong_FromDouble
PyLong_FromString
```



### 小整数对象池

Python将那些使用频率高的整数预先创建好，而且都是单例模式，这些预先创建好的整数会放在一个静态数组里面，我们称为小整数对象池

>  注意：这些整数在Python解释器启动的时候，就已经创建了。

实现 `Objects\longobject.c`： [跳转](Objects\longobject.c)

```c
#ifndef NSMALLPOSINTS
#define NSMALLPOSINTS           257
#endif
#ifndef NSMALLNEGINTS
#define NSMALLNEGINTS           5
#endif

static PyLongObject small_ints[NSMALLNEGINTS + NSMALLPOSINTS];
```

python3.8的优化：对于那些在编译期就能确定的常量，即使不在同一个编译单元中，那么也会只有一份 





















