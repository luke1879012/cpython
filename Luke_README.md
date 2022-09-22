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

**sys.getsizeof**和**`__sizeof__`**方法，两者有什么区别呢？

答案是使用**sys.getsizeof**会比调用对象的**`__sizeof__`**方法计算出来的内存大小多16个字节。

原因是一个可以发生循环引用的对象，而对于可以发生循环引用的对象，都将参与GC。因此它们除了PyObject_Head之外，还会额外有一个16字节的PyGC_Head。



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





## tuple

 元组存在的意义：它可以作为字典的 key、以及可以作为集合的元素。 



实例对象：`PyTupleObject`  [跳转](Include\cpython\tupleobject.h)

```c
typedef struct {
    PyObject_VAR_HEAD
    PyObject *ob_item[1];
} PyTupleObject;
```

类型对象：`PyTuple_Type` [跳转](Objects\tupleobject.c)



### 生命周期

#### 创建对象

Python/C API：

```
PyTuple_New
```



### 缓存池

20条链表，每个链表可以有2000个元组，链表的索引对应元组的长度

元组的这项技术也被称为**静态资源缓存**，因为元组在执行析构函数时，**不仅对象本身没有被回收，连底层的指针数组也被缓存起来了**。那么当再次分配时，速度就会快一些。 



## dict

**哈希表就是一种空间换时间的方法** 

实例对象：` PyDictObject` [跳转](Include\cpython\dictobject.h)

```c
typedef struct {
    PyObject_HEAD
    Py_ssize_t ma_used;
    uint64_t ma_version_tag;
    PyDictKeysObject *ma_keys;
    PyObject **ma_values;
} PyDictObject;
```



类型对象：`PyDict_Type` [跳转](Objects\dictobject.c)





### 结合表和分离表 

* 结合表的话，键和值就存在一起；
* 分离表的话，键和值就存在不同的地方。 



 分离表是在PEP-0412中被引入的，主要是为了提高内存使用率，也就是让不同的字典共享相同的一组key。 



我们平时自己创建的字典，使用的都是结合表



### 原理

**哈希索引数组**和**键值对数组** 分开存储




###  **容量策略** 

 实践经验表明，一个**1/2**到**2/3**满的哈希表，性能较为理想 



### dict的内存大小

`PyDictObject`：48字节

```
ob_refcnt: 8字节
ob_type: 8字节
ma_used: 8字节
ma_version_tag: 8字节
ma_keys: 8字节
ma_values: 8字节
```

`PyDictKeysObject`：40字节 + dk_size*1字节

```
dk_refcnt: 8字节
dk_size: 8字节
dk_lookup: 8字节
dk_usable: 8字节
dk_nentries: 8字节
dk_indices: dk_size*1字节
```

`PyDictKeyEntry`：24字节

```
me_hash: 8字节
me_key: 8字节
me_value: 8字节
```

空字典(不通过Python/C API创建)：

一个`PyDictObject`，一个`PyDictKeysObject`

最少8个哈希索引数组，通过2/3计算，得到5个键值对容量(`PyDictKeyEntry`)

= `48 + 40 + 8*1 + 24*5` = 216



空字典(`d={}`)：

只有`PyDictObject`对象

= `48`



###  **索引冲突** 

**解决索引冲突的常用方法有两种：**

- 分离链接法(separate chaining)
- 开放寻址法(open addressing)



####  **分离链接法** 

分离链接法为每个哈希槽维护一个链表，所有哈希到同一槽位的键保存到对应的链表中



####  **开放寻址法** 

 Python依旧是将key映射成索引，并存在哈希索引数组的槽中，若发现槽被占了，那么就尝试另一个 

#####  常见的探测函数 

- 线性探测(linear probing)
- 平方探测(quadratic probing)

 

Python对此进行了优化，探测函数会参考对象哈希值，生成不同的探测序列，进一步降低索引冲突的可能性 

Python的这种做法被称为**迭代探测**，当然迭代探测也属于开放寻址法的一种。 

公式：

