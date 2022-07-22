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
* [float](#float内存大小)
* [int](#int内存大小)
* [bytes](#bytes内存大小)
* [str](#str内存大小)



##  其他

 类型对象定义了哪些**操作**，决定了实例对象具有哪些**行为** 






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



### float内存大小

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



### int内存大小

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



### 行为

#### **整数的大小比较** 

```
PyLong_Type -> long_richcompare -> long_compare
```

首先整数在底层是通过数组存储的，ob_size 的绝对值维护了数组的长度。显然数组越长，整数的绝对值就越大。 

那么两个整数进行比较时：

- 如果ob_size均为正，显然 ob_size 越大，底层数组越长，对应的整数越大；
- 如果ob_size均为负，显然 ob_size 越大（越靠近0），底层数组越短，整数的绝对值越小。但因为是负数，还要乘上**-1**，因此整数反而会越大；
- 如果ob_size一正一负，这个应该就不必说了， ob_size 体现了整数的正负。正数肯定大于负数，所以 ob_size 大的那一方，对应的整数更大。



#### 整数的加法

```
PyLong_Type -> long_as_number -> long_add -> x_add
                                          -> x_sub
```

逻辑：

1. 两个整数的ob_size都不超过1，直接通过MEDIUM_VALUE相加，并返回

2. `if Py_SIZE(a) < 0 and Py_SIZE(b) < 0`

   说明都是负数，转换为`-(|a| + |b|)`，并返回

3. `if Py_SIZE(a) < 0 and Py_SIZE(b) > 0`

   说明a为负数，b为正数，转换为`|b| - |a|`，并返回

4. `if Py_SIZE(a) > 0 and Py_SIZE(b) < 0`

   说明a为正数，b为负数，转换为`|a| - |b|`，并返回

5. `if Py_SIZE(a) > 0 and Py_SIZE(b) > 0`

   说明都是正数，直接相加就好了

因为`ob_digit`来维护具体数值，没有正负，所以需要转换为 **绝对值加法(x_add)** 和**绝对值减法(x_sub)**



具体计算： 逢`2**30`进一 

```c
#define PyLong_SHIFT    30
#define PyLong_BASE     ((digit)1 << PyLong_SHIFT)
#define PyLong_MASK     ((digit)(PyLong_BASE - 1))
```



#### 绝对值加法(x_add)

逻辑：

1. 对比a和b，将长一些的整数，放在a，短一些的整数，放在b
2. 遍历短整数b，每次循环中间产物放在carry中
3. 用`PyLong_MASK`保留没有溢出的部分，存放到结果z中
4. 将carry右移，保留溢出部分，引入到下次循环
5. 等到整数b循环完成后，从结束处开始，继续循环a
6. 逐步赋值到结果z中
7. 标准化整数z，删除高位为0的情况



#### 绝对值减法(x_sub)

逻辑：

1. 确定a和b的大小，将长的整数移到a，短的整数移动到b，用sign记录最终的符号

2. 如果长度相同，则从高位开始遍历，直到找到不同的位

3. 因为要做减法，高位直接舍弃，更新a、b和对应的`ob_size`

   将长的整数移动到a，短的移动到b，用sign记录符号

4. 遍历短整数b，每次减法产物存放在borrow中

   因为borrow是无符号32位整数，所以会自动借位

5. 用`PyLong_MASK`保留没有溢出的部分，存放到结果z中

6. 因为`ob_digit`中都是30位整数，所以通过第31位判断是否借位

7. 将`borrow`右移30位，并与1进行与运算，得到是否借位，并引入到下次循环

8. 从结束处，继续循环a

9. 判断sign，给结果z添加符号

10. 标准化整数z，删除高位为0的情况



#### 整数的减法

```
PyLong_Type -> long_as_number -> long_sub -> x_add
                                          -> x_sub
```

逻辑：

1. 快分支
2. 通过a，b的判断，分成四个部分
3. 转换成对应的绝对值计算



## bytes

实例对象：`PyBytesObject` [跳转](Include\bytesobject.h)

```c
typedef struct {
    PyObject_VAR_HEAD
    Py_hash_t ob_shash;
    char ob_sval[1];
} PyBytesObject;
```

类型对象：`PyBytes_Type` [跳转](Objects\bytesobject.c)



### bytes内存大小

```
ob_refcnt: 8字节
ob_type: 8字节
ob_size: 8字节
ob_shash: 8字节
ob_sval: 1字节 * (ob_size+1)
```

 一个空的字节序列，占33个字节 



### 生命周期

#### 创建对象



### 行为

#### bytes_as_number

```c
static PyNumberMethods bytes_as_number = {
    0,              /*nb_add*/
    0,              /*nb_subtract*/
    0,              /*nb_multiply*/
    bytes_mod,      /*nb_remainder*/
};
```

借用 % 运算符实现了格式化



#### bytes_as_sequence

```c
static PySequenceMethods bytes_as_sequence = {
    (lenfunc)bytes_length, /*sq_length*/
    (binaryfunc)bytes_concat, /*sq_concat*/
    (ssizeargfunc)bytes_repeat, /*sq_repeat*/
    (ssizeargfunc)bytes_item, /*sq_item*/
    0,                  /*sq_slice*/
    0,                  /*sq_ass_item*/
    0,                  /*sq_ass_slice*/
    (objobjproc)bytes_contains /*sq_contains*/
};
```

* `sq_length`: 查看序列的长度
* `sq_concat`: 将两个序列合并为一个
* `sq_repeat`: 将序列重复多次
* `sq_item`: 根据索引获取指定位置的字节，返回的是一个整数
* `sq_contains`: 判断某个序列是不是该序列的子序列，对应Python中的in操作符



### 效率问题

Python的不可变对象在运算时，处理方式是再创建一个新的。

所以三个bytes对象a、b、c在相加时，那么会先根据a+b创建**临时对象**，然后再根据**临时对象**+c创建新的对象，最后返回指针

效率非常低下，因为涉及大量临时对象的创建和销毁。

官方推荐的做法，使用`join`方法



### 字节序列缓存池

256个单字节序列

实现：`Objects\bytesobject.c` [跳转](Objects\bytesobject.c)

```c
static PyBytesObject *characters[UCHAR_MAX + 1];
```

```
PyBytes_FromStringAndSize
```



## str

实例对象：`PyUnicodeObject` [跳转](Include\cpython\unicodeobject.h)

```c
typedef struct {
    PyObject_HEAD
    Py_ssize_t length;          
    Py_hash_t hash;            
    struct {
        unsigned int interned:2;
        unsigned int kind:3;
        unsigned int compact:1;
        unsigned int ascii:1;
        unsigned int ready:1;
        unsigned int :24;
    } state;
    wchar_t *wstr;              
} PyASCIIObject;

typedef struct {
    PyASCIIObject _base;
    Py_ssize_t utf8_length;     
    char *utf8;                
    Py_ssize_t wstr_length; 
} PyCompactUnicodeObject;

typedef struct {
    PyCompactUnicodeObject _base;
    union {
        void *any;
        Py_UCS1 *latin1;
        Py_UCS2 *ucs2;
        Py_UCS4 *ucs4;
    } data;                   
} PyUnicodeObject;

```

​    标志位：

* `interned`：是否被intern机制维护
* `kind`：类型，用于区分底层存储单元的大小。如果是Latin1编码，那么就是1；UCS2是2；UCS4是4；
* `compact`：内存分配方式，对象与文本缓冲区是否分离
* `ascii`：字符串是否是纯ASCII字符串，如果是则为1，不是为0。注意：只有对应的ASCII码为0~127之间的才是ASCII字符



类型对象：`PyUnicode_Type` [跳转](Objects\unicodeobject.c)



### unicode的三种编码

为了减少内存消耗并提高性能，Python使用了三种编码方式表示Unicode:

* `Latin-1` 编码：每个字符一字节
* `UCS2 `编码： 每个字符两个字节
* `UCS4 `编码： 每个字符四个字节



### 为什么不使用utf-8编码

utf-8可变，字符占的大小不同，有些占1字节，有些占3字节

采用utf-8的话，就无法以`O(1)`的时间复杂度去准确地定位字符



### 三种编码，该使用哪一种？

Python创建字符串的时候，会先扫描，尝试使用占字节数最少的编码进行存储

一旦改变编码，字符串中的所有字符都会使用同样的编码，因为它们不具备可变长功能



### 各个结构体

|              |     maxchar < 2**7     |      maxchar < 2**8      |     maxchar < 2**16      |     maxchar < 2**32      |
| :----------: | :--------------------: | :----------------------: | :----------------------: | :----------------------: |
|     kind     | `PyUnicode_1BYTE_KIND` |  `PyUnicode_1BYTE_KIND`  |  `PyUnicode_2BYTE_KIND`  |  `PyUnicode_4BYTE_KIND`  |
|    ascii     |           1            |            0             |            0             |            0             |
| 字符存储单元 |           1            |            1             |            2             |            4             |
|  底层结构体  |    `PyASCIIObject`     | `PyCompactUnicodeObject` | `PyCompactUnicodeObject` | `PyCompactUnicodeObject` |





### str内存大小

Python的每一个字符串都需要额外占用49-80字节。因为要存储一些额外信息，比如：公共的头部、哈希、长度、字节长度、编码类型等等

* `Latin-1`：额外占用49字节 + 每个字符一字节
* `UCS2`：额外占用74字节 + 每个字符两个字节
* `UCS4`：额外占用76字节 + 每个字符四个字节



#### PyASCIIObject

```
ob_refcnt: 8字节
ob_type: 8字节
length: 8字节
hash: 8字节
state: 4字节
	interned: 2比特
	kind: 3比特
	compact: 1比特
	ascii: 1比特
	ready: 1比特
	     : 24比特
空洞: 4字节(8字节对齐)
wstr: 8字节
```

总共是48字节，字符数组内部有一个`\0`表示结尾，所以要加1，为49字节



#### PyCompactUnicodeObject

```
_base: 48字节(PyASCIIObject)
utf8_length: 8字节
utf8: 8字节
wstr_length: 8字节
```

总共是72字节，加上`\0`结果，所以根据类型：

* 如果128<=maxchar<=256，对应则是73字节
* 如果是UCS2，对应则是74字节
* 如果是UCS4，对应则是76字节



### kind类型

####  **PyUnicode_1BYTE_KIND** 

```
PyASCIIObject -> state -> kind == 1
```



####  **PyUnicode_2BYTE_KIND**

```
PyASCIIObject -> state -> kind == 2
```



####  **PyUnicode_4BYTE_KIND** 

```
PyASCIIObject -> state -> kind == 4
```



### 生命周期

#### 创建对象

Python/C API:

```
PyUnicode_FromString -> PyUnicode_DecodeUTF8Stateful -> unicode_decode_utf8
PyUnicode_FromStringAndSize
```



#### 行为

[跳转](Objects\unicodeobject.c)

`unicode_as_number`

`unicode_as_sequence`



### intern机制

在Python中，某些字符串也可以像小整数对象池的整数一样，共享给所有变量使用，从而通过避免重复创建来降低内存使用、减少性能开销，这就是intern机制

Python的做法是在内部维护一个全局字典，所有开启intern机制的字符串均会保存在这里，后续如果需要使用的话，会先尝试在全局字典中获取，从而实现避免重复创建的功能。



> 所以现在我们就明白了intern机制，并不是说先判断是否存在，如果存在，就不创建。而是先创建，然后发现已经有其他的PyUnicodeObject维护了一个与之相同的字符数组，于是intern机制将引用计数减一，导致引用计数为0，最终被回收。 

 但是这么做的原因是什么呢？为什么非要创建一个PyUnicodeObject来完成intern操作呢？这是因为PyDictObject必须要求必须以**PyObject \***作为key。 



 **另外我们也可以通过sys.intern强制开启字符串的intern机制。** 



####  **什么样的字符才会开启intern机制呢？** 

 在Python3.8中，如果一个字符串的所有字符都位于0 ~ 127之间，那么会开启intern机制。 

 在Python3.8中，如果一个字符串只有一个字符，并且位于0~255之间，那么会开启intern机制。 





## list

实例对象：` PyListObject ` [跳转](Include\listobject.h)

```c
typedef struct {
    PyObject_VAR_HEAD
    PyObject **ob_item;
    Py_ssize_t allocated;
} PyListObject;
```

- ob_item：一个二级指针，指向PyObject *类型的指针数组，这个指针数组保存的便是对象的指针，而操作底层数组都是通过ob_item来进行操作的
- allocated：最大容量（`lst.__sizeof__()`查看）



类型对象：`PyList_Type` [跳转](Objects\listobject.c)



### list内存大小

底层对应的PyListObject实例的大小其实是不变的，因为指针数组没有存在PyListObject里面。但是Python在计算内存大小的时候是会将这个指针数组也算进去的，所以Python中列表的大小是可变的。 

```
ob_refcnt: 8字节
ob_type: 8字节
ob_size: 8字节
ob_item: 8字节
allocated: 8字节
```

总共为40字节，因为ob_item的指针数组也要算进去，所以为 40+8*allocated



### 行为

`list_as_sequence` [跳转](Objects\listobject.c)

`list_as_mapping`



### 生命周期

#### 创建对象

Python/C API:

```
PyList_New
```







### 注意点

 copy.deepcopy虽然在拷贝指针的同时会将指针指向的对象也拷贝一份，但这仅仅是针对于**可变对象**，对于**不可变对象**是不会拷贝的。 



```python

lst = [[]] * 5
lst[0].append(1)
print(lst)  # [[1], [1], [1], [1], [1]]
# 列表乘上一个n，等于把列表里面的元素重复n次
# 但列表里面存储的是指针，也就是将指针重复n次
# 所以上面的列表里面的5个指针存储的地址是相同的
# 也就是说，它们都指向了同一个列表


# 这种方式创建的话，里面的指针都指向了不同的列表
lst = [[], [], [], [], []]
lst[0].append(1)
print(lst)  # [[1], [], [], [], []]


# 再比如字典，在后续系列中会说
d = dict.fromkeys([1, 2, 3, 4], [])
print(d)  # {1: [], 2: [], 3: [], 4: []}
d[1].append(123)
print(d)  # {1: [123], 2: [123], 3: [123], 4: [123]}
# 它们都指向了同一个列表
```











