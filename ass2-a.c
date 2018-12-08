/* Author: James Barnes (barnesj2, 820946) */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>


#define UNDEF_VAL   999         /* sentinel value for street time values */
#define GROWTH_MUL  1.618       /* growth factor for array sizes, phi */
#define CARD_DIRS   4           /* cardinal    */
#define EAST        0           /* directions  */
#define NORTH       1           /*    NORTH    */
#define WEST        2           /* WEST + EAST */
#define SOUTH       3           /*    SOUTH    */
#define BORDER_CNR  "+----+"	/* strings */
#define BORDER_TOP  "--------+"	/* for     */
#define BORDER_SIDE "| "        /* the     */
#define ARROW_EAST  " >>>>"     /* route   */
#define ARROW_NORTH "   ^"      /* map     */
#define ARROW_WEST  " <<<<"     /* printed */
#define ARROW_SOUTH "   v"      /* in      */
#define BLANK_LAT   "     "     /* stage   */
#define BLANK_LON   "    "      /* 3       */


/* TYPEDEFS */
typedef struct node_t node_t;
typedef struct edge_t edge_t;
typedef struct city_t city_t;
typedef struct array_t array_t;


/* FUNCTION PROTOTYPES */
void 		print_stage_1(city_t*);
void 		print_stage_2(city_t*);
void 		print_stage_3(city_t*);
void		print_arrow(node_t*, node_t*, int);
city_t*		read_city_data();
void		find_paths(array_t*, array_t*);
void*		safe_malloc(size_t);
void*		safe_realloc(void*, size_t);
array_t*	array_new(size_t);
array_t*	array_insert(void*, array_t*, int);
void*		array_remove(int, array_t*);


/* STRUCTS */
struct node_t
{
	char *name, y;
	int x;
	array_t *out; 	/* outgoing edges of node */
	node_t *via; 	/* previous node in the path to the node */
	int cost;    	/* total cost of the path to node */
};

struct edge_t
{
	node_t *to, *from;
	int weight, dir;
};

struct city_t
{	
	array_t *cnrs; 	/* corners (intersections) in the city */
	array_t *locs; 	/* locations of taxis in the city */
	int x_dim, y_dim, total_secs, unusable_count;
};

struct array_t
{
	void **items;
	int count;		/* number of items in items */
	size_t size;	/* allocated size of items */
};


/* FUNCTIONS */
int main(int argc, char *argv[])
{
	/* read data from stdin */
	city_t *city = read_city_data();
	
	print_stage_1(city);
	print_stage_2(city);
	print_stage_3(city);
	
	/* free city memory */
	int i;
	for (i = 0; i < city->cnrs->count; i++)
	{
		node_t *cnr = city->cnrs->items[i];
		free(cnr->name);
		free(cnr->out->items);
		free(cnr->out);
	}
	free(city->cnrs->items);
	free(city->cnrs);
	free(city->locs->items);
	free(city->locs);
	free(city);
	
	return 0;
}


void print_stage_1(city_t *city)
{
	printf("S1: grid is %d x %d, and has %d intersections\n",
		city->x_dim, city->y_dim, city->cnrs->count);
	printf("S1: of %d possibilities, %d of them cannot be used\n",
		CARD_DIRS * city->cnrs->count, city->unusable_count);
	printf("S1: total cost of remaining possibilities is %d seconds\n",
		city->total_secs);
	printf("S1: %d grid locations supplied",
		city->locs->count);
	if (city->locs->count)
	{
		printf(", first one is %s, last one is %s",
			((node_t*)city->locs->items[0])->name, 
			((node_t*)city->locs->items[city->locs->count - 1])->name);
	}
	printf("\n\n");
}

void print_stage_2(city_t *city)
{
	int i, j;
	node_t *cnr;
	array_t *locs = city->locs;
	array_t *arr;
	
	if (locs->count)
	{
		arr = array_insert(locs->items[0], array_new(1), 0);
		/* find the paths from the first location and each other location */
		find_paths(city->cnrs, arr);
		free(arr->items);
		free(arr);
		for (i = 1; i < locs->count; i++)
		{
			if ((cnr = locs->items[i])->via)
			{
				arr = array_new(1);
				/* trace backwards from the end, to the start, 
				 * adding the corners to a list */
				while (cnr->via)
				{
					array_insert(cnr, arr, 0);
					cnr = cnr->via;
				}
				printf("S2: start at grid %s, cost of %d\n", 
					cnr->name, cnr->cost);
				j = arr->count;
				while (j > 0 && (cnr = arr->items[--j]))
				{
					printf("S2:%7cthen to %s, cost of %d\n", 
						' ', cnr->name, cnr->cost);
				}
				free(arr->items);
				free(arr);
			}
		}
	}
}

