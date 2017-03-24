/*
 *	Vector data structure with automatic allocation.
 */

#include	<string.h>
#include	<stdlib.h>
#include	<assert.h>
#include	"vector.h"

#define	DEFAULT_VEC_INCR	10	/* default increment of vector size */


// ensure we have the capacity to store element num. So the capacity must be > num
static void
ensure_capacity(vector_t p, unsigned size)
{
	void *		d;
	unsigned	newsize;

	if(size < p->count)
		size = p->count;
	newsize = p->capacity;
	if(newsize == 0)
		newsize = p->increment;
	while(newsize <= size)
		newsize *= 2;
	if(p->capacity < newsize) {
		d = calloc(newsize,sizeof *p->data);
		if(p->capacity) {
			memcpy(d, p->data, p->count*sizeof *p->data);
			free(p->data);
		}
		p->data = d;
		p->capacity = newsize;
	}
}

vector_t
vec_new()
{
	return vec_new_incr(DEFAULT_VEC_INCR);
}

vector_t
vec_new_incr(unsigned incr)
{
	static unsigned unique;
	vector_t	p;

	p = calloc(1, sizeof *p);
	p->increment = incr;
	p->unique = unique++;
	return p;
}

void
vec_destroy(vector_t p)
{
	if(p == NULL)
		return;
	if(p->data != NULL)
		free(p->data);
	free(p);
}

vector_t
vec_copy(const vector_t p)
{
	vector_t t;

	if(p == NULL)
		return NULL;
	t = calloc(1, sizeof *t);
	memcpy(t, p, sizeof *t);
	t->data = calloc(t->capacity, sizeof *t->data);
	memcpy(t->data, p->data, t->count * sizeof *t->data);
	return t;
}

unsigned
vec_add(vector_t p, const void * e)
{
	assert(p);
	ensure_capacity(p, 0);
	p->data[p->count] = e;
	return p->count++;
}

void *
vec_top(vector_t p)
{

	assert(p);
	if(p->count == 0)
		return NULL;
	return (void *) p->data[p->count-1];
}

void *
vec_pop(vector_t p)
{

	assert(p);
	if(p->count == 0)
		return NULL;
	return (void *) p->data[--p->count];
}

unsigned
vec_addAll(vector_t p, vector_t n)
{
	void *	vp;

	assert(p);
	assert(n);
	VEC_ITERATE(n, vp, void *)
		vec_add(p, vp);
	return vec_size(n);
}


unsigned
vec_addNotPresent(vector_t p, const void * e)
{
	unsigned	i;

	if((i = vec_indexOf(p, e)) == -1)
		return vec_add(p, e);
	return i;
}

/*
 * Add all elements of another set to the current set if
 * not already present. The current set becomes the union
 * of itself and the other set.
 */

unsigned
vec_addAllNotPresent(vector_t p, vector_t n)
{
	void *	vp;

	assert(p);
	assert(n);
	VEC_ITERATE(n, vp, void *)
		vec_addNotPresent(p, vp);
	return vec_size(n);
}


unsigned
vec_insert(vector_t p, const void * e, unsigned pos)
{
	unsigned	i;

	ensure_capacity(p, 0);
	if(pos > p->count)
		pos = p->count;
	if(pos <= p->marker)
		p->marker++;
	i = p->count;
	while(i-- > pos)
		p->data[i+1] = p->data[i];
	p->data[pos] = e;
	p->count++;
	return pos;
}

/* move an element. The element at "from" becomes element "to".
 */

void
vec_move(vector_t p, unsigned from, unsigned to)
{
	const void *	e;

	if(from >= p->count)
		return;
	ensure_capacity(p, to);
	e = p->data[from];
	if(from < to) {
		do
			p->data[from] = p->data[from+1];
		while(++from != to);
	} else {
		do
			p->data[from] = p->data[from-1];
		while(--from != to);
	}
	p->data[to] = e;
}


void
vec_setAt(vector_t p, const void * e, unsigned pos)
{
	ensure_capacity(p, pos);
	if (pos >= p->count) {
		p->count = pos+1;
	}
	p->data[pos] = e;
}

unsigned	
vec_remove(vector_t p, const void * e)
{
	int	i;
	
	i = vec_indexOf(p, e);
	if(i == -1)
		return 0;
	return vec_removeAt(p, i);
}

unsigned
vec_removeAt(vector_t p, unsigned pos)
{
	if(pos >= p->count)
		return 0;
	p->count--;
	if(pos < p->marker)
		p->marker--;
	while(pos != p->count) {
		p->data[pos] = p->data[pos+1];
		pos++;
	}
	p->data[pos] = 0;
	return 1;
}

void
vec_removeAll(vector_t p)
{
	p->count = 0;
}

void *
vec_elementAt(const vector_t p, unsigned pos)
{
	if(pos < p->count)
		return (void *) p->data[pos];
	return NULL;
}

int
vec_indexOf(const vector_t p, const void * e)
{
	unsigned int	i;
	
	for(i = 0 ; i != p->count ; i++)
		if(p->data[i] == e)
			return i;
	return -1;
}

unsigned
vec_size(const vector_t p)
{
	if(p == NULL)
		return 0;
	return p->count;
}

void *
vec_first(vector_t p)
{
	if (!p)
		return NULL;
	p->marker = 0;
	return vec_next(p);
}

void *
vec_next(vector_t p)
{
	if(p->marker < p->count)
		return vec_elementAt(p, p->marker++);
	return NULL;
}

void *
vec_first_nest(vector_t p)
{
	if (!p)
		return NULL;
	p->marker_nest = 0;
	return vec_next_nest(p);
}

void *
vec_next_nest(vector_t p)
{
	if(p->marker_nest < p->count)
		return vec_elementAt(p, p->marker_nest++);
	return NULL;
}

void *
vec_look(vector_t p)
{
	if(p->marker < p->count)
		return vec_elementAt(p, p->marker);
	return NULL;
}

void *
vec_look_nest(vector_t p)
{
	if(p->marker_nest < p->count)
		return vec_elementAt(p, p->marker_nest);
	return NULL;
}

void
vec_reset(vector_t p)
{
	p->marker = 0;
}

unsigned
vec_hasNext(vector_t p)
{
	return p->marker < p->count;
}

void
vec_sort(vector_t p, int (*cmpfunc)(const void *, const void *))
{
	if(p->count == 0)
		return;
	qsort(p->data, p->count, sizeof *p->data, cmpfunc);
}
