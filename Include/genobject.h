
/* Generator object interface */

#ifndef Py_LIMITED_API
#ifndef Py_GENOBJECT_H
#define Py_GENOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "pystate.h"   /* _PyErr_StackItem */

struct _frame; /* Avoid including frameobject.h */

/* _PyGenObject_HEAD defines the initial segment of generator
   and coroutine objects. */
#define _PyGenObject_HEAD(prefix)                                           \
    /* 头部信息 */                                                          \
    PyObject_HEAD                                                           \
    /* 生成器执行时对应的栈帧对象 */                                         \
    /* 用于保存执行上下文信息 */                                             \
    /* Note: gi_frame can be NULL if the generator is "finished" */         \
    struct _frame *prefix##_frame;                                          \
    /* 表示生成器是否在运行当中 */                                           \
    /* True if generator is being executed. */                              \
    char prefix##_running;                                                  \
    /* 生成器函数的 PyCodeObject 对象 */                                     \
    /* The code object backing the generator */                             \
    PyObject *prefix##_code;                                                \
    /* 弱引用相关，不深入讨论 */                                             \
    /* List of weak reference. */                                           \
    PyObject *prefix##_weakreflist;                                         \
    /* 生成器的名字 */                                                      \
    /* Name of the generator. */                                            \
    PyObject *prefix##_name;                                                \
    /* 生成器的全限定名 */                                                   \
    /* Qualified name of the generator. */                                  \
    PyObject *prefix##_qualname;                                            \
    /* 生成器执行出现异常时的异常栈 */                                       \
    /* 更准确的说，其实是异常栈的一个entry */                                \
    /* 里面包含了 exc_type、exc_value、exc_traceback */                      \
    /* 以及通过 previous_item 指针指向上一个entry */                         \
    _PyErr_StackItem prefix##_exc_state;
// 所以生成器在底层对应 PyGenObject，它的类型是 PyGen_Type

typedef struct {
    /* The gi_ prefix is intended to remind of generator-iterator. */
    _PyGenObject_HEAD(gi)
} PyGenObject;

PyAPI_DATA(PyTypeObject) PyGen_Type;

#define PyGen_Check(op) PyObject_TypeCheck(op, &PyGen_Type)
#define PyGen_CheckExact(op) (Py_TYPE(op) == &PyGen_Type)

PyAPI_FUNC(PyObject *) PyGen_New(struct _frame *);
PyAPI_FUNC(PyObject *) PyGen_NewWithQualName(struct _frame *,
    PyObject *name, PyObject *qualname);
PyAPI_FUNC(int) PyGen_NeedsFinalizing(PyGenObject *);
PyAPI_FUNC(int) _PyGen_SetStopIterationValue(PyObject *);
PyAPI_FUNC(int) _PyGen_FetchStopIterationValue(PyObject **);
PyAPI_FUNC(PyObject *) _PyGen_Send(PyGenObject *, PyObject *);
PyObject *_PyGen_yf(PyGenObject *);
PyAPI_FUNC(void) _PyGen_Finalize(PyObject *self);

#ifndef Py_LIMITED_API
typedef struct {
    _PyGenObject_HEAD(cr)
    PyObject *cr_origin;
} PyCoroObject;

PyAPI_DATA(PyTypeObject) PyCoro_Type;
PyAPI_DATA(PyTypeObject) _PyCoroWrapper_Type;

PyAPI_DATA(PyTypeObject) _PyAIterWrapper_Type;

#define PyCoro_CheckExact(op) (Py_TYPE(op) == &PyCoro_Type)
PyObject *_PyCoro_GetAwaitableIter(PyObject *o);
PyAPI_FUNC(PyObject *) PyCoro_New(struct _frame *,
    PyObject *name, PyObject *qualname);

/* Asynchronous Generators */

typedef struct {
    _PyGenObject_HEAD(ag)
    PyObject *ag_finalizer;

    /* Flag is set to 1 when hooks set up by sys.set_asyncgen_hooks
       were called on the generator, to avoid calling them more
       than once. */
    int ag_hooks_inited;

    /* Flag is set to 1 when aclose() is called for the first time, or
       when a StopAsyncIteration exception is raised. */
    int ag_closed;

    int ag_running_async;
} PyAsyncGenObject;

PyAPI_DATA(PyTypeObject) PyAsyncGen_Type;
PyAPI_DATA(PyTypeObject) _PyAsyncGenASend_Type;
PyAPI_DATA(PyTypeObject) _PyAsyncGenWrappedValue_Type;
PyAPI_DATA(PyTypeObject) _PyAsyncGenAThrow_Type;

PyAPI_FUNC(PyObject *) PyAsyncGen_New(struct _frame *,
    PyObject *name, PyObject *qualname);

#define PyAsyncGen_CheckExact(op) (Py_TYPE(op) == &PyAsyncGen_Type)

PyObject *_PyAsyncGenValueWrapperNew(PyObject *);

int PyAsyncGen_ClearFreeLists(void);

#endif

#undef _PyGenObject_HEAD

#ifdef __cplusplus
}
#endif
#endif /* !Py_GENOBJECT_H */
#endif /* Py_LIMITED_API */