void print_stage_3(city_t *city)
{
	int i, j, k, index, x = city->x_dim, y = city->y_dim;
	node_t *cnr;
	array_t *cnrs = city->cnrs;
	
	/* find the shortest route to each corner via one of the locations */
	find_paths(cnrs, city->locs);
	
	printf("\nS3:");
	for (i = 0; i < x; i++) 
	{
		printf("%9d", i);
	}
	printf("\nS3:%9s", BORDER_CNR);
	for (i = 1; i < x; i++) 
	{
		printf(BORDER_TOP);
	}
	for (i = 0; i < y; i++)
	{
		printf("\nS3:%2c%3s", i + 'a', BORDER_SIDE);
		for (j = 0; j < x; j++)
		{
			/* print the arrow to/from the west */
			cnr = cnrs->items[(index = i * x + j)];
			if (j > 0)
			{
				print_arrow(cnr, cnrs->items[index - 1], WEST); 
			}
			printf("%4d", cnr->cost);
		}
		if (i + 1 != y) 
		{
			for (k = 0; k < 2; k++)
			{
				printf("\nS3:%5s", BORDER_SIDE);
				for (j = 0; j < x; j++)
				{
					/* print the arrow to/from the south */
					cnr = cnrs->items[(index = i * x + j)];
					if (index + x < x * y) 
					{
						print_arrow(cnr, cnrs->items[index + x], SOUTH);
					}
					if (j < x - 1) 
					{
						printf(BLANK_LAT); 
					}
				}
			}
		}
	}
	printf("\n\n");
}

/* prints the arrow corresponding to the direction the edge flows, if the edge
 * is used in the city routing. for stage 3 route map */
void print_arrow(node_t *cnr_1, node_t *cnr_2, int dir)
{
	char *arrow_to, *arrow_from, *blank;
	
	arrow_to = (dir == EAST)  ? ARROW_EAST  :
	           (dir == NORTH) ? ARROW_NORTH :
	           (dir == WEST)  ? ARROW_WEST  :
	                            ARROW_SOUTH;
	arrow_from = (dir == EAST)  ? ARROW_WEST  :
	             (dir == NORTH) ? ARROW_SOUTH :
	             (dir == WEST)  ? ARROW_EAST  :
	                              ARROW_NORTH;
	blank = (dir == EAST || dir == WEST) ? BLANK_LAT : 
	                                       BLANK_LON;
	printf((cnr_2->via && cnr_2->via == cnr_1) ? arrow_to   :
	       (cnr_1->via && cnr_1->via == cnr_2) ? arrow_from :
	                                             blank);
}

/* read from stdin: x_dim y_dim [corner_name [4 travel_times]] [locations],
 * and use this to build our city */
city_t* read_city_data()
{
	int i, j, secs, x, y, x_coord;
	char y_coord;
	node_t *cnr;
	edge_t *st;
	city_t *city = (city_t*)safe_malloc(sizeof(city_t));
	
	/* read the city dimensions */
	if (scanf(" %d %d", &x, &y) < 0)
	{
		exit(EXIT_FAILURE);
	}
	city->x_dim = x;
	city->y_dim = y;
	
	/* initialise the city */
	array_t *cnrs = array_new(x * y);
	array_t *locs = array_new(1);
	for (i = 0; i < x * y; i++)
	{
		cnr = (node_t*)safe_malloc(sizeof(node_t));
		cnr->out = array_new(CARD_DIRS);
		cnr->via = NULL;
		cnr->cost = 0;
		/* max space required for the name */
		cnr->name = (char*)safe_malloc(sizeof(char) * (log10(x + 1) + 3));
		array_insert(cnr, cnrs, -1);
	}
	city->total_secs = city->unusable_count = 0;
	                                                        
	/* read data for corner and street info. it is assumed that all city
	 * corners are present in stdin */
	for (i = 0; i < cnrs->count; i++)
	{
		cnr = cnrs->items[i];
		if (scanf(" %s", cnr->name) < 0)
		{
			exit(EXIT_FAILURE);
		};
		cnr->x = atoi(cnr->name);
		cnr->y = cnr->name[strlen(cnr->name) - 1];
		
		/* create the outgoing edges from corner that have valid times */
		for (j = 0; j < CARD_DIRS; j++)
		{
			if (scanf(" %d" , &secs) < 0)
			{
				exit(EXIT_FAILURE);
			}
			if (secs == UNDEF_VAL) 
			{
				city->unusable_count++;
			}
			else
			{
				st = (edge_t*)safe_malloc(sizeof(edge_t));
				city->total_secs += (st->weight = secs);
				/* get the index of the corner in the current direction */
				st->to = cnrs->items[(j == EAST)  ? i + 1 :
				                     (j == NORTH) ? i - x :
				                     (j == WEST)  ? i - 1 : 
				                                    i + x];
				st->from = cnr;
				array_insert(st, cnr->out, -1);
			}
		}
	}
	/* read corner names of locations */
	while (scanf(" %d%c", &x_coord, &y_coord) > 0)
	{
		i = (y_coord - 'a') * x + x_coord;
		array_insert(cnrs->items[i], locs, -1);
	}
	city->cnrs = cnrs;
	city->locs = locs;
	return city;
}

