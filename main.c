#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define READERS_COUNT 15
#define WRITERS_COUNT 10
#define TESTS 10
struct guarded {
	size_t memsize;
	void *memory;
	pthread_mutex_t counter_access;
	pthread_cond_t read_ready, write_ready;
	int readers_counter;
};

struct rw_op_args {
	struct guarded memory;
	void* (*custom_function)(void*);
};

void *to_read(void *args) {
	struct rw_op_args* a = (struct rw_op_args*)args;
	pthread_mutex_lock(&(a->memory.counter_access));
	while (a->memory.readers_counter<0) {
		pthread_cond_wait(&(a->memory.read_ready), &(a->memory.counter_access));
	}
	a->memory.readers_counter++;
#ifdef DEBUG
	printf("(read_cnt=%d)", a->memory.readers_counter);
#endif
	pthread_mutex_unlock(&(a->memory.counter_access));
	a->custom_function(a->memory.memory);/* READING function */
	pthread_mutex_lock(&(a->memory.counter_access));
	a->memory.readers_counter--;
	if (a->memory.readers_counter==0) {
		pthread_cond_signal(&(a->memory.write_ready));
		pthread_cond_broadcast(&(a->memory.read_ready));
	}
	pthread_mutex_unlock(&(a->memory.counter_access));
	return NULL;
}

void *to_write(void* args) {
	struct rw_op_args* a = (struct rw_op_args*)args;
	pthread_mutex_lock(&(a->memory.counter_access));
	while (a->memory.readers_counter!=0) {
		pthread_cond_wait(&(a->memory.write_ready), &(a->memory.counter_access));
	}
	a->memory.readers_counter--;
	a->custom_function(a->memory.memory);/* WRITING function */
	a->memory.readers_counter++;
	pthread_mutex_unlock(&(a->memory.counter_access));
	pthread_cond_signal(&(a->memory.write_ready));
	pthread_cond_broadcast(&(a->memory.read_ready));
	return NULL;
}

void *read_elementary (void *here) {
	char *c = (char*)here;
	printf("Read %c\t", *c);
	return NULL;
}

void *write_elementary (void *here){
	char *c = (char*)here;
	if (*c == 'a') *c = 'v'; else *c = 'a';
	printf("Wrote %c\t",*c);
	return NULL;
}

void guard_init (struct guarded* memory) {
	pthread_mutex_init(&(memory->counter_access), NULL);
	pthread_cond_init(&(memory->read_ready),      NULL);
	pthread_cond_init(&(memory->write_ready),     NULL);
	memory->readers_counter = 0;
}

void guard_destroy (struct guarded* memory) {
	pthread_cond_destroy(&(memory->write_ready));
	pthread_cond_destroy(&(memory->read_ready));
	pthread_mutex_destroy(&(memory->counter_access));
}

void args_assign (struct rw_op_args *rw_op, struct guarded memory, void* (*custom_function)(void*)) {
	rw_op->memory = memory;
	rw_op->custom_function = custom_function;
}

int main(void) {
	size_t test, i;
	char c;
	pthread_t r[READERS_COUNT], w[WRITERS_COUNT];
	struct rw_op_args rargs, wargs;
	struct guarded memory;
	for (test = 0; test<TESTS; ++test) {
		printf("TEST%zu:\t", test+1);
		guard_init(&memory);
		memory.memory = &c;/*for proof of concept only 1 chr, not memory page */
		args_assign(&rargs, memory, read_elementary);
		args_assign(&wargs, memory, write_elementary);
		
		for (i = 0; (i<READERS_COUNT)||(i<WRITERS_COUNT); ++i) {
			if (i<READERS_COUNT) pthread_create(r+i, NULL, to_read, &rargs);
			if (i<WRITERS_COUNT) pthread_create(w+i, NULL, to_write,&wargs);
		}

		for (i = 0; (i<READERS_COUNT)||(i<WRITERS_COUNT); ++i) {
			if (i<READERS_COUNT) pthread_join(r[i], NULL);
			if (i<WRITERS_COUNT) pthread_join(w[i], NULL);
		}
		printf("\n\n");
		guard_destroy(&memory);
	}
	return 0;
}
