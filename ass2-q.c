/* Author: James Barnes (barnesj2, 820946) */

/* ~~LIBRARIES~~ */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>


/* ~~MACROS~~ */
#define MAX_SECS    999         /* sentinel value for street time values */
#define GROWTH_MUL  2           /* growth factor for array sizes */
#define CARD_DIRS   4           /* cardinal directions */
#define EAST        0
#define NORTH       1
#define WEST        2
#define SOUTH       3
#define BORDER_CNR  "+----+"	/* route map (stage 3) strings/lengths */
#define BORDER_TOP  "--------+"	
#define BORDER_SIDE " | "
#define ARROW_WEST  " <<<<"
#define ARROW_EAST  " >>>>"
#define BLANK_LAT   "     "
#define ARROW_SOUTH "   v"
#define ARROW_NORTH "   ^"
#define BLANK_LON   "    "
#define LON_LEN     2


/* ~~TYPEDEFS~~ */
typedef struct node_t  node_t;
typedef struct edge_t  edge_t;
typedef struct city_t  city_t;
typedef struct list_t  list_t;
typedef struct queue_t queue_t;
typedef struct elem_t  elem_t;


/* ~~FUNCTION PROTOTYPES~~ */
void     print_stage_1(city_t*);
void     print_stage_2(city_t*);
void     print_stage_3(city_t*);
city_t*  read_city_data();
int 	 dir_offset(int, int);
void     find_paths(list_t*, list_t*);
list_t* new_list(size_t);
list_t* list_insert(void*, list_t*, int);
void*    list_remove(int, list_t*);
void     clear_list(list_t*);
queue_t* new_queue();
queue_t* enqueue(void*, queue_t*);
void*    dequeue(queue_t*);
void     clear_queue(queue_t*);
void*    safe_malloc(size_t);
void*    safe_realloc(void*, size_t);


/* ~~STRUCTS~~ */
struct city_t
{
	list_t *cnrs;  /* corners (intersections) in the city */
	list_t *locs;  /* locations of taxis in the city */
	int x_dim, y_dim, total_secs, unusable;
};

struct node_t
{
	char *name;
	int x, y;
	list_t *out;    /* outgoing edges of node */
	node_t *via;    /* previous node in the path to the node */
	int cost;       /* net cost of the path to node */
};

struct edge_t
{
	node_t *to, *from;
	int weight;
};

/* basically an augmented array, which is more user-friendly, allowing for
   resizing on insertion, and storage of the length, etc. */
struct list_t
{
	void **items;   /* storage component, an array */
	int len;		/* number of items in items */
	size_t size;	/* allocated size of items */
};

struct queue_t
{
	elem_t *first, *last;
	size_t len;
};

struct elem_t
{
	void *data;
	elem_t *prev, *next;
};


/* ~~FUNCTIONS~~ */
int main(int argc, char *argv[])
{
	int i;
	node_t *cnr;

	/* read data from stdin */
	city_t *city = read_city_data();

	print_stage_1(city);
	print_stage_2(city);
	print_stage_3(city);

	/* free city memory */
	for (i = 0; i < city->cnrs->len; i++)
	{
		free((cnr = city->cnrs->items[i])->name);
		clear_list(cnr->out);
		free(cnr->out);
		free(cnr);
	}
	cnr = NULL;
	clear_list(city->cnrs);
	free(city->cnrs);
	clear_list(city->locs);
	free(city->locs);
	free(city);
	city = NULL;

	return 0;
}

void print_stage_1(city_t *city)
{
	printf("S1: grid is %d x %d, and has %d intersections\n",
		city->x_dim, city->y_dim, city->cnrs->len);
	printf("S1: of %d possibilities, %d of them cannot be used\n",
		CARD_DIRS * city->cnrs->len, city->unusable);
	printf("S1: total cost of remaining possibilities is %d seconds\n",
		city->total_secs);
	printf("S1: %d grid locations supplied",
		city->locs->len);
	if (city->locs->len)
	{
		printf(", first one is %s, last one is %s",
			((node_t*)city->locs->items[0])->name,
			((node_t*)city->locs->items[city->locs->len - 1])->name);
	}
	printf("\n\n");
}

void print_stage_2(city_t *city)
{
	int i;
	node_t *cnr;
	list_t *locs = city->locs;
	list_t *start = list_insert(locs->items[0], new_list(1), 0);
	list_t *path = new_list(1);

	/* find the paths from the first location and each other location */
	find_paths(city->cnrs, start);

	for (i = 1; i < locs->len; i++)
	{
		if ((cnr = locs->items[i])->via)
		{
			/* trace backwards from the end, to the start,
			   adding the corners to a list */
			while (cnr->via)
			{
				list_insert(cnr, path, -1);
				cnr = cnr->via;
			}
			printf("S2: start at grid %s, cost of %d\n",
				cnr->name, cnr->cost);
			while ((cnr = list_remove(-1, path)))
			{
				printf("S2:       then to %s, cost of %d\n",
					cnr->name, cnr->cost);
			}
		}
	}
	clear_list(start);
	free(start);
	start = NULL;
	clear_list(path);
	free(path);
	path = NULL;
}

