#ifndef ITIDY_EXEC_LIST_COMPAT_H
#define ITIDY_EXEC_LIST_COMPAT_H

#include <exec/types.h>
#include <exec/lists.h>

/*
 * Some VBCC builds do not expose the NewList() macro even after including
 * exec/lists.h. Provide a safe fallback so files can rely on the helper
 * without triggering implicit declaration warnings.
 */
#ifndef NewList
#define NewList(list_ptr)                               \
    do {                                                \
        struct List *_list = (list_ptr);                \
        _list->lh_Head = (struct Node *)&_list->lh_Tail;\
        _list->lh_Tail = NULL;                          \
        _list->lh_TailPred = (struct Node *)&_list->lh_Head; \
    } while (0)
#endif

#endif /* ITIDY_EXEC_LIST_COMPAT_H */
