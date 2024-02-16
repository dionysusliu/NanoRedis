# Takeaways

## hashtable
1. intrusive data structure for generic datatypes
    - no data encapsulation, therefore flexible
        - developer doesn't care about data type AT ALL
        - users should include developer's data strcture
        - use the macro `container_of` for data retrievel 
            - effectively, locate the frame address of struct by its member address
```c
// for intrusive data structure
#define container_of(ptr, T, member) ({                  \
    const typeof( ((T *)0)->member ) *__mptr = (ptr);    \
    (T *)( (char *)__mptr - offsetof(T, member) );})
```

2. Avoid special edge cases of "NULL" via **pointer indirection**
    - refer to `h_detach` in `src/hashtable.cpp`
        - delete **head** is non-trivial for first-order pointer, but not for the **second order pointer**
    - Use *Second Order pointers* for data structure manipulation
    - Use *First Order pointers* for data retrieval

3. Consider latency and worst cases when handling lots of data
    - Not sure what it means by now 
    - I guess when workload accumulates we encounter **fat tail** in risk distribution (concept borrowed from financials)



## format string
given a byte string, print certain section of it to prompt
```c
    printf("%.*s", len, data[start]);
```
- the `.` represents the *precision* (length) of string, and `*` (wildcard) would be replaced by the length