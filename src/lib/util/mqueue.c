#include "mqueue.h"
#define debug //printf
/* Method to initialize queue */
void mqueue_init(mqueue_t *pqueue, int nitems) {
	mqueue_t queue;
	*pqueue = (mqueue_t)malloc(sizeof(mqueue));
	queue = *pqueue;
	queue->count=0;
	queue->init = nitems;
	queue->nbuf = 0;
	queue->first=NULL;
	queue->last=NULL;
	queue->index = 0;
	queue->indexed = NULL;

	if(queue->init) {
		//mqueue_item_t item;
		queue->buf_front= (mqueue_item_t)malloc(sizeof(mqueue_item));
		queue->buf_front->next = NULL;
		queue->buf_back = queue->buf_front;
		for(queue->nbuf=1; queue->nbuf < queue->init; queue->nbuf++) {
			queue->buf_back->next = (mqueue_item_t)malloc(sizeof(mqueue_item));
			queue->buf_back = queue->buf_back->next;
			queue->buf_back->next = NULL;
		}
	}
	else {
		queue->buf_front= NULL;
		queue->buf_back = NULL;
	}

}


/* Method to destroy queue */
void  mqueue_free(mqueue_t queue) {
	if(queue->init) {
		mqueue_item_t item;;
		while(queue->buf_front) {
			item = queue->buf_front;
			queue->buf_front = queue->buf_front->next;
			free(item);
		}
	}
	free(queue);
}

mqueue_item_t mqueue_alloc_item(mqueue_t queue) {
	mqueue_item_t item;
	if((queue->init) && (queue->buf_front)) {
		item = queue->buf_front;
		queue->buf_front=queue->buf_front->next;
		queue->nbuf--;
		if(queue->buf_front == NULL) {
			queue->buf_back =  NULL;
		}
	}
	else
		item = (mqueue_item_t)malloc(sizeof(mqueue_item));
	return item;
}

void mqueue_free_item(mqueue_t queue, mqueue_item_t item) {
	item->next = NULL;
	if((queue->init)&&(queue->nbuf < 3*queue->init)) {
		if(queue->buf_back) {
			queue->buf_back->next = item;
		}
		else {
			queue->buf_front = item;
			queue->buf_back  = item;
		}
		queue->nbuf++;
	}
	else
		free(item);
}


/* Set queue buffer parameter*/
void  mqueue_resize_buffer(mqueue_t queue, int num) {
	queue->init = num;
}

/* Downsize queue buffer*/
void mqueue_downsize_buffer(mqueue_t queue, int percent) {
	mqueue_item_t item;
	int clear = queue->nbuf * percent / 100;
	while(clear) {
		item = mqueue_alloc_item(queue);
		free(item);
		clear--;
	}
}

/* Method to add to front of queue */
void mqueue_add(mqueue_t queue, void *_content) {
	mqueue_item_t item = mqueue_alloc_item(queue);
	item->content = _content;
	item->next = NULL;

	if(queue->last ==NULL) { 	/* first entry in queue */
		queue->last = item;
		queue->first = item;
		queue->index = 0;
		queue->indexed = queue->first;
	}
	else {
		item->next = queue->first;
		queue->first = item;
		queue->index++;
	}
	queue->count++;
	debug("\n adding %p,size=%d",_content, queue->count);
}

/* Method to add to end of queue */
void mqueue_append(mqueue_t queue, void *_content) {
	mqueue_item_t item = mqueue_alloc_item(queue);
	item->content = _content;
	item->next = NULL;

	if(queue->last ==NULL) { 	/* first entry in queue */
		queue->last = item;
		queue->first = item;
		queue->index = 0;
		queue->indexed = queue->first;
	}
	else {
		queue->last->next = item;
		queue->last = item;
	}
	queue->count++;
	debug("\n adding %p,size=%d",_content, queue->count);

}

/* Method to remove item from front of queue */
void* mqueue_remove(mqueue_t queue) {
	mqueue_item_t item;
	void* retval = NULL;
	item = queue->first;
	if(item!=NULL) {					/* If queue is not empty */
		retval = item->content;
		queue->first = item->next;
		if(queue->first == NULL) {
			queue->last=NULL;
		}
		queue->index = 0;
		queue->indexed = queue->first;
		mqueue_free_item(queue, item);
		queue->count--;
	}
	debug("\n removing %p,size=%d",retval, queue->count);
	return retval;
}

/* Method to get queue size */
int mqueue_size(mqueue_t queue) {
	return queue->count;
}


/* Method to access item from queue by index without removing*/
void* mqueue_peek_at_index(mqueue_t queue, int index) {
	//mqueue_item_t item;
	void* retval = NULL;

	if(index < queue->count) {
		if(queue->index > index) {
			queue->indexed 	= queue->first;
			queue->index 	= 0;
		}
		while(queue->index < index) {
			queue->indexed  = queue->indexed->next;
			queue->index++;
		}
		retval = queue->indexed->content;
	}
	return retval;
}

/* Method to remove item from queue by index*/
void* mqueue_remove_at_index(mqueue_t queue,int index) {
	void* retval;
	mqueue_item_t prev_item;
	int ix;

	retval = NULL;

	if(index < queue->count) {
		queue->indexed = queue->first;
		prev_item=NULL;
		if(0) {
			if(index > queue->index) {
				queue->indexed 	= queue->first;
				queue->index 	= 0;
			}
			while(queue->index < index) {
				if(queue->index == (index-1)) {
					prev_item = queue->indexed;
				}
				queue->indexed  = queue->indexed->next;
				queue->index++;
			}

			if(prev_item!=NULL) {
				prev_item->next = queue->indexed->next;
			}
			else {
				queue->first = queue->indexed->next;
			}
			prev_item = queue->indexed;
			if(queue->indexed->next) {
				queue->indexed = queue->indexed->next;
			}
			retval = prev_item->content;
			mqueue_free_item(queue, prev_item);
		}
		else {
			for(ix =0; ix < index; ix++) {
				if(ix==index-1) {
					prev_item = queue->indexed;
				}
				queue->indexed = queue->indexed->next;
			}
			//play safe and reset indexed
			queue->indexed 	= queue->first;
			queue->index 	= 0;

			if(prev_item!=NULL) {
				prev_item->next = queue->indexed->next;
			}
			else {
				queue->first = queue->indexed->next;
			}
			retval = queue->indexed->content;
			mqueue_free_item(queue, queue->indexed);
		}
		queue->count--;
	}
	return retval;
}

/* Method to remove given item from queue by pointer comparison*/
void* mqueue_remove_item(mqueue_t queue, void* content) {
	void* retval;
	mqueue_item_t item;
	int index;

	index=0;

	item = queue->first;
	while(item!=queue->last) {
		if(item->content!=content) {
			index++;
			item = item->next;
		}
		else {
			retval = mqueue_remove_at_index(queue,index);
			break;
		}
	}
	queue->index = 0;
	queue->indexed = queue->first;
	return retval;
}