void print_stage_3(city_t *city)
{
	int x, y, index, x_d = city->x_dim, y_d = city->y_dim, i;
	node_t *cnr, *cnr_2;
	list_t *cnrs = city->cnrs;

	/* find the shortest route to each corner via one of the locations */
	find_paths(cnrs, city->locs);

	printf("\nS3:");
	for (i = 0; i < x_d; i++)
	{
		printf("%9d", i);
	}
	printf("\nS3:   %s", BORDER_CNR);
	for (i = 0; i < x_d - 1; i++)
	{
		printf(BORDER_TOP);
	}
	for (y = 0; y < y_d; y++)
	{
		printf("\nS3: %c%s", y + 'a', BORDER_SIDE);
		for (x = 0; x < x_d; x++)
		{
			/* print the arrow to/from the west */
			cnr = cnrs->items[(index = y * x_d + x)];
			/* check there is a corner to the west */
			if (x > 0)
			{
				cnr_2 = cnrs->items[index + dir_offset(WEST, x_d)];
				printf(cnr_2->via == cnr ? ARROW_WEST :
				       cnr->via == cnr_2 ? ARROW_EAST :
				                           BLANK_LAT);
			}
			printf("%4d", cnr->cost);
		}
		for (i = 0; y + 1 != y_d && i < LON_LEN; i++)
		{
			printf("\nS3:  %s", BORDER_SIDE);
			for (x = 0; x < x_d; x++)
			{
				/* print the arrow to/from the south */
				cnr = cnrs->items[(index = y * x_d + x)];
				/* check there is a corner to the south */
				if (index + x_d < x_d * y_d)
				{
					cnr_2 = cnrs->items[index + dir_offset(SOUTH, x_d)];
					printf(cnr_2->via == cnr ? ARROW_SOUTH :
						   cnr->via == cnr_2 ? ARROW_NORTH :
						                       BLANK_LON);
				}
				if (x < x_d - 1)
				{
					printf(BLANK_LAT);
				}
			}
		}
	}
	printf("\n\n");
}

/* read from stdin: x_dim y_dim [corner name [4 travel times]] [locations],
   and use this to build our city. it is assumed to be valid data */
city_t* read_city_data()
{
	int index, i, dir, secs, x_d, y_d, count = 0;
	node_t *cnr;
	edge_t *st;
	city_t *city = safe_malloc(sizeof(city_t));

	/* read the city dimensions */
	if (scanf(" %d %d", &x_d, &y_d) != 2)
	{
		exit(EXIT_FAILURE);
	}
	city->x_dim = x_d;
	city->y_dim = y_d;

	/* maximum length of a corner name */
	char tmp[(int)ceil(log10(x_d + 1) + 3)];

	/* initialise the city */
	list_t *cnrs = city->cnrs = new_list(x_d * y_d);
	list_t *locs = city->locs = new_list(1);
	for (i = 0; i < x_d * y_d; i++)
	{
		cnr = safe_malloc(sizeof(node_t));
		cnr->out = new_list(CARD_DIRS);
		cnr->via = NULL;
		cnr->cost = 0;
		list_insert(cnr, cnrs, -1);
	}
	city->total_secs = city->unusable = 0;

	/* read data for corner, street travel times and taxi locations */
	while (scanf(" %s", tmp) > 0)
	{
		/* index : x-value + y-value * x-dimension */
		index = atoi(tmp) + (tmp[strlen(tmp) - 1] - 'a') * x_d;
		if (count++ < x_d * y_d)
		{
			cnr = cnrs->items[index];
			cnr->name = safe_malloc((strlen(tmp) + 1) * sizeof(char));
			strcpy(cnr->name, tmp);
			/* integer component of name */
			cnr->x = atoi(cnr->name);
			/* alphabetic component of name */
			cnr->y = (int)(cnr->name[strlen(cnr->name) - 1] - 'a');

			/* read street times */
			for (dir = 0; dir < CARD_DIRS; dir++)
			{
				if (scanf(" %d" , &secs) < 0)
				{
					/* no value read */
					exit(EXIT_FAILURE);
				}
				if (secs == MAX_SECS)
				{
					/* street is unusable */
					city->unusable++;
				}
				else
				{
					/* we have a valid street */
					st = safe_malloc(sizeof(edge_t));
					city->total_secs += (st->weight = secs);
					/* corner in the current dir, by index */
					st->to = cnrs->items[index + dir_offset(dir, x_d)];
					st->from = cnr;
					list_insert(st, cnr->out, -1);
				}
			}
		}
		else
		{
			/* read all of the corner data, now onto the taxis */
			list_insert(cnrs->items[index], locs, -1);
		}
	}
	return city;
}

/* return the index offset in a given dirention */
int dir_offset(int dir, int x_dim)
{
	return (dir == EAST) ? 1 :
		   (dir == NORTH)? - x_dim :
		   (dir == WEST) ? - 1 :
						   x_dim;
}

/* find the shortest paths to all nodes from any start.
   this is a version of Dijkstra's algorithm (1956), modified to allow for
   multiple starting nodes */