```c
// 将当前哈希值右移PERTURB_SHIFT个位
perturb >>= PERTURB_SHIFT;
// 然后将哈希值加上 i*5 + 1，这个 i 就是当前冲突的索引
// 运算之后的结果再和mask按位与，得到一个新的 i
// 然后判断变量 i 是否可用，不可用重复当前逻辑
// 直到出现一个可用的槽
i = (i*5 + perturb + 1) & mask;
```





####  **探测函数** 

 Python为哈希表搜索提供了多种探测函数，例如

* `lookdict`：一般通用
* `lookdict_unicode`： 专门针对key为字符串的entry 
* `lookdict_index`： 针对key为整数的entry 



因此lookdict这个函数只是告诉我们当前哈希表能否找到一个可用的槽去存储，如果能，那么再由find_empty_slot函数将**槽**的索引返回， 





####  entry的三种状态 

*  **unused态** 

*  **active态** 

*  **dummy态** 

  当key被删除时，它所在的entry便从`active`态编程了`dummy`态( 伪删除技术 )





###  **哈希攻击** 

- Python解释器进程启动后，产生一个随机数作为盐；
- 哈希函数同时参考对象本身以及随机数计算哈希值；





### 生命周期

#### 创建

`PyDict_New`





### 行为

#### 添加键值对

`PyDict_SetItem`



#### 获取值

`dict_subscript`



#### 删除键值对

`PyDict_DelItem`



### 扩容机制

#### `ma_used`与`dk_nentries`

PyDictObject里面有一个ma_used字段，它维护的是键值对的数量，充当ob_size；

而在PyDictKeysObject里面有一个dk_nentries，它维护键值对数组中已使用的entry数量，而这个entry又可以理解为键值对 



如果不涉及元素的删除，那么两者的值会是一样的。而一旦删除某个已存在的key，那么ma_used会**减1**，而dk_nentries则保持不变。 



首先ma_used**减1**表示键值对数量相比之前少了一个，这显然符合我们在Python里面使用字典时的表现；但我们知道元素的删除其实是伪删除，会将对应的entry从active态变成dummy态，然而entry的总数量并没有改变。


 ma_used其实等于active态的entry总数；如果将dk_nentries减去dummy态的entry总数，那么得到的就是ma_used。 





#### 扩容

 当已使用entry的数量达到了总容量的2/3时，会发生扩容。 

逻辑：

1. 确定哈希表的容量，容量必须是2的幂次方，并且大于`ma_used*3`
2. 重新申请内存
3. 判断`dk_nentries`是否与`ma_used`相等，如果相等，说明没有`dummy`态的entry，直接复制过去；如果不相等，只需要复制 `me_value==NULL`的entry（`active`态的entry）



### 缓存池

`PyDictObject`和`PyDictKeysObject`都有缓存池

```c
#ifndef PyDict_MAXFREELIST
#define PyDict_MAXFREELIST 80
#endif
static PyDictObject *free_list[PyDict_MAXFREELIST];
static int numfree = 0;
static PyDictKeysObject *keys_free_list[PyDict_MAXFREELIST];
static int numfreekeys = 0;
```

大小都是80

`PyDictKeysObject`缓存条件：当缓存池未满、并且dk_size为8的时候被缓存





## set

实例对象：`PySetObject` [跳转](Include\setobject.h)

```c
typedef struct {
    PyObject_HEAD
    Py_ssize_t fill;
    Py_ssize_t used;
    Py_ssize_t mask;
    setentry *table;
    Py_hash_t hash;
    Py_ssize_t finger;
    setentry smalltable[PySet_MINSIZE];
    PyObject *weakreflist;
} PySetObject;
```

类型对象：`PySet_Type` [跳转](Objects\setobject.c)



### 行为

####  **添加元素** 

`PySet_Add`



#### **删除元素**

`set_remove`



### 扩容

`set_table_resize`

逻辑：

