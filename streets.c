#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "streets.h"
#define INFINITY_COST 1e308
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * "OSM Data Processing Project." ChatGPT, 13 Feb. version, OpenAI, 9 Mar. 2023, chat.openai.com/chat.
 * Also used copilot's autofill features for minor functions.
 * 
*/

/**
 * this is the structure that should record all the information about a node.
*/

struct node {
    double lat;
    double lon;
    int id;
    int osmid;
    int num_ways;
    int *way_ids;
};


/**
 * this is the structure that stores priority node IDs in a priority queue.
*/

typedef struct HeapNode {
    int node_id;     // The node identifier
    double priority; // The node's priority
} HeapNode;


/**
 * this is the structure that follows minheap properties.
*/


typedef struct MinHeap {
    int size;       // Current size of the heap
    int capacity;   // Maximum capacity of the heap
    HeapNode *elements; // Array of heap nodes
} MinHeap;

/**
 * The following are methods that allow for the minheap to implement a Priority Queue.
 * Used CSC263 notes for reference, and Chat-GPT.
 * https://chat.openai.com/share/748cadf8-8358-47e2-a72e-3260fabd87e2
*/

MinHeap *
create_min_heap(int capacity) {
    MinHeap *heap = (MinHeap *)malloc(sizeof(MinHeap));
    heap->size = 0;
    heap->capacity = capacity;
    heap->elements = (HeapNode *)malloc(sizeof(HeapNode) * capacity);
    return heap;
}

void 
swap_heap_nodes(HeapNode *a, HeapNode *b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
}

void 
min_heapify(MinHeap *heap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < heap->size && heap->elements[left].priority < heap->elements[smallest].priority)
        smallest = left;

    if (right < heap->size && heap->elements[right].priority < heap->elements[smallest].priority)
        smallest = right;

    if (smallest != idx) {
        swap_heap_nodes(&heap->elements[idx], &heap->elements[smallest]);
        min_heapify(heap, smallest);
    }
}

HeapNode 
remove_min(MinHeap *heap) {
    if (heap->size <= 0)
        return (HeapNode){-1, 0}; // Return a default value if heap is empty

    HeapNode root = heap->elements[0];
    heap->elements[0] = heap->elements[--heap->size];
    min_heapify(heap, 0);

    return root;
}

