/*
 *	Vector data structure with automatic allocation.
 */

#ifndef _VECTOR_H_
#define _VECTOR_H_

/*
 * This structure should be considered private.
 */
typedef struct vector_t
{
	unsigned	count;		/* number of elements */
	unsigned	capacity;	/* available slots */
	unsigned	increment;	/* increment of vector size */
	unsigned	marker;		/* next one to return from vec_next() */
	unsigned	marker_nest;	/* next one to return from vec_next_nest() */
	const void **	data;		/* the list of data */
	unsigned	unique;		/* a unique number */
} *	vector_t;

/*
 * Constructor. Uses default increment.
 */
extern vector_t	vec_new();

/*
 * Constructor. Uses given increment.
 */
extern vector_t	vec_new_incr(unsigned);

/*
 * Destructor.
 */
extern void	vec_destroy(vector_t);

/*
 * Create a deep copy.
 */
extern vector_t	vec_copy(const vector_t);

/*
 * Add an element to the end.
 */
extern unsigned	vec_add(vector_t, const void *);

/*
 * 	Add all elements in one vector to another
 * 	Return the number added.
 */

extern unsigned vec_addAll(vector_t, vector_t);

#define vec_present(p, e)	(vec_indexOf(p, e) != -1)

/*
 * Add an element only if it is not already in the vector
 */

extern unsigned vec_addNotPresent(vector_t p, const void * e);

/*
 * Add all elements of another vector to the current vector if
 * not already present. The current vector becomes the union
 * of itself and the other vector.
 */

extern unsigned vec_addAllNotPresent(vector_t p, vector_t);

/*
 * Insert an element at a particular place.
 */
extern unsigned	vec_insert(vector_t, const void *, unsigned);

/*
 * Set an element at a particular place.
 */
extern void		vec_setAt(vector_t, const void *, unsigned);

/*
 * Move an element from one slot to another
 */

extern void vec_move(vector_t, unsigned from, unsigned to);
/*
 * Remove the specified element.
 */
extern unsigned	vec_remove(vector_t, const void *);

/*
 * Remove the element at the given index.
 */
extern unsigned	vec_removeAt(vector_t, unsigned);

/*
 * Remove all elements. Reset the size and capacity to zero.
 */
extern void		vec_removeAll(vector_t);

/*
 * Retrieve element at the given index.
 * Returns NULL if not found.
 */
extern void *		vec_elementAt(const vector_t, unsigned);

/*
 * Retrieve the index of the given element.
 * Returns -1 if not found.
 */
extern int		vec_indexOf(const vector_t, const void *);

/*
 * Return the number of elements.
 */
extern unsigned	vec_size(const vector_t);

/*
 * Get the first element, and initialize the marker. And nested version.
 */
extern void *		vec_first(vector_t);
extern void *		vec_first_nest(vector_t);

/*
 * Get the next element, and update the marker. And nested version.
 */
extern void *		vec_next(vector_t);
extern void *		vec_next_nest(vector_t);

/*
 * Get the next element, without updating the marker. And nested version.
 */
extern void *		vec_look(vector_t);
extern void *		vec_look_nest(vector_t);

/*
 * Reset the marker, but don't increment it.
 * Use this with vec_hasNext() to allow iteration through vectors
 * that might include NULL pointers.
 */

extern void			vec_reset(vector_t);

/*
 * 	Are there more elements to get with vec_next() ?
 */

extern unsigned		vec_hasNext(vector_t);

/*
 * Sort the vector, as in qsort().
 */
extern void		vec_sort(vector_t, int (*)(const void *, const void *));

/* stack emulation */

#define	vec_push(p, a)	vec_add((p), (a))

/* pop the top stack element and return it. */

extern void *	vec_pop(vector_t);

// return the top stack element but don't pop it

extern void *	vec_top(vector_t);

/*
 * Convenience macro for iterating through a vector with a given
 * index variable and type.
 */
#define	VEC_ITERATE(vec, el, eltype)	\
	for(vec_reset(vec) ; vec_hasNext(vec) && ((el = (eltype)vec_next(vec)), 1) ; )

/*
 * Convenience macro for iterating through a vector with a given
 * index variable and type. This one can be used recursively.
 * Do NOT use this if the vector will be modified during the loop.
 */
#define	VEC_ITERATE_R(vec, el, eltype, marker)	\
	for(marker=0; \
	    marker!=vec_size(vec) && ((el = (eltype)vec_elementAt(vec, marker)), 1); \
	    marker++)

#define	vec_contains(v, p)	(vec_indexOf(v, p) != -1)

#endif