1. `newsize`从8开始，不断左移，直到大于`minused`
2. 如果`newsize`等于8，扔掉`dumm`y，将`smalltable`复制过去
3. 如果`newsize`大于8，申请`table`空间，大小为`newsize`
4. 创建新的`setentry`，赋值`mask`、`table`
5. 如果没有`dummy`态，直接复制过去（优化性能）
6. 如果存在`dummy`态，删除`dummy`态的`entry`





# 内置函数

所有的内置函数都在 `Python\bltinmodule.c` 中  [跳转](Python\bltinmodule.c)



## iter/next

 迭代器本质上只是对原始数据的一层封装罢了 





# Python虚拟机

## 基本流程

### 基本流程

解释器执行py文件的流程：

1. 首先将文件里面的内容读取出来
2. 读取文件里面的内容之后会对其进行分词，将源代码切分成一个一个的token
3. 然后Python编译器会对token进行语法解析，建立抽象语法树（AST，abstract syntax tree）
4. 编译器再将得到的AST编译成PyCodeObject对象
5. 最终由Python虚拟机来执行字节码



###  Python编译器、Python虚拟机 、Python解释器

 **Python解释器=Python编译器+Python虚拟机** 

* Python编译器：负责将Python源代码编译成`PyCodeObject`对象
* Python虚拟机：执行`PyCodeObject`对象



之所以要存在编译，是为了提前分配好常量、能够让虚拟机更快速地执行，而且还可以尽早检测出语法上的错误。 





###  **PyCodeObject对象和pyc文件的关系** 

 在Python开发时，我们肯定都见过这个pyc文件，它一般位于`__pycache__`目录中

 pyc文件里面的内容是PyCodeObject对象。 

在程序运行期间，编译结果存在于内存的PyCodeObject对象当中，而Python结束运行之后，编译结果又被保存到了pyc文件当中。当下一次运行的时候，Python会根据pyc文件中记录的编译结果直接建立内存中的PyCodeObject对象，而不需要再度重新编译了，当然前提是没有对源文件进行修改。 





###  **PyCodeObject的底层结构** 

路径：`Include\code.h`  [跳转](Include\code.h)

```c
typedef struct {
    PyObject_HEAD
    int co_argcount;            /* #arguments, except *args */
    int co_posonlyargcount;     /* #positional only arguments */
    int co_kwonlyargcount;      /* #keyword only arguments */
    int co_nlocals;             /* #local variables */
    int co_stacksize;           /* #entries needed for evaluation stack */
    int co_flags;               /* CO_..., see below */
    int co_firstlineno;         /* first source line number */
    PyObject *co_code;          /* instruction opcodes */
    PyObject *co_consts;        /* list (constants used) */
    PyObject *co_names;         /* list of strings (names used) */
    PyObject *co_varnames;      /* tuple of strings (local variable names) */
    PyObject *co_freevars;      /* tuple of strings (free variable names) */
    PyObject *co_cellvars;      /* tuple of strings (cell variable names) */
    /* The rest aren't used in either hash or comparisons, except for co_name,
       used in both. This is done to preserve the name and line number
       for tracebacks and debuggers; otherwise, constant de-duplication
       would collapse identical functions/lambdas defined on different lines.
    */
    Py_ssize_t *co_cell2arg;    /* Maps cell vars which are arguments. */
    PyObject *co_filename;      /* unicode (where it was loaded from) */
    PyObject *co_name;          /* unicode (name, for reference) */
    PyObject *co_lnotab;        /* string (encoding addr<->lineno mapping) See
                                   Objects/lnotab_notes.txt for details. */
    void *co_zombieframe;       /* for optimization only (see frameobject.c) */
    PyObject *co_weakreflist;   /* to support weakrefs to code objects */
    /* Scratch space for extra data relating to the code object.
       Type is a void* to keep the format private in codeobject.c to force
       people to go through the proper APIs. */
    void *co_extra;

    /* Per opcodes just-in-time cache
     *
     * To reduce cache size, we use indirect mapping from opcode index to
     * cache object:
     *   cache = co_opcache[co_opcache_map[next_instr - first_instr] - 1]
     */

    // co_opcache_map is indexed by (next_instr - first_instr).
    //  * 0 means there is no cache for this opcode.
    //  * n > 0 means there is cache in co_opcache[n-1].
    unsigned char *co_opcache_map;
    _PyOpcache *co_opcache;
    int co_opcache_flag;  // used to determine when create a cache.
    unsigned char co_opcache_size;  // length of co_opcache.
} PyCodeObject;
```



 一个**code block**对应一个**名字空间(或者说作用域)**、同时也对应一个**PyCodeObject对象** 