void find_paths(list_t *nodes, list_t *starts)
{
	int i, new_cost, x, y;
	node_t *cur_node, *node;
	edge_t *edge;
	queue_t *to_check = new_queue();

	/* initialise node data */
	for (i = 0; i < nodes->len; i++)
	{
		(node = nodes->items[i])->via = NULL;
		node->cost = MAX_SECS;
	}
	for (i = 0; i < starts->len; i++)
	{
		(node = starts->items[i])->cost = 0;
		enqueue(node, to_check);
	}
	
	while ((cur_node = dequeue(to_check)))
	{
		x = cur_node->x;
		y = cur_node->y;
		/* check each outgoing edge */
		for (i = 0; i < cur_node->out->len; i++)
		{
			edge = cur_node->out->items[i];
			node = edge->to;
			new_cost = cur_node->cost + edge->weight;
			/* lower cost path to node */
			if (new_cost < node->cost)
			{
				node->via = cur_node;
				node->cost = new_cost;
				enqueue(node, to_check);
			}
			/* cur_node new_cost is equal and is lexographically lower */
			else if (new_cost == node->cost && (x < node->via->x || 
				(x == node->via->x && y < node->via->y)))
			{
				node->via = cur_node;
			}
		}
	}
	clear_queue(to_check);
	free(to_check);
	to_check = NULL;
}

/* ~LIST_T FUNCTIONS~ */
/* malloc and return a pointer to an list_t with preallocated space */
list_t* new_list(size_t size)
{
	list_t *list = safe_malloc(sizeof(list_t));
	list->size = (size > 0) ? size : 1;
	list->items = safe_malloc(list->size * sizeof(void*));
	list->len = 0;
	return list;
}

/* insert item into the list maintaining order, and return a pointer to
   the list_t.
   if the index is out of the bounds of the list, insert at the end */
list_t* list_insert(void *item, list_t *list, int index)
{
	int i = list->len++;
	if (list->len >= list->size)
	{
		list->size = (list->size > 0) ? ceil(list->size * GROWTH_MUL) : 1;
		list->items = safe_realloc(list->items, list->size * sizeof(void*));
	}
	if (index >= 0 && index < list->len)
	{
		/* shift items up to make space for the new item */
		while (i > index + 1)
		{
			list->items[i] = list->items[i - 1];
			i--;
		}
	}
	list->items[i] = item;
	return list;
}

/* remove the indexth item from list, then return that item.
   if index is -1, return the last item,
   else if index is out of the bounds of the list, return NULL */
void* list_remove(int index, list_t *list)
{
	if (list->len == 0 || index < -1 || index >= list->len)
	{
		return NULL;
	}
	int i = (index == -1) ? list->len - 1 : index;
	void *item = list->items[i];
	if (i != list->len - 1)
	{
		/* fill the void left by the item with the subsequent items */
		for (i = index; i >= 0 && i < list->len; i++)
		{
			list->items[i] = list->items[i + 1];
		}
	}
	list->items[--list->len] = NULL;
	return item;
}

void clear_list(list_t *list)
{
	free(list->items);
	list->items = NULL;
	list->len = list->size = 0;
}

/* ~QUEUE_T FUNCTIONS~ */
/* malloc and initialise a new queue_t and return a pointer to it */
queue_t* new_queue()
{
	queue_t *q = safe_malloc(sizeof(queue_t));
	q->first = q->last = NULL;
	q->len = 0;
	return q;
}

/* add an item to the end of a queue, and return a pointer to the queue */
queue_t* enqueue(void *data, queue_t *q)
{
	elem_t *new = safe_malloc(sizeof(elem_t));
	new->data = data;
	if (!q->len++)
	{
		/* queue was empty */
		q->first = new;
	}
	else
	{
		q->last->next = new;
	}
	new->prev = q->last;
	q->last = new;
	new->next = NULL;
	return q;
}

/* remove and return the data of the first item in a queue. 
   NULL if queue is empty */
void* dequeue(queue_t *q)
{
	if (!q->len)
	{
		/* queue is empty */
		return NULL;
	}
	if (!(--q->len))
	{
		q->last = NULL;
	}
	elem_t *elem = q->first;
	void *data = elem->data;
	q->first = elem->next;
	free(elem);
	return data;
}

/* frees all elements in a queue */
void clear_queue(queue_t *q)
{
	elem_t *elem, *next = q->first;
	while (next)
	{
		elem = next;
		next = elem->next;
		free(elem);
	}
	q->first = q->last = NULL;
	q->len = 0;
}

/* ~MEMORY ALLOCATION FUNCTIONS~ */
/* malloc, check we got a pointer allocated, and return the new pointer */
void* safe_malloc(size_t size)
{
	void *ptr = malloc(size * sizeof(void*));
	assert(ptr);
	return ptr;
}

/* realloc, check we got a pointer allocated, and return the new pointer */   
void* safe_realloc(void *ptr, size_t size)
{
	assert((ptr = realloc(ptr, size)));
	return ptr;
}

/* algorithms are fun */