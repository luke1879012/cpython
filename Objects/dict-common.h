#ifndef Py_DICT_COMMON_H
#define Py_DICT_COMMON_H

typedef struct {
    /* Cached hash code of me_key. */
    // 哈希值
    Py_hash_t me_hash;
    // 指向键
    PyObject *me_key;
    // 指向值
    PyObject *me_value; /* This field is only meaningful for combined tables */
} PyDictKeyEntry;

/* dict_lookup_func() returns index of entry which can be used like DK_ENTRIES(dk)[index].
 * -1 when no entry found, -3 when compare raises error.
 */
typedef Py_ssize_t (*dict_lookup_func)
    (PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject **value_addr);

// 索引槽的标志
#define DKIX_EMPTY (-1)
#define DKIX_DUMMY (-2)  /* Used internally */
#define DKIX_ERROR (-3)

/* See dictobject.c for actual layout of DictKeysObject */
struct _dictkeysobject {
    // key的引用计数，也就是key被多少字典所使用。
    // 如果是结合表，那么该成员始终是1，因为结合表独占一组key
    // 如果是分离表，那么该成员大于等于1，因为分离表可以共享一组key
    Py_ssize_t dk_refcnt;

    /* Size of the hash table (dk_indices). It must be a power of 2. */
    // 哈希表大小，并且大小是2的n次方，这样可将模运算优化成按位与运算
    Py_ssize_t dk_size;

    /* Function to lookup in the hash table (dk_indices):

       - lookdict(): general-purpose, and may return DKIX_ERROR if (and
         only if) a comparison raises an exception.

       - lookdict_unicode(): specialized to Unicode string keys, comparison of
         which can never raise an exception; that function can never return
         DKIX_ERROR.

       - lookdict_unicode_nodummy(): similar to lookdict_unicode() but further
         specialized for Unicode string keys that cannot be the <dummy> value.

       - lookdict_split(): Version of lookdict() for split tables. */
    // 哈希函数，用于计算key的哈希值，然后映射成索引
    // 一个好的哈希函数应该能尽量少的避免冲突，并且哈希函数对哈希表的性能起着至关重要的作用
    // 所以底层的哈希函数有很多种，会根据对象的种类选择最合适的一个
    dict_lookup_func dk_lookup;

    /* Number of usable entries in dk_entries. */
    // 键值对数组的长度
    Py_ssize_t dk_usable;

    /* Number of used entries in dk_entries. */
    // 哈希表中已使用的entry数量
    // 这个entry你可以理解为键值对，一个entry就是一个键值对
    Py_ssize_t dk_nentries;

    /* Actual hash table of dk_size entries. It holds indices in dk_entries,
       or DKIX_EMPTY(-1) or DKIX_DUMMY(-2).

       Indices must be: 0 <= indice < USABLE_FRACTION(dk_size).

       The size in bytes of an indice depends on dk_size:

       - 1 byte if dk_size <= 0xff (char*)
       - 2 bytes if dk_size <= 0xffff (int16_t*)
       - 4 bytes if dk_size <= 0xffffffff (int32_t*)
       - 8 bytes otherwise (int64_t*)

       Dynamically sized, SIZEOF_VOID_P is minimum. */
    // 哈希索引数组
    char dk_indices[];  /* char is required to avoid strict aliasing. */

    /* "PyDictKeyEntry dk_entries[dk_usable];" array follows:
       see the DK_ENTRIES() macro */
    // dk_entries: 键值对数组，类型是PyDictKeyEntry类型的数组，用于存储键值对
};

#endif
