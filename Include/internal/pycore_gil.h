#ifndef Py_INTERNAL_GIL_H
#define Py_INTERNAL_GIL_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_condvar.h"
#include "pycore_atomic.h"

#ifndef Py_HAVE_CONDVAR
#  error You need either a POSIX-compatible or a Windows system!
#endif

/* Enable if you want to force the switching of threads at least
   every `interval`. */
#undef FORCE_SWITCHING
#define FORCE_SWITCHING

struct _gil_runtime_state {
    // 一个线程拥有 GIL 的间隔，默认是5000微妙
    // 也就是调用 sys.getswitchinterval() 得到的 0.005
    /* microseconds (the Python API uses seconds, though) */
    unsigned long interval;

    // 最后一个持有 GIL 的 PyThreadState
    // 这有助于我们知道在丢弃GIL后是否还有其他线程被调度
    /* Last PyThreadState holding / having held the GIL. This helps us
       know whether anyone else was scheduled after we dropped the GIL. */
    _Py_atomic_address last_holder;

    // GIL是否被获取，这是原子性的
    // 因为在ceval.c中不需要任何锁就能读取它
    // 可以看成一个布尔变量，访问受到mutex字段保护，是否改变取决于cond字段
    /* Whether the GIL is already taken (-1 if uninitialized). This is
       atomic because it can be read without any lock taken in ceval.c. */
    _Py_atomic_int locked;

    // 从GIL创建之后，总共切换的次数
    /* Number of GIL switches since the beginning. */
    unsigned long switch_number;

    // cond允许一个或者多个线程等待，直到GIL被释放
    /* This condition variable allows one or several threads to wait
       until the GIL is released. In addition, the mutex also protects
       the above variables. */
    PyCOND_T cond;

    // mutex则是负责保护上面的变量
    PyMUTEX_T mutex;
#ifdef FORCE_SWITCHING
    // "GIL 等待线程"在被调度获取GIL之前
    // "GIL 释放线程"一直处于等待状态
    /* This condition variable helps the GIL-releasing thread wait for
       a GIL-awaiting thread to be scheduled and take the GIL. */
    PyCOND_T switch_cond;
    PyMUTEX_T switch_mutex;
#endif
};

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GIL_H */
