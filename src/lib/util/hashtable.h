#ifndef __HASHTABLE_H
#define __HASHTABLE_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define  HTBL_KEY_INT 1
#define  HTBL_KEY_STR 2

typedef struct htbl_entry {
	int 	type;
	long 	hash;
	void 	*key;
	void 	*data;
	struct htbl_entry *next;
} htbl_entry;

typedef struct hashtbl {
	int 	tbl_size;
	int 	n_entries;
	htbl_entry **entries;
	int		*entries_chain_lengths;

	int 		nbuf;
	int			initbuf;
	htbl_entry	*entry_front;
	htbl_entry	*entry_back;
} hashtbl, *htbl_t;

int htbl_init(htbl_t *pphtbl, int init, int buf);
int htbl_free(htbl_t phtbl);

int htbl_count(htbl_t phtbl);

int htbl_resize_buf(htbl_t phtbl, int buf);
int htbl_size(htbl_t phtbl);
int htbl_longest_chain(htbl_t phtbl, int *index);
int htbl_empty_slots(htbl_t phtbl);
int htbl_rehash(htbl_t phtbl, int newsize);

int htbl_enum_str_keys(htbl_t phtbl, void **keys, int nkeys);
int htbl_enum_int_keys(htbl_t phtbl, int *keys[], int nkeys);

int htbl_put(htbl_t phtbl, int key, void *data);
int htbl_str_put(htbl_t phtbl, char *key, void *data);

int htbl_get(htbl_t phtbl, int key, void **data);
int htbl_str_get(htbl_t phtbl, char *key, void **data);

int htbl_remove(htbl_t phtbl, int key, void **data);
int htbl_str_remove(htbl_t phtbl, char *key, void **data);

int htbl_replace(htbl_t phtbl, int key, void *data, void **replaced);
int htbl_str_replace(htbl_t phtbl, char *key, void *data, void **replaced);

#endif