##  **PyCodeObject** 

 **co_argcount：可以通过位置参数传递的参数个数** 

```python
def foo(a, b, c=3):
    pass
print(foo.__code__.co_argcount)  # 3

def bar(a, b, *args):
    pass
print(bar.__code__.co_argcount)  # 2

def func(a, b, *args, c):
    pass
print(func.__code__.co_argcount)  # 2
```



 **co_posonlyargcount：只能通过位置参数传递的参数个数，Python3.8新增** 

```python
def foo(a, b, c):
    pass
print(foo.__code__.co_posonlyargcount)  # 0

def bar(a, b, /, c):
    pass
print(bar.__code__.co_posonlyargcount)  # 2
```



 **co_kwonlyargcount：只能通过关键字参数传递的参数个数** 

```python
def foo(a, b=1, c=2, *, d, e):
    pass
print(foo.__code__.co_kwonlyargcount)  # 2
```



 **co_nlocals：代码块中局部变量的个数，也包括参数** 

```python
def foo(a, b, *, c):
    name = "xxx"
    age = 16
    gender = "f"
    c = 33

print(foo.__code__.co_nlocals)  # 6
```



 **co_stacksize：执行该段代码块需要的栈空间** 

```python
def foo(a, b, *, c):
    name = "xxx"
    age = 16
    gender = "f"
    c = 33

print(foo.__code__.co_stacksize)  # 1
```



 **co_flags：参数类型标识** 

```python
def foo1():
    pass
# 结果全部为假
print(foo1.__code__.co_flags & 0x04)  # 0
print(foo1.__code__.co_flags & 0x08)  # 0

def foo2(*args):
    pass
# co_flags & 0x04 为真，因为出现了 *args
print(foo2.__code__.co_flags & 0x04)  # 4
print(foo2.__code__.co_flags & 0x08)  # 0

def foo3(*args, **kwargs):
    pass
# 显然 co_flags & 0x04 和 co_flags & 0x08 均为真
print(foo3.__code__.co_flags & 0x04)  # 4
print(foo3.__code__.co_flags & 0x08)  # 8
```

co_flags 可以做的事情并不止这么简单，它还能检测一个函数的类型。比如函数内部出现了 yield，那么它就是一个生成器函数，调用之后可以得到一个生成器；使用 async def 定义，那么它就是一个协程函数，调用之后可以得到一个协程。 



 **co_firstlineno：代码块在对应文件的起始行** 

```python
def foo(a, b, *, c):
    pass

# 显然是文件的第一行
# 或者理解为 def 所在的行
print(foo.__code__.co_firstlineno)  # 1
```



 **co_names：符号表，一个元组，保存代码块中引用的其它作用域的变量** 

```python
c = 1

def foo(a, b):
    print(a, b, c)
    d = (list, int, str)

print(
    foo.__code__.co_names
)  # ('print', 'c', 'list', 'int', 'str')
```



 **co_varnames：符号表，一个元组，保存在当前作用域中的变量** 

```python
c = 1

def foo(a, b):
    print(a, b, c)
    d = (list, int, str)
print(foo.__code__.co_varnames)  # ('a', 'b', 'd')
```



 **co_consts：常量池，一个元组，保存代码块中的所有常量** 

```python
x = 123

def foo(a, b):
    c = "abc"
    print(x)
    print(True, False, list, [1, 2, 3], {"a": 1})
    return ">>>"

print(
    foo.__code__.co_consts
)  # (None, 'abc', True, False, 1, 2, 3, 'a', '>>>')
```



 **co_freevars：内层函数引用的外层函数的作用域中的变量** 

