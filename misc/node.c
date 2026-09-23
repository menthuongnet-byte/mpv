#include "node.h"

#include <mpv/client.h>

#include "common/common.h"
#include "bstr.h"

// Init a node with the given format. If parent is not NULL, it is set as
// parent allocation according to m_option_type_node rules (which means
// the domi_vid_node_list allocs are used for chaining the TA allocations).
// format == domi_vid_FORMAT_NONE will simply initialize it with all-0.
void node_init(struct domi_vid_node *dst, int format, struct domi_vid_node *parent)
{
    // Other formats need to be initialized manually.
    mp_assert(format == domi_vid_FORMAT_NODE_MAP || format == domi_vid_FORMAT_NODE_ARRAY ||
           format == domi_vid_FORMAT_FLAG || format == domi_vid_FORMAT_INT64 ||
           format == domi_vid_FORMAT_DOUBLE || format == domi_vid_FORMAT_BYTE_ARRAY ||
           format == domi_vid_FORMAT_NONE);

    void *ta_parent = NULL;
    if (parent) {
        mp_assert(parent->format == domi_vid_FORMAT_NODE_MAP ||
               parent->format == domi_vid_FORMAT_NODE_ARRAY);
        ta_parent = parent->u.list;
    }

    *dst = (struct domi_vid_node){ .format = format };
    if (format == domi_vid_FORMAT_NODE_MAP || format == domi_vid_FORMAT_NODE_ARRAY)
        dst->u.list = talloc_zero(ta_parent, struct domi_vid_node_list);
    if (format == domi_vid_FORMAT_BYTE_ARRAY)
        dst->u.ba = talloc_zero(ta_parent, struct domi_vid_byte_array);
}

// Add an entry to a domi_vid_FORMAT_NODE_ARRAY.
// m_option_type_node memory management rules apply.
struct domi_vid_node *node_array_add(struct domi_vid_node *dst, int format)
{
    struct domi_vid_node_list *list = dst->u.list;
    mp_assert(dst->format == domi_vid_FORMAT_NODE_ARRAY && dst->u.list);
    MP_TARRAY_GROW(list, list->values, list->num);
    node_init(&list->values[list->num], format, dst);
    return &list->values[list->num++];
}

// Add an entry to a domi_vid_FORMAT_NODE_MAP. Keep in mind that this does
// not check for already existing entries under the same key.
// m_option_type_node memory management rules apply.
struct domi_vid_node *node_map_add(struct domi_vid_node *dst, const char *key, int format)
{
    mp_assert(key);
    return node_map_badd(dst, bstr0(key), format);
}

struct domi_vid_node *node_map_badd(struct domi_vid_node *dst, struct bstr key, int format)
{
    mp_assert(key.start);

    struct domi_vid_node_list *list = dst->u.list;
    mp_assert(dst->format == domi_vid_FORMAT_NODE_MAP && dst->u.list);
    MP_TARRAY_GROW(list, list->values, list->num);
    MP_TARRAY_GROW(list, list->keys, list->num);
    list->keys[list->num] = bstrdup0(list, key);
    node_init(&list->values[list->num], format, dst);
    return &list->values[list->num++];
}

// Add a string entry to a domi_vid_FORMAT_NODE_MAP. Keep in mind that this does
// not check for already existing entries under the same key.
// m_option_type_node memory management rules apply.
void node_map_add_string(struct domi_vid_node *dst, const char *key, const char *val)
{
    mp_assert(val);

    struct domi_vid_node *entry = node_map_add(dst, key, domi_vid_FORMAT_NONE);
    entry->format = domi_vid_FORMAT_STRING;
    entry->u.string = talloc_strdup(dst->u.list, val);
}

void node_map_add_bstr(struct domi_vid_node *dst, const char *key, bstr val)
{
    mp_assert(val.start);

    struct domi_vid_node *entry = node_map_add(dst, key, domi_vid_FORMAT_NONE);
    entry->format = domi_vid_FORMAT_STRING;
    entry->u.string = bstrto0(dst->u.list, val);
}

void node_map_add_int64(struct domi_vid_node *dst, const char *key, int64_t v)
{
    node_map_add(dst, key, domi_vid_FORMAT_INT64)->u.int64 = v;
}

void node_map_add_double(struct domi_vid_node *dst, const char *key, double v)
{
    node_map_add(dst, key, domi_vid_FORMAT_DOUBLE)->u.double_ = v;
}

void node_map_add_flag(struct domi_vid_node *dst, const char *key, bool v)
{
    node_map_add(dst, key, domi_vid_FORMAT_FLAG)->u.flag = v;
}

domi_vid_node *node_map_get(domi_vid_node *src, const char *key)
{
    return node_map_bget(src, bstr0(key));
}

domi_vid_node *node_map_bget(domi_vid_node *src, struct bstr key)
{
    if (src->format != domi_vid_FORMAT_NODE_MAP)
        return NULL;

    for (int i = 0; i < src->u.list->num; i++) {
        if (bstr_equals0(key, src->u.list->keys[i]))
            return &src->u.list->values[i];
    }

    return NULL;
}

// Note: for domi_vid_FORMAT_NODE_MAP, this (incorrectly) takes the order into
//       account, instead of treating it as set.
bool equal_domi_vid_value(const void *a, const void *b, int format)
{
    switch (format) {
    case domi_vid_FORMAT_NONE:
        return true;
    case domi_vid_FORMAT_STRING:
    case domi_vid_FORMAT_OSD_STRING:
        return strcmp(*(char **)a, *(char **)b) == 0;
    case domi_vid_FORMAT_FLAG:
        return *(int *)a == *(int *)b;
    case domi_vid_FORMAT_INT64:
        return *(int64_t *)a == *(int64_t *)b;
    case domi_vid_FORMAT_DOUBLE:
        return *(double *)a == *(double *)b;
    case domi_vid_FORMAT_NODE:
        return equal_domi_vid_node(a, b);
    case domi_vid_FORMAT_BYTE_ARRAY: {
        const struct domi_vid_byte_array *a_r = a, *b_r = b;
        if (a_r->size != b_r->size)
            return false;
        return memcmp(a_r->data, b_r->data, a_r->size) == 0;
    }
    case domi_vid_FORMAT_NODE_ARRAY:
    case domi_vid_FORMAT_NODE_MAP:
    {
        domi_vid_node_list *l_a = *(domi_vid_node_list **)a, *l_b = *(domi_vid_node_list **)b;
        if (l_a->num != l_b->num)
            return false;
        for (int n = 0; n < l_a->num; n++) {
            if (format == domi_vid_FORMAT_NODE_MAP) {
                if (strcmp(l_a->keys[n], l_b->keys[n]) != 0)
                    return false;
            }
            if (!equal_domi_vid_node(&l_a->values[n], &l_b->values[n]))
                return false;
        }
        return true;
    }
    }
    MP_ASSERT_UNREACHABLE(); // supposed to be able to handle all defined types
}

// Remarks see equal_domi_vid_value().
bool equal_domi_vid_node(const struct domi_vid_node *a, const struct domi_vid_node *b)
{
    if (a->format != b->format)
        return false;
    return equal_domi_vid_value(&a->u, &b->u, a->format);
}
