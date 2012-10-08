#include "hashtable.h"

unsigned int hash_string(char *str, int m) {
	unsigned int i;
	for( i=0; *str; str++ )
		i = 131*i + *str;
	return( i % m );
}

unsigned int hash_int(int key, int m) {
	return( (unsigned int)key % m );
}


htbl_entry *alloc_entry(htbl_t phtbl) {
	htbl_entry* entry;
	if((phtbl->initbuf) && (phtbl->entry_front)) {
		entry = phtbl->entry_front;
		phtbl->entry_front = phtbl->entry_front->next;
		phtbl->nbuf--;
		if(phtbl->entry_front == NULL) {
			phtbl->entry_back =  NULL;
		}
	}
	else
		entry = (htbl_entry*)malloc(sizeof(htbl_entry));
	return entry;
}

void release_entry(htbl_t phtbl, htbl_entry* entry) {
	entry->next = NULL;
	if((phtbl->initbuf) && (phtbl->nbuf < 3*phtbl->initbuf)) {
		if(phtbl->entry_back) {
			phtbl->entry_back->next = entry;
		}
		else {
			phtbl->entry_front = entry;
			phtbl->entry_back  = entry;
		}
		phtbl->nbuf++;
	}
	else
		free(entry);
}

int   htbl_init(htbl_t *pphtbl, int init, int initbuf) {
	*pphtbl = (htbl_t)malloc(sizeof(hashtbl));
	(*pphtbl)->tbl_size = init;
	(*pphtbl)->n_entries = 0;
	(*pphtbl)->entries = (htbl_entry**) malloc(init * sizeof(htbl_entry*));
	memset((*pphtbl)->entries, 0, init * sizeof(htbl_entry*));
	(*pphtbl)->entries_chain_lengths = (int*)malloc(init * sizeof(int));
	memset((*pphtbl)->entries_chain_lengths, 0, init * sizeof(int));

	(*pphtbl)->nbuf = 0;
	(*pphtbl)->initbuf = initbuf;
	if((*pphtbl)->initbuf) {
		htbl_entry* entry;
		(*pphtbl)->entry_front= (htbl_entry*)malloc(sizeof(htbl_entry));
		(*pphtbl)->entry_front->next = NULL;
		(*pphtbl)->entry_back = (*pphtbl)->entry_front;
		for((*pphtbl)->nbuf=1; (*pphtbl)->nbuf < (*pphtbl)->initbuf; (*pphtbl)->nbuf++) {
			(*pphtbl)->entry_back->next = (htbl_entry*)malloc(sizeof(htbl_entry));
			(*pphtbl)->entry_back = (*pphtbl)->entry_back->next;
			(*pphtbl)->entry_back->next = NULL;
		}
	}
	else {
		(*pphtbl)->entry_front = NULL;
		(*pphtbl)->entry_back = NULL;
	}
	return 0;
}

int htbl_resize_buf(htbl_t phtbl, int buf) {
	phtbl->initbuf = buf;
	return 0;
}

int   htbl_free(htbl_t phtbl) {
	int itr;
	htbl_entry *prev;
	htbl_entry *entry = NULL;
	for(itr=0; itr < phtbl->tbl_size; itr++) {
		entry = phtbl->entries[itr];
		while(entry) {
			prev = entry;
			entry = entry->next;
			free(prev);
		}
	}
	free(phtbl->entries_chain_lengths);
	free(phtbl->entries);
	free(phtbl);
	return 0;
}

int   htbl_count(htbl_t phtbl) {
	return phtbl->n_entries;
}

int   htbl_size(htbl_t phtbl) {
	return phtbl->tbl_size;
}

int   htbl_empty_slots(htbl_t phtbl) {
	int itr;
	int n=0;
	for(itr=0; itr < phtbl->tbl_size; itr++) {
		if(phtbl->entries[itr]==NULL) {
			n++;
		}
	}
	return n;
}

int   htbl_longest_chain(htbl_t phtbl, int *index) {
	int ix;
	int itr;
	int longest=0;
	for(itr=0; itr < phtbl->tbl_size; itr++) {
		if(phtbl->entries_chain_lengths[itr] > longest) {
			longest = phtbl->entries_chain_lengths[itr];
			ix = itr;
		}
	}
	if(index)
		*index = ix;
	return longest;
}

