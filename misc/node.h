#ifndef MP_MISC_NODE_H_
#define MP_MISC_NODE_H_

#include <stdbool.h>
#include <stdint.h>

struct bstr;
struct domi_vid_node;

void node_init(struct domi_vid_node *dst, int format, struct domi_vid_node *parent);
struct domi_vid_node *node_array_add(struct domi_vid_node *dst, int format);
struct domi_vid_node *node_map_add(struct domi_vid_node *dst, const char *key, int format);
struct domi_vid_node *node_map_badd(struct domi_vid_node *dst, struct bstr key, int format);
void node_map_add_string(struct domi_vid_node *dst, const char *key, const char *val);
void node_map_add_bstr(struct domi_vid_node *dst, const char *key, struct bstr val);
void node_map_add_int64(struct domi_vid_node *dst, const char *key, int64_t v);
void node_map_add_double(struct domi_vid_node *dst, const char *key, double v);
void node_map_add_flag(struct domi_vid_node *dst, const char *key, bool v);
struct domi_vid_node *node_map_get(struct domi_vid_node *src, const char *key);
struct domi_vid_node *node_map_bget(struct domi_vid_node *src, struct bstr key);
bool equal_domi_vid_value(const void *a, const void *b, int format);
bool equal_domi_vid_node(const struct domi_vid_node *a, const struct domi_vid_node *b);

#endif