/* find the shortest paths to all nodes from any start.
 * this is a version of Dijkstra's algorithm, modified to allow for
 * multiple starting nodes */
void find_paths(array_t *nodes, array_t *starts)
{
	int i, new_cost, x, y;
	node_t *cur_node, *node;
	edge_t *edge;
	array_t *to_check = array_new(1);
	
	/* initialise node data */
	for (i = 0; i < nodes->count; i++)
	{
		(node = nodes->items[i])->via = NULL;
		node->cost = UNDEF_VAL;
	}
	for (i = 0; i < starts->count; i++)
	{
		(node = starts->items[i])->cost = 0;
		array_insert(node, to_check, -1);
	}
	while ((cur_node = array_remove(0, to_check)))
	{
		x = cur_node->x;
		y = cur_node->y;
		/* check each outgoing edge */
		for (i = 0; i < cur_node->out->count; i++)
		{
			edge = cur_node->out->items[i];
			node = edge->to;
			new_cost = cur_node->cost + edge->weight;
			/* new lowest cost path to node */
			if (new_cost < node->cost)
			{
				node->via = cur_node;
				node->cost = new_cost;
				array_insert(node, to_check, -1);
			}
			/* cur_node is lexographically lower and cost is equal */
			else if (node->via && new_cost == node->cost &&
				(x < node->via->x || (x == node->via->x && y < node->via->x)))
			{
				node->via = cur_node;
			}
		}
	}
	free(to_check->items);
	free(to_check);
}

void* safe_malloc(size_t size)
{
	void *ptr = malloc(size * sizeof(void*));
	assert(ptr);
	return ptr;
}

void* safe_realloc(void *ptr, size_t size)
{
	assert((ptr = realloc(ptr, size)));
	return ptr;
}

/* ARRAY_T FUNCTIONS */
/* malloc and return a pointer to a array_t with preallocated space (size) */
array_t* array_new(size_t size)
{
	array_t *list = (array_t*)safe_malloc(sizeof(array_t));
	list->size = (size > 0) ? size : 1;
	list->items = (void**)safe_malloc(list->size * sizeof(void*));
	list->count = 0;
	return list;
}

/* insert item into the final position in the list, and return the list */
array_t* array_insert(void *item, array_t *list, int index)
{
	int i = list->count++;
	if (list->count >= list->size)
	{
		list->size = ceil(list->size * GROWTH_MUL);
		list->items = (void**)safe_realloc(list->items, 
									list->size * sizeof(void*));
	}
	if (index >= 0 && index < list->count) 
	{
		while (i > index + 1)
		{
			list->items[i] = list->items[i - 1];
			i--;
		}
	}
	list->items[i] = item;
	return list;
}

/* remove the index'th item from list, then return that item.
 * if index == -1, return the last item,
 * else if index is out of the bounds of the list, return NULL */
void* array_remove(int index, array_t *list)
{
	if (list->count == 0 || index < -1 || index >= list->count) 
	{
		return NULL;
	}
	int i = (index < -1) ? list->count - 1 : index;
	void *item = list->items[i];
	if (i != list->count - 1)
	{
		/* fill the void left by the item with the subsequent items */
		for (i = index; i >= 0 && i < list->count; i++) 
		{
			list->items[i] = list->items[i + 1];
		}
	}
	list->items[--list->count] = NULL;
	return item;
}


/* algorithms are fun */