int   htbl_rehash(htbl_t phtbl, int newsize) {
	int oldsize;
	int itr;
	htbl_entry *entry;
	htbl_entry *prev;
	htbl_entry **old_entries = phtbl->entries;

	if(newsize == phtbl->tbl_size)
		return 0;

	oldsize = phtbl->tbl_size;
	phtbl->tbl_size = newsize;
	itr = newsize * sizeof(htbl_entry*);
	phtbl->entries = (htbl_entry**) malloc(itr);
	memset((phtbl)->entries, 0, itr);

	free(phtbl->entries_chain_lengths);
	itr = newsize * sizeof(int);
	phtbl->entries_chain_lengths = (int*)malloc(itr);
	memset(phtbl->entries_chain_lengths, 0, itr);

	phtbl->n_entries=0;
	entry = prev = NULL;

	for(itr =0; itr < oldsize; itr++) {
		entry = old_entries[itr];
		while(entry) {
			if(entry->type == HTBL_KEY_INT) {
				htbl_put(phtbl, (int)entry->key, entry->data);
			}
			else {
				htbl_str_put(phtbl, (char*)entry->key, entry->data);
				free(entry->key);
			}

			prev = entry;
			entry = entry->next;
			release_entry(phtbl, prev);
		}
	}
	free(old_entries);
	return 0;
}

int   htbl_enum_int_keys(htbl_t phtbl, int *keys[], int nkeys) {
	int itr;
	int *ikeys;
	int count=0;
	htbl_entry *entry;
	ikeys = (int*)keys;
	for(itr=0; itr < phtbl->tbl_size; itr++) {
		entry = phtbl->entries[itr];
		while(entry) {
			if(entry->type == HTBL_KEY_INT) {
				ikeys[count++] = (int)entry->key;
			}
			else
				ikeys[count++] = 0;

			entry = entry->next;
			if(count >= nkeys)
				break;
		}
	}
	return count;
}


int   htbl_enum_str_keys(htbl_t phtbl, void **keys, int nkeys) {
	int itr;
	int *ikeys;
	int count=0;
	htbl_entry *entry;
	ikeys = (int*)*keys;
	for(itr=0; itr < phtbl->tbl_size; itr++) {
		entry = phtbl->entries[itr];
		while(entry) {
			if(entry->type == HTBL_KEY_INT) {
				keys[count++] = (void*)entry->key;
			}
			else
				keys[count++] = entry->key;

			entry = entry->next;
			if(count >= nkeys)
				break;
		}
	}
	return count;
}


htbl_entry *locate_entry(htbl_t phtbl, int key) {
	htbl_entry *entry = phtbl->entries[hash_int(key, phtbl->tbl_size)];
	while(entry) {
		if((entry->type == HTBL_KEY_INT)
			&& (key==(int)entry->key))
				break;
		entry = entry->next;
	}
	return entry;
}

htbl_entry *locate_str_entry(htbl_t phtbl, char *key) {
	htbl_entry *entry = phtbl->entries[hash_string(key, phtbl->tbl_size)];
	while(entry) {
		if((entry->type == HTBL_KEY_STR)
			&& (strcmp(key,(char*)entry->key)==0))
			break;
		entry = entry->next;
	}
	return entry;
}


int   htbl_put(htbl_t phtbl, int key, void *data) {
	int h;
	htbl_entry *entry = locate_entry(phtbl, key);

	if(entry)
		return -1;
	h = hash_int(key, phtbl->tbl_size);

	entry = alloc_entry(phtbl);
	entry->type = HTBL_KEY_INT;
	entry->key  = (void*)key;
	entry->hash = h;
	entry->data = data;
	entry->next = phtbl->entries[h];
	phtbl->entries[h] = entry;
	phtbl->n_entries++;
	phtbl->entries_chain_lengths[h]++;
	return 0;
}