void 
decrease_key(MinHeap *heap, int node_id, double priority) {
    // Find the node in the heap
    int i;
    for (i = 0; i < heap->size; i++) {
        if (heap->elements[i].node_id == node_id) {
            heap->elements[i].priority = priority;
            break;
        }
    }
    
    // Move up through the heap and fix disturbed heap property
    while (i != 0 && heap->elements[(i - 1) / 2].priority > heap->elements[i].priority) {
        swap_heap_nodes(&heap->elements[i], &heap->elements[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

void 
insert_into_heap(MinHeap *heap, int node_id, double priority) {
    if (heap->size == heap->capacity) {
        // Heap is full
        return;
    }

    // Insert the new key at the end of the heap
    int i = heap->size++;
    heap->elements[i].node_id = node_id;
    heap->elements[i].priority = priority;

    // Fix the min heap property if it's violated
    while (i != 0 && heap->elements[(i - 1) / 2].priority > heap->elements[i].priority) {
        swap_heap_nodes(&heap->elements[i], &heap->elements[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

void 
destroy_min_heap(MinHeap *heap) {
    free(heap->elements);
    free(heap);
}



/**
 * this is the structure that should record all the information about a way.
*/

struct way {
    int id;
    int osmid;
    char *name;
    float speed_limit;
    bool one_way;
    int num_nodes;
    int *node_ids;
};


/**
 * this is the structure that should keep a list of all node and way objects
*/

struct ssmap {
    int nr_nodes;
    int nr_ways;
    struct node *nodes;
    struct way *ways;

};


/**
 * SSMap is the main structure that stores all OSM nodes and ways.
 * Code from Chat-GPT "https://chat.openai.com/g/g-HgZuFuuBK-professional-coder-auto-programming/c/c19ecb6c-ad05-4186-9db5-a63894b7c183"
*/


struct ssmap * 
ssmap_create(int nr_nodes, int nr_ways)
{
    if (nr_ways <= 0 || nr_nodes <= 0) {
        return NULL;
    }

    // Allocating memory for the ssmap structure
    struct ssmap *map = (struct ssmap *)malloc(sizeof(struct ssmap));
    if (!map) {
        // Out of memory
        return NULL;
    }
    map->nodes = (struct node *)malloc(nr_nodes * sizeof(struct node));
    if (!map->nodes) {
        // Out of memory, clean up space and return NULL
        free(map);
        return NULL;
    }
    map->ways = (struct way *)malloc(nr_ways * sizeof(struct way));
    if (!map->ways) {
        // Out of memory, clean up space and return NULL
        free(map->nodes);
        free(map);
        return NULL;
    }
    map->nr_nodes = nr_nodes;
    map->nr_ways = nr_ways;

    return map;
}

bool
ssmap_initialize(struct ssmap * m)
{
    if (m == NULL) {
        printf("ssmap_initialize: Invalid ssmap pointer.\n");
        return false;
    }

    return true;
}

void
ssmap_destroy(struct ssmap * m)
{
    if (m == NULL) {
        return;
    }
    for (int i = 0; i < m->nr_ways; i++) {
        free(m->ways[i].name);
        free(m->ways[i].node_ids);
    }
    for (int i = 0; i < m->nr_nodes; i++) {
        free(m->nodes[i].way_ids);
    }
    free(m->ways);
    free(m->nodes);
    m->nr_ways = 0;
    m->nr_nodes = 0;
    free(m);

}

struct way * 
ssmap_add_way(struct ssmap * m, int id, const char * name, float maxspeed, bool oneway, 
              int num_nodes, const int node_ids[num_nodes])
{
    if (num_nodes <= 0) {
        return NULL;
    }
    // Allocating memory for a new way structure
    struct way *new_way = &(m->ways[id]);
    new_way->id = id;
    new_way->osmid = -1; // Assuming OSM ID is not used directly in this context
    new_way->name = strdup(name); // Duplicating the name string
    if (!new_way->name) {
        printf("Out of memory when setting name for way ID: %d\n", id);
        return NULL;
    }
    new_way->speed_limit = maxspeed;
    new_way->one_way = oneway;
    new_way->num_nodes = num_nodes;

    // Allocating memory for node_ids array and copy the contents
    new_way->node_ids = (int *)malloc(num_nodes * sizeof(int));
    if (!new_way->node_ids) {
        printf("Out of memory when allocating node IDs for way ID: %d\n", id);
        // Freeing the previously allocated name string to avoid memory leak
        free(new_way->name);
        return NULL;
    }
    for (int i = 0; i < num_nodes; i++) {
        new_way->node_ids[i] = node_ids[i];
    }

    return new_way;

}

struct node * 
ssmap_add_node(struct ssmap * m, int id, double lat, double lon, 
               int num_ways, const int way_ids[num_ways])
{
    // Allocating memory for a new node structure
    struct node *new_node = &(m->nodes[id]);
    new_node->id = id;
    // Assuming OSM ID is not used
    new_node->lat = lat; // init latitude
    new_node->lon = lon; // init longitude
    new_node->num_ways = num_ways;

    // Allocating memory for way_ids array and copy the contents
    if (num_ways > 0) {
        new_node->way_ids = (int *)malloc(num_ways * sizeof(int));
        if (!new_node->way_ids) {
            printf("Out of memory when allocating way IDs for node ID: %d\n", id);
            return NULL;
        }
        for (int i = 0; i < num_ways; i++) {
            new_node->way_ids[i] = way_ids[i];
        }
    } else {
        new_node->way_ids = NULL;
    }

    return new_node;
}

void
ssmap_print_way(const struct ssmap * m, int id)
{
    if (id < 0 || id >= m->nr_ways) {
        printf("error: way %d does not exist.\n", id);
        return;
    }
    printf("Way %d: %s\n", m->ways[id].id, m->ways[id].name);
}

void
ssmap_print_node(const struct ssmap * m, int id)
{
    if (id < 0 || id >= m->nr_nodes) {
        printf("error: node %d does not exist.\n", id);
        return;
    }
    printf("Node %d: (%.7lf, %.7lf)\n", m->nodes[id].id, m->nodes[id].lat, m->nodes[id].lon);
}


/**
 * Find all way objects with a particular keyword in its name and print them.
 *
 * The format of the output should be space separated way ids. e.g.
 * 9 32 105 5 782 96
 *
 * You may assume name will never be a null pointer.
 * 
 * @param m The ssmap structure where the way objects should be located.
 * @param name The keyword that should be used to search for way objects.
 */


void 
ssmap_find_way_by_name(const struct ssmap * m, const char * name)
{
    for (int i = 0; i < m->nr_ways; i++) {
        if (strstr(m->ways[i].name, name) != NULL) {
            printf("%d ", m->ways[i].id);
        }
    }
    printf("\n");
    
}

/**
 * Find all node objects that are associated with way objects that have the 
 * specified keywords in their names. The main purpose of this function
 * is to find a particular intersection, e.g., Yonge and Bloor. A node should
 * only be printed if each keyword matches a distinct way object, e.g., Yonge
 * matches way 42 and Bloor matches way 69. 
 * 
 * The format of the output should be space separated node ids. e.g.
 * 9 32 105 5 782 96
 *
 * You may assume name1 will never be a null pointer, but not name2.
 * 
 * @param m The ssmap structure where the node objects should be located.
 * @param name1 The first keyword that should be used to search for a node.
 * @param name2 The second keyword that should be used to search for a node.
 *              This parameter is allowed to NULL. In which case, the function
 *              displays all nodes with ways that have name1 in their names.
 */


void 
ssmap_find_node_by_names(const struct ssmap * m, const char * name1, const char * name2)
{
    if (name2 == NULL) {
        for (int i = 0; i < m->nr_nodes; i++) {
            // Checking the way name for the keyword of every node id of
            for (int j = 0; j < m->nodes[i].num_ways; j++) {
                if (strstr(m->ways[m->nodes[i].way_ids[j]].name, name1) != NULL) {
                    printf("%d ", m->nodes[i].id);
                    break;
                }
            }
        }
        printf("\n");
    } else {
        for (int i = 0; i < m->nr_nodes; i++) {
            bool found1 = false;
            bool found2 = false;
            for (int j = 0; j < m->nodes[i].num_ways; j++) {
                if (strstr(m->ways[m->nodes[i].way_ids[j]].name, name1) != NULL) {
                    found1 = true;
                }
                if (strstr(m->ways[m->nodes[i].way_ids[j]].name, name2) != NULL) {
                    found2 = true;
                }
            }
            if (found1 && found2) {
                printf("%d ", m->nodes[i].id);
            }
        }
        printf("\n");
    }
}

/**
 * Converts from degree to radian
 *
 * @param deg The angle in degrees.
 * @return the equivalent value in radian
 */
#define d2r(deg) ((deg) * M_PI/180.)

/**
 * Calculates the distance between two nodes using the Haversine formula.
 *
 * @param x The first node.
 * @param y the second node.
 * @return the distance between two nodes, in kilometre.
 */
static double
distance_between_nodes(const struct node * x, const struct node * y) {
    double R = 6371.;       
    double lat1 = x->lat;
    double lon1 = x->lon;
    double lat2 = y->lat;
    double lon2 = y->lon;
    double dlat = d2r(lat2-lat1); 
    double dlon = d2r(lon2-lon1); 
    double a = pow(sin(dlat/2), 2) + cos(d2r(lat1)) * cos(d2r(lat2)) * pow(sin(dlon/2), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a)); 
    return R * c; 
}

/**
 * Function to check if nodes share a way and return if true
 * */ 

int
shared_way(const struct ssmap * m, int node1, int node2) {
    for (int i = 0; i < m->nodes[node1].num_ways; i++) {
        for (int j = 0; j < m->nodes[node2].num_ways; j++) {
            if (m->nodes[node1].way_ids[i] == m->nodes[node2].way_ids[j]) {
                // Found a common way id, return it.
                return m->nodes[node1].way_ids[i];
            }
        }
    }
    // No common way id found
    return -1;
}


/**
 * Function to calculate travel time between two nodes in minutes
 * */ 
double 
calculate_travel_time(struct node node1, struct node node2, double speed_limit) {
    struct node *a_ptr = &node1;
    struct node *b_ptr = &node2;
    double distance = distance_between_nodes(a_ptr, b_ptr) * 1000; // Distance in meters

    double speed = speed_limit / 3.6; // Convert speed to m/s
    double time_seconds = distance / speed; // Time in seconds
    return time_seconds / 60.0; // Convert time to minutes
}


/**
 * Calculate the travel time of a path (an ordered array of node ids)
 *
 * You may make the following simplifications
 * 1. There are no traffic. You are the only one driving on the road.
 * 2. You are always able to drive at the speed limit of the road you are on.
 * 3. There are no acceleration or deceleration. You instantly drive at the speed
 *    limit of the road, and you can turn instantaneously.
 * 4. There are no turn restrictions, i.e., you can turn in any direction at any 
 *    intersection. 
 *
 * You must check for the following errors:
 * 1. Each node id must be valid. If not, print "error: node <id> does not exist."
 * 2. There must be a way object between two adjacent nodes in the array. If not,
 *    print "error: there are no roads between node <a> and node <b>."
 * 3. If a way object contains multiple nodes [a, b, c, d, ...], you must not
 *    skip any nodes in between. If violated, print "error: cannot go directly from
 *     node <a> to node <b>."
 * 4. If a way object is one-way, the nodes in the path cannot go in reverse order.
 *    e.g., suppose a way object has nodes [a, b, c, d, ...] and it is one-way,
 *    then it would be a violation to go from node c to node b. If this happens,
 *    print "error: cannot go in reverse from node <a> to node <b>."
 *
 * If an error occurs, print the error message and return -1.0.
 * 
 * @param m The ssmap structure where the path lies.
 * @param size The size of the path array
 * @param node_ids The path array. Each element of the array is a node id, and
 * the array represents the order in which a traveler goes from one node to
 * another. E.g., suppose the path is [a, b, c, d], it means the travelers starts
 * from node a, goes to node b, c, and finaly node d in that order. For each 
 * adjacent set of nodes, there must be a valid way object that connects them.
 * @return the total travel time of a path in MINUTES, which can be calculated as 
 * the sum of the distance between each node divided by the speed limit of each 
 * segment. E.g., suppose the path is [a, b, c] and way x and y connects a and b,
 * b and c, respectively. Then the travel time would be distance(a, b) / maxspeed(x)
 * + distance(b, c) / maxspeed(y). Hint: use the distance function provided.
 */


double 
ssmap_path_travel_time(const struct ssmap * m, int size, int node_ids[size])
{
    double total_travel_time = 0.0;
    struct node *curr_ptr;
    struct node *next_ptr;

    // Error 1: Check for valid node IDs
    for (int i = 0; i < size; i++) {
        if (node_ids[i] < 0 || node_ids[i] >= m->nr_nodes) {
            printf("error: node %d does not exist.\n", node_ids[i]);
            return -1.0;
        }
    }


    for (int i = 0; i < size - 1; i++) {
        int current_node_id = node_ids[i];
        int next_node_id = node_ids[i + 1];
        curr_ptr = &m->nodes[current_node_id];
        next_ptr = &m->nodes[next_node_id];

        // Error 5: Check for duplicate nodes
        int index = i + 1;
        while (index < size) {
            if (node_ids[index] != current_node_id) {
                index++;
            } else {
                printf("error: node %d appeared more than once.\n", current_node_id);
                return -1.0;
            }
        }

        // Iterate through all ways to find a connecting road
        bool adjacent_in_way = false;

        int way_id = shared_way(m, current_node_id, next_node_id);
        if (way_id == -1) {
            printf("error: there are no roads between node %d and node %d.\n", current_node_id, next_node_id);
            return -1.0;
        }
        struct way way1 = m->ways[way_id];
        int j = 0;
        for (; j < way1.num_nodes - 1; j++) {
            if (((way1.node_ids[j] == next_node_id) && (way1.node_ids[j + 1] == current_node_id)) || ((way1.node_ids[j] == current_node_id) && (way1.node_ids[j + 1] == next_node_id))) {
                adjacent_in_way = true;
                break;
            }
        }
        if (!adjacent_in_way) {
            printf("error: cannot go directly from node %d to node %d.\n", current_node_id, next_node_id);
            return -1.0;
        }
        if (way1.one_way && !(way1.node_ids[j] == current_node_id && way1.node_ids[j + 1] == next_node_id)) {
            printf("error: cannot go in reverse from node %d to node %d.\n", current_node_id, next_node_id);
            return -1.0;
        }

        // Calculate travel time for this segment.
        total_travel_time += calculate_travel_time(*curr_ptr, *next_ptr, way1.speed_limit);
    
    }

    return total_travel_time;

}


/**
 * Function finds the neighbors or adjecent nodes of given node and
 * return all of them in a MinHeap
*/

struct MinHeap *
find_neighbors(const struct ssmap * m, int node_id) {
    struct node curr_node = m->nodes[node_id];
    struct MinHeap* heap = create_min_heap(curr_node.num_ways * 2);
    for (int i = 0; i < curr_node.num_ways; i++) {
        struct way way1 = m->ways[curr_node.way_ids[i]];
        if (way1.node_ids[0] == node_id) {
            insert_into_heap(heap, way1.node_ids[1], 0.0);
        } else if (way1.node_ids[way1.num_nodes - 1] == node_id) {
            if (!way1.one_way){
                insert_into_heap(heap, way1.node_ids[way1.num_nodes - 2], 0.0);
            }
        } else {
            for (int j = 0; j < way1.num_nodes; j++) {
                if (way1.node_ids[j] == node_id) {
                    if (way1.one_way) {
                        insert_into_heap(heap, way1.node_ids[j + 1], 0.0);
                    }  else {
                        insert_into_heap(heap, way1.node_ids[j - 1], 0.0);
                        insert_into_heap(heap, way1.node_ids[j + 1], 0.0);
                    }
                    
                }
            }
        }
    }
    return heap;
}


/**
 * Compute a path from one node to another.
 *
 * You may make all of the assumptions in the ssmap_path_travel_time, and you
 * must ensure the absence of all errors described under ssmap_path_travel_time.
 *
 * The format of the output should be space-separated node ids. e.g.
 * 9 32 105 5 782 96
 *
 * Note: you should try to optimize this function so that the travel time is 
 * minimized.
 * 
 * @param m The ssmap structure where the path will be created.
 * @param start_id the starting node id 
 * @param end_id the destination node id
 */


void 
ssmap_path_create(const struct ssmap * m, int start_id, int end_id)
{
    int V = m->nr_nodes;
    if (start_id < 0 || end_id >= V){
        printf("No path found from %d to %d.\n", start_id, end_id);
        return;
    }
    MinHeap* heap = create_min_heap(V);
    double* times = malloc(V * sizeof(double));
    int* predecessors = malloc(V * sizeof(int));
    bool* visited = malloc(V * sizeof(bool));

    if (!times || !predecessors || !visited || !heap) {
        fprintf(stderr, "Memory allocation failed.\n");
        // Cleanup and early exit if memory allocation fails.
        free(times); 
        free(predecessors);
        free(visited);
        destroy_min_heap(heap);
        return;
    }
    times[start_id] = 0.0;
    predecessors[start_id] = -1;
    visited[start_id] = false;

    for (int i = 0; i < V; i++) {
        if (i != start_id) {
            times[i] = INFINITY_COST;
            predecessors[i] = -1;
            visited[i] = false;
            insert_into_heap(heap, i, INFINITY_COST);
        }
    }


    insert_into_heap(heap, start_id, 0.0);

    while (heap->size > 0) {
        int current_node = remove_min(heap).node_id;

        if (current_node == end_id) {
            break; // Found the shortest path to the destination.
        }
        visited[current_node] = true;

        struct node current_map_node = m->nodes[current_node];
        MinHeap* neighbors = find_neighbors(m, current_node);
        while (neighbors->size > 0) {
            int next_node = remove_min(neighbors).node_id;
            if (visited[next_node]) continue; // Ensure forward movement.
            int common_way = shared_way(m, current_node, next_node);
            double time = calculate_travel_time(current_map_node, m->nodes[next_node], m->ways[common_way].speed_limit);
            double new_time = times[current_node] + time;
            if (new_time < times[next_node]) {
                times[next_node] = new_time;
                predecessors[next_node] = current_node;
                decrease_key(heap, next_node, new_time);
            }
        }
        destroy_min_heap(neighbors);
    }

    // Reconstruct and print the path from end_id to start_id.
    int u = end_id;
    int *path = (int *)malloc(V * sizeof(int));
    if (!path) {
        fprintf(stderr, "Memory allocation failed.\n");
        // Cleanup and early exit if memory allocation fails.
        free(times); 
        free(predecessors); 
        destroy_min_heap(heap);
        free(visited);
        return;
    }

    if ((u == start_id) || (predecessors[u] != -1)) {
        int cc = 0;
        while (u != -1) {
            path[cc++] = u;
            u = predecessors[u];
        }
        for (int i = cc - 1; i >= 0; i--) {
            printf("%d ", path[i]);
        }
        printf("\n");
    } else {
        printf("No path found from %d to %d.\n", start_id, end_id);
    }
    free(visited);
    free(times);
    free(predecessors);
    destroy_min_heap(heap);
}