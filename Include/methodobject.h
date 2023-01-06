
/* Method object interface */

#ifndef Py_METHODOBJECT_H
#define Py_METHODOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/* This is about the type 'builtin_function_or_method',
   not Python methods in user-defined classes.  See classobject.h
   for the latter. */

PyAPI_DATA(PyTypeObject) PyCFunction_Type;

#define PyCFunction_Check(op) (Py_TYPE(op) == &PyCFunction_Type)

typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);
typedef PyObject *(*_PyCFunctionFast) (PyObject *, PyObject *const *, Py_ssize_t);
typedef PyObject *(*PyCFunctionWithKeywords)(PyObject *, PyObject *,
                                             PyObject *);
typedef PyObject *(*_PyCFunctionFastWithKeywords) (PyObject *,
                                                   PyObject *const *, Py_ssize_t,
                                                   PyObject *);
typedef PyObject *(*PyNoArgsFunction)(PyObject *);

PyAPI_FUNC(PyCFunction) PyCFunction_GetFunction(PyObject *);
PyAPI_FUNC(PyObject *) PyCFunction_GetSelf(PyObject *);
PyAPI_FUNC(int) PyCFunction_GetFlags(PyObject *);

/* Macros for direct access to these values. Type checks are *not*
   done, so use with care. */
#ifndef Py_LIMITED_API
#define PyCFunction_GET_FUNCTION(func) \
        (((PyCFunctionObject *)func) -> m_ml -> ml_meth)
#define PyCFunction_GET_SELF(func) \
        (((PyCFunctionObject *)func) -> m_ml -> ml_flags & METH_STATIC ? \
         NULL : ((PyCFunctionObject *)func) -> m_self)
#define PyCFunction_GET_FLAGS(func) \
        (((PyCFunctionObject *)func) -> m_ml -> ml_flags)
#endif
PyAPI_FUNC(PyObject *) PyCFunction_Call(PyObject *, PyObject *, PyObject *);

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyCFunction_FastCallDict(PyObject *func,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwargs);
#endif

// [module] 12. module->m_methods 的结构体
// [module] 13. 每一个PyMethodDef，都会基于它创建一个PyCFunctionObject对象
struct PyMethodDef {
    // 内置函数的名称
    const char  *ml_name;   /* The name of the built-in function/method */
    // 实现对应逻辑的C函数，但是需要转成PyCFunction类型
    // 主要是为了更好的处理关键字参数
    PyCFunction ml_meth;    /* The C function that implements it */
    /* 参数类型 
       #define METH_VARARGS  0x0001  扩展位置参数
       #define METH_KEYWORDS 0x0002  扩展关键字参数
       #define METH_NOARGS   0x0004  不需要参数
       #define METH_O        0x0008  需要一个参数
       #define METH_CLASS    0x0010  被classmethod装饰
       #define METH_STATIC   0x0020  被staticmethod装饰   
    */
    int         ml_flags;   /* Combination of METH_xxx flags, which mostly
                               describe the args expected by the C func */
    // 函数的__doc__
    const char  *ml_doc;    /* The __doc__ attribute, or NULL */
};
typedef struct PyMethodDef PyMethodDef;

#define PyCFunction_New(ML, SELF) PyCFunction_NewEx((ML), (SELF), NULL)
PyAPI_FUNC(PyObject *) PyCFunction_NewEx(PyMethodDef *, PyObject *,
                                         PyObject *);

/* Flag passed to newmethodobject */
/* #define METH_OLDARGS  0x0000   -- unsupported now */
#define METH_VARARGS  0x0001
#define METH_KEYWORDS 0x0002
/* METH_NOARGS and METH_O must not be combined with the flags above. */
#define METH_NOARGS   0x0004
#define METH_O        0x0008

/* METH_CLASS and METH_STATIC are a little different; these control
   the construction of methods for a class.  These cannot be used for
   functions in modules. */
#define METH_CLASS    0x0010
#define METH_STATIC   0x0020

/* METH_COEXIST allows a method to be entered even though a slot has
   already filled the entry.  When defined, the flag allows a separate
   method, "__contains__" for example, to coexist with a defined
   slot like sq_contains. */

#define METH_COEXIST   0x0040

#ifndef Py_LIMITED_API
#define METH_FASTCALL  0x0080
#endif

/* This bit is preserved for Stackless Python */
#ifdef STACKLESS
#define METH_STACKLESS 0x0100
#else
#define METH_STACKLESS 0x0000
#endif

#ifndef Py_LIMITED_API
// [module] 14. PyCFunctionObject的结构体
// [PyCFunctionObject] 1. PyCFunctionObject的结构体
typedef struct {
    // 头部信息
    PyObject_HEAD
    // PyMethodDef
    PyMethodDef *m_ml; /* Description of the C function to call */
    // self参数
    PyObject    *m_self; /* Passed as 'self' arg to the C func, can be NULL */
    // __module__属性
    PyObject    *m_module; /* The __module__ attribute, can be anything */
    // 弱引用列表，不讨论
    PyObject    *m_weakreflist; /* List of weak references */
    // 为了效率而单独实现的调用函数
    vectorcallfunc vectorcall;
} PyCFunctionObject;

PyAPI_FUNC(PyObject *) _PyMethodDef_RawFastCallDict(
    PyMethodDef *method,
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwargs);

PyAPI_FUNC(PyObject *) _PyMethodDef_RawFastCallKeywords(
    PyMethodDef *method,
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwnames);
#endif

PyAPI_FUNC(int) PyCFunction_ClearFreeList(void);

#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _PyCFunction_DebugMallocStats(FILE *out);
PyAPI_FUNC(void) _PyMethod_DebugMallocStats(FILE *out);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_METHODOBJECT_H */