```python
def f1():
    a = 1
    b = 2
    def f2():
        print(a)
    return f2

# 这里拿到的是f2的字节码
print(f1().__code__.co_freevars)  # ('a',)
```



 **co_cellvars：外层函数的作用域中被内层函数引用的变量，本质上和co_freevars是一样的** 

```python
def f1():    
    a = 1
    b = 2
    def f2():
        print(a)
    return f2

# 但这里调用的是f1的字节码
print(f1.__code__.co_cellvars)  # ('a',)
```



 **co_filename：代码块所在的文件名** 

```python
def foo():
    pass
    
print(foo.__code__.co_filename)  # D:/satori/main.py
```



 **co_name：代码块的名字** 

```python
def foo():
    pass
# 这里就是函数名
print(foo.__code__.co_name)  # foo
```



 **co_code：字节码** 

```python
def foo(a, b, /, c, *, d, e):
    f = 123
    g = list()
    g.extend([tuple, getattr, print])

print(foo.__code__.co_code)
#b'd\x01}\x05t\x00\x83\x00}\x06|\x06\xa0\x01t\x02t\x03t\x04g\x03\xa1\x01\x01\x00d\x00S\x00'
```



 **co_lnotab：字节码指令与源代码行号之间的对应关系，以PyByteObject的形式存在** 

```python
def foo(a, b, /, c, *, d, e):
    f = 123
    g = list()
    g.extend([tuple, getattr, print])
    
print(foo.__code__.co_lnotab)  # b'\x00\x01\x04\x01\x06\x01'
```



## 编译

 **eval：传入一个字符串，然后把字符串里面的内容拿出来。** 

```python
a = 1
# 所以eval("a")就等价于a
print(eval("a"))  # 1

print(eval("1 + 1 + 1"))  # 3
```

 注意：eval是有返回值的，返回值就是字符串里面内容。 



 **exec：传入一个字符串，把字符串里面的内容当成语句来执行，这个是没有返回值的，或者说返回值是None。** 

```python
# 相当于 a = 1
exec("a = 1")  
print(a)  # 1

statement = """
a = 123
if a == 123:
    print("a等于123")
else:
    print("a不等于123")
"""
exec(statement)  # a等于123
```



 **compile：执行后返回的就是一个PyCodeObject对象。** 

参数：

1. 当成代码执行的字符串；
2. 可以为这些代码起一个文件名；
3. 执行方式，支持三种，分别是exec、single、eval 
   1. exec：将源代码当做一个模块来编译；
   2. single：用于编译一个单独的Python语句(交互式下)；
   3. eval：用于编译一个eval表达式；





### **字节码与反编译**

操作指令 [跳转](Include\opcode.h)



## **pyc文件**

### 触发条件

 import会触发pyc的生成 





###  **pyc文件里面包含哪些内容** 

 **1. magic number** 

这是Python定义的一个整数值，不同版本的Python会定义不同的magic number，这个值是为了保证Python能够加载正确的pyc。 

**2. pyc的创建时间** 

这个很好理解，判断源代码的最后修改时间和pyc文件的创建时间。如果pyc文件的创建时间比源代码的修改时间要早，说明在生成pyc之后，源代码被修改了，那么会重新编译并生成新的pyc，而反之则会直接加载已存在的pyc。 

**3. PyCodeObject对象** 





### **pyc文件的写入**

[跳转](Python\marshal.c)



### **PyCodeObject的包含关系**





## 虚拟机的执行环境

 运行时栈的地址是从高地址向低地址延伸的 

 **栈指针(rsp)**和**帧指针(rbp)** 

`rsp`: 指向栈顶

`rbp`: 当前栈



###  **栈帧的底层结构** 

` PyFrameObject `  [跳转](Include\frameobject.h)



### **在Python中访问PyFrameObject**

 可以通过inspect模块。 



### 名字、作用域、名字空间

 f_locals、f_globals、f_builtins 







