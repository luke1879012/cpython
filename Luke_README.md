边看代码边记笔记

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
```

float -> 24



```