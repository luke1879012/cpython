/* Frame object interface */

#ifndef Py_LIMITED_API
#ifndef Py_FRAMEOBJECT_H
#define Py_FRAMEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int b_type;                 /* what kind of block this is */
    int b_handler;              /* where to jump to find handler */
    int b_level;                /* value stack level to pop to */
} PyTryBlock;

typedef struct _frame {
    // 可变对象的头部信息
    PyObject_VAR_HEAD
    // 上一级栈帧，也就是调用者的栈帧
    struct _frame *f_back;      /* previous frame, or NULL */
    // PyCodeObject对象
    // 通过栈帧的f_code属性可以获取对应的PyCodeObject对象
    PyCodeObject *f_code;       /* code segment */
    // builtin名字空间，一个PyDictObject对象
    PyObject *f_builtins;       /* builtin symbol table (PyDictObject) */
    // global名字空间，一个PyDictObject对象
    PyObject *f_globals;        /* global symbol table (PyDictObject) */
    // local名字空间，一个PyDictObject对象
    PyObject *f_locals;         /* local symbol table (any mapping) */
    // 运行时的栈底位置
    PyObject **f_valuestack;    /* points after the last local */
    /* Next free slot in f_valuestack.  Frame creation sets to f_valuestack.
       Frame evaluation usually NULLs it, but a frame that yields sets it
       to the current stack top. */
    // 运行时的栈顶位置
    PyObject **f_stacktop;
    // 回溯函数，打印异常栈
    PyObject *f_trace;          /* Trace function */
    // 是否触发每一行的回溯事件
    char f_trace_lines;         /* Emit per-line trace events? */
    // 是否触发每一个操作码的回溯事件
    char f_trace_opcodes;       /* Emit per-opcode trace events? */

    // 是否是基于生成器的PyCodeObject构建的栈帧
    /* Borrowed reference to a generator, or NULL */
    PyObject *f_gen;

    // 上一条已执行完毕的指令在f_code中的偏移量
    int f_lasti;                /* Last instruction if called */
    /* Call PyFrame_GetLineNumber() instead of reading this field
       directly.  As of 2.3 f_lineno is only valid when tracing is
       active (i.e. when f_trace is set).  At other times we use
       PyCode_Addr2Line to calculate the line from the current
       bytecode index. */
    // 当前字节码对应的源码行号
    int f_lineno;               /* Current line number */
    // 当前指令在栈f_blockstack中的索引
    int f_iblock;               /* index in f_blockstack */
    // 当前栈帧是否仍在执行
    char f_executing;           /* whether the frame is still executing */
    // 用于try和loop代码块
    PyTryBlock f_blockstack[CO_MAXBLOCKS]; /* for try and loop blocks */
    // 动态内存
    // 维护"局部变量+cell对象集合+free对象集合+运行时栈"所需要的空间
    PyObject *f_localsplus[1];  /* locals+stack, dynamically sized */
} PyFrameObject;


/* Standard object interface */

PyAPI_DATA(PyTypeObject) PyFrame_Type;

#define PyFrame_Check(op) (Py_TYPE(op) == &PyFrame_Type)

PyAPI_FUNC(PyFrameObject *) PyFrame_New(PyThreadState *, PyCodeObject *,
                                        PyObject *, PyObject *);

/* only internal use */
PyFrameObject* _PyFrame_New_NoTrack(PyThreadState *, PyCodeObject *,
                                    PyObject *, PyObject *);


/* The rest of the interface is specific for frame objects */

/* Block management functions */

PyAPI_FUNC(void) PyFrame_BlockSetup(PyFrameObject *, int, int, int);
PyAPI_FUNC(PyTryBlock *) PyFrame_BlockPop(PyFrameObject *);

/* Extend the value stack */

PyAPI_FUNC(PyObject **) PyFrame_ExtendStack(PyFrameObject *, int, int);

/* Conversions between "fast locals" and locals in dictionary */

PyAPI_FUNC(void) PyFrame_LocalsToFast(PyFrameObject *, int);

PyAPI_FUNC(int) PyFrame_FastToLocalsWithError(PyFrameObject *f);
PyAPI_FUNC(void) PyFrame_FastToLocals(PyFrameObject *);

PyAPI_FUNC(int) PyFrame_ClearFreeList(void);

PyAPI_FUNC(void) _PyFrame_DebugMallocStats(FILE *out);

/* Return the line of code the frame is currently executing. */
PyAPI_FUNC(int) PyFrame_GetLineNumber(PyFrameObject *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_FRAMEOBJECT_H */
#endif /* Py_LIMITED_API */