int   htbl_str_put(htbl_t phtbl, char *key, void *data) {
	int h;
	htbl_entry *entry = locate_str_entry(phtbl, key);

	if(entry)
		return -1;

	entry = alloc_entry(phtbl);
	entry->type = HTBL_KEY_STR;
	h = strlen(key)+1;
	entry->key  = malloc(h);
	memcpy(entry->key, key, h);
	h = hash_string(key, phtbl->tbl_size);
	entry->data = data;
	entry->hash = h;
	entry->next = phtbl->entries[h];
	phtbl->entries[h] = entry;
	phtbl->n_entries++;
	phtbl->entries_chain_lengths[h]++;
	return 0;
}

int htbl_get(htbl_t phtbl, int key, void **data) {
	htbl_entry *entry = locate_entry(phtbl, key);
	if(entry) {
		*data = entry->data;
		return 1;
	}
	else {
		return 0;
	}
}

int htbl_str_get(htbl_t phtbl, char *key, void **data) {
	htbl_entry *entry = locate_str_entry(phtbl, key);
	if(entry) {
		*data= entry->data;
		return 1;
	}
	else {
		return 0;
	}
}

int htbl_remove(htbl_t phtbl, int key, void **data) {
	int h;
	htbl_entry *entry;
	htbl_entry *prev;
	int ret = 0;
	entry = prev = NULL;

	h = hash_int(key, phtbl->tbl_size);

	entry = phtbl->entries[h];

	while(entry) {
		if((entry->type == HTBL_KEY_INT)
			&& (key== (int)entry->key)) {
			if(prev)
				prev->next = entry->next;
			else
				phtbl->entries[h] = entry->next;

			*data = entry->data;
			ret = 1;
			release_entry(phtbl, entry);
			phtbl->n_entries--;
			phtbl->entries_chain_lengths[h]--;
			break;
		}
		prev = entry;
		entry = entry->next;
	}
	return ret;
}

int htbl_str_remove(htbl_t phtbl, char *key, void **data) {
	int h;
	htbl_entry *entry;
	htbl_entry *prev;
	int ret = 0;
	entry = prev = NULL;

	h = hash_string(key, phtbl->tbl_size);

	entry = phtbl->entries[h];

	while(entry) {
		if((entry->type == HTBL_KEY_STR)
			&& (strcmp(key,(char*)entry->key)==0)) {
			if(prev)
				prev->next = entry->next;
			else
				phtbl->entries[h] = entry->next;

			*data = entry->data;
			ret = 1;
			free(entry->key);
			release_entry(phtbl, entry);
			phtbl->n_entries--;
			phtbl->entries_chain_lengths[h]--;
			break;
		}
		prev = entry;
		entry = entry->next;
	}
	return ret;
}

int htbl_replace(htbl_t phtbl, int key, void* data, void **replaced) {
	int h;
	htbl_entry *entry;
	htbl_entry *prev;
	int ret = 0;
	entry = prev = NULL;

	if(htbl_put(phtbl,key,data) < 0) {
		entry = locate_entry(phtbl, key);
		if(replaced)
			*replaced = entry->data;
		ret = 1;
		entry->data = data;
		entry->type = HTBL_KEY_INT;
	}
	return ret;
}

int htbl_str_replace(htbl_t phtbl, char *key, void *data, void **replaced) {
	int h;
	htbl_entry *entry;
	htbl_entry *prev;
	int ret = 0;
	entry = prev = NULL;

	if(htbl_str_put(phtbl,key,data) < 0) {
		entry = locate_str_entry(phtbl, key);
		if(replaced)
			*replaced = entry->data;
		ret = 1;
		entry->data = data;
		entry->type = HTBL_KEY_STR;
	}
	return ret;
}

int htbl_remove_next(htbl_t phtbl, void **data) {
	int itr;

	for(itr=0; itr < phtbl->n_entries; itr++) {
		if(phtbl->entries[itr]) {
			if(phtbl->entries[itr]->type == HTBL_KEY_STR)
				free(phtbl->entries[itr]->key);

			*data = phtbl->entries[itr]->data;
			release_entry(phtbl, phtbl->entries[itr]);
			phtbl->entries[itr] = phtbl->entries[itr]->next;
			phtbl->n_entries--;
			phtbl->entries_chain_lengths[itr]--;
			return 0;
		}
	}
	return 0;
}
