/*
 * mqueue.h -- interface of queue library
 */

#ifndef MQUEUE_H
#define MQUEUE_H

#include <stdio.h>
#include <stdlib.h>

/*********** Queue **************/

/* Queue item struct */
typedef struct mqueue_item {
  void *content;
  /*struct mqueue_item_t prev;*/
  struct mqueue_item *next;
} mqueue_item, *mqueue_item_t;


/* Queue Struct */
typedef struct mqueue {
  int count;
  int init;
  int nbuf;

  int index;
  mqueue_item_t indexed;

  mqueue_item_t buf_front;
  mqueue_item_t buf_back;

  mqueue_item_t first;
  mqueue_item_t last;
} mqueue, *mqueue_t;

/* Method to initialize queue */
void  mqueue_init(mqueue_t *queue, int nitems);

/* Method to destroy queue */
void  mqueue_free(mqueue_t queue);

/* Set queue buffer parameter*/
void  mqueue_resize_buffer(mqueue_t queue, int num);

/* Downsize queue buffer*/
void  mqueue_downsize_buffer(mqueue_t queue, int num);

/* Method to add to front of queue */
void  mqueue_add(mqueue_t queue, void *item);

/* Method to add to end of queue */
void  mqueue_append(mqueue_t queue, void *item);

/* Method to remove item from end of queue */
void* mqueue_remove(mqueue_t queue);

/* Method to get queue size */
int   mqueue_size(mqueue_t queue);

/* Method to access item from queue by index without removing*/
void* mqueue_peek_at_index(mqueue_t queue,int index);

/* Method to remove item from queue by index*/
void* mqueue_remove_at_index(mqueue_t queue,  int index);

/* Method to remove given item from queue by pointer comparison*/
void* mqueue_remove_item(mqueue_t queue, void* content);

#endif

