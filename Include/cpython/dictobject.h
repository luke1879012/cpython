#ifndef Py_CPYTHON_DICTOBJECT_H
#  error "this header file must not be included directly"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _dictkeysobject PyDictKeysObject;

/* The ma_values pointer is NULL for a combined table
 * or points to an array of PyObject* for a split table
 */
typedef struct {
    // 定长对象的头部信息，问题来了，字典是变长对象，头部应该是
    // PyObject_VAR_HEAD才对。因此肯定有别的成员来维护字典的长度
    PyObject_HEAD

    /* Number of items in the dictionary */
    // 字典里面键值对的个数，它充当了ob_size
    Py_ssize_t ma_used;

    /* Dictionary version: globally unique, value change each time
       the dictionary is modified */
    // 字典的版本号，对字典的每一次修改都会导致其改变
    // 除此之外还有一个全局的字典版本计数器pydict_global_version，任何一个字典修改都会影响它
    // 并且pydict_global_version会和最后操作的字典内部的ma_version_tag一致
    uint64_t ma_version_tag;

    // 它是一个指针，指向了PyDictKeyObject
    // Python里面的哈希表分成两种：combined table和split table，即结合表和分离表
    // 如果是结合表，那么键值对全部由ma_keys维护，此时ma_values为NULL
    PyDictKeysObject *ma_keys;

    /* If ma_values is NULL, the table is "combined": keys and values
       are stored in ma_keys.

       If ma_values is not NULL, the table is splitted:
       keys are stored in ma_keys and values are stored in ma_values */
    // 如果是分离表，那么键有ma_keys维护，值由ma_values维护
    // 而ma_values是一个二级指针，指向 PyObject * 类型的指针数组
    PyObject **ma_values;
} PyDictObject;

PyAPI_FUNC(PyObject *) _PyDict_GetItem_KnownHash(PyObject *mp, PyObject *key,
                                       Py_hash_t hash);
PyAPI_FUNC(PyObject *) _PyDict_GetItemIdWithError(PyObject *dp,
                                                  struct _Py_Identifier *key);
PyAPI_FUNC(PyObject *) _PyDict_GetItemStringWithError(PyObject *, const char *);
PyAPI_FUNC(PyObject *) PyDict_SetDefault(
    PyObject *mp, PyObject *key, PyObject *defaultobj);
PyAPI_FUNC(int) _PyDict_SetItem_KnownHash(PyObject *mp, PyObject *key,
                                          PyObject *item, Py_hash_t hash);
PyAPI_FUNC(int) _PyDict_DelItem_KnownHash(PyObject *mp, PyObject *key,
                                          Py_hash_t hash);
PyAPI_FUNC(int) _PyDict_DelItemIf(PyObject *mp, PyObject *key,
                                  int (*predicate)(PyObject *value));
PyDictKeysObject *_PyDict_NewKeysForClass(void);
PyAPI_FUNC(PyObject *) PyObject_GenericGetDict(PyObject *, void *);
PyAPI_FUNC(int) _PyDict_Next(
    PyObject *mp, Py_ssize_t *pos, PyObject **key, PyObject **value, Py_hash_t *hash);

/* Get the number of items of a dictionary. */
#define PyDict_GET_SIZE(mp)  (assert(PyDict_Check(mp)),((PyDictObject *)mp)->ma_used)
PyAPI_FUNC(int) _PyDict_Contains(PyObject *mp, PyObject *key, Py_hash_t hash);
PyAPI_FUNC(PyObject *) _PyDict_NewPresized(Py_ssize_t minused);
PyAPI_FUNC(void) _PyDict_MaybeUntrack(PyObject *mp);
PyAPI_FUNC(int) _PyDict_HasOnlyStringKeys(PyObject *mp);
Py_ssize_t _PyDict_KeysSize(PyDictKeysObject *keys);
PyAPI_FUNC(Py_ssize_t) _PyDict_SizeOf(PyDictObject *);
PyAPI_FUNC(PyObject *) _PyDict_Pop(PyObject *, PyObject *, PyObject *);
PyObject *_PyDict_Pop_KnownHash(PyObject *, PyObject *, Py_hash_t, PyObject *);
PyObject *_PyDict_FromKeys(PyObject *, PyObject *, PyObject *);
#define _PyDict_HasSplitTable(d) ((d)->ma_values != NULL)

PyAPI_FUNC(int) PyDict_ClearFreeList(void);

/* Like PyDict_Merge, but override can be 0, 1 or 2.  If override is 0,
   the first occurrence of a key wins, if override is 1, the last occurrence
   of a key wins, if override is 2, a KeyError with conflicting key as
   argument is raised.
*/
PyAPI_FUNC(int) _PyDict_MergeEx(PyObject *mp, PyObject *other, int override);
PyAPI_FUNC(PyObject *) _PyDict_GetItemId(PyObject *dp, struct _Py_Identifier *key);
PyAPI_FUNC(int) _PyDict_SetItemId(PyObject *dp, struct _Py_Identifier *key, PyObject *item);

PyAPI_FUNC(int) _PyDict_DelItemId(PyObject *mp, struct _Py_Identifier *key);
PyAPI_FUNC(void) _PyDict_DebugMallocStats(FILE *out);

int _PyObjectDict_SetItem(PyTypeObject *tp, PyObject **dictptr, PyObject *name, PyObject *value);
PyObject *_PyDict_LoadGlobal(PyDictObject *, PyDictObject *, PyObject *);

/* _PyDictView */

typedef struct {
    PyObject_HEAD
    PyDictObject *dv_dict;
} _PyDictViewObject;

PyAPI_FUNC(PyObject *) _PyDictView_New(PyObject *, PyTypeObject *);
PyAPI_FUNC(PyObject *) _PyDictView_Intersect(PyObject* self, PyObject *other);

#ifdef __cplusplus
}
#endif
