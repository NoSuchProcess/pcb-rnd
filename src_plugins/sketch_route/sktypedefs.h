#ifndef SKTYPEDEFS_H
#define SKTYPEDEFS_H

typedef enum {
  SIDE_LEFT = (1<<0),
  SIDE_RIGHT = (1<<1),
  SIDE_TERM = (1<<1)|(1<<0)
} side_t;

typedef struct spoke_s spoke_t;
typedef struct pointdata_s pointdata_t;
typedef struct wirelist_node_s wirelist_node_t;

#endif
