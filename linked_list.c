#include "linked_list.h"

struct node* initialize_list() {
	struct node *root = calloc(1, sizeof(struct node));
	root->data = NULL;
	root->next_node = NULL;
	return root;
}

size_t list_size(struct node *root) {
	size_t size = 0;
	struct node *current = root;
	while(current != NULL) {
		size++;
		current = current->next_node;
	}
	return size;
}

struct node* add_data(struct node *root, void *data) {
	/* checks if root is empty */
	if(root->data == NULL) {
		root->data = data;
		return root;
	}
	/* creates new node and adds it in front */
	struct node *new = calloc(1, sizeof(struct node));
	new->data = data;
	new->next_node = root;
	root = new;
	return root;
}

struct node* remove_node(struct node *root, void *data, size_t data_size) {
	struct node *current = root;
	struct node *previous = NULL;
	while(current != NULL) {
		/* if data is the same */
		if(current->data != NULL && memcmp(current->data, data, data_size) == 0) {
			/* handle different edge cases */
			if(previous == NULL && current->next_node == NULL) {
				printf("clearing root \n");
				free(current->data);
				current->data = NULL;
				current->next_node = NULL;
				return current;
			} else if(previous == NULL) {
				root = current->next_node;
				free(current->data);
				free(current);
				return root;
			} else if(current->next_node == NULL) {
				previous->next_node = NULL;
				free(current->data);
				free(current);
				return root;
			} else {
				previous->next_node = current->next_node;
				free(current->data);
				free(current);
				return root;
			}
		}
		previous = current;
		current = current->next_node;
	}
	return NULL;
}

struct node* find_node(struct node *root, void *data, size_t data_size) {
	struct node *current = root;
	while(current != NULL) {
		/* if data is the same return node */
		if(memcmp(current->data, data, data_size) == 0) {
			return current;
		}
		current = current->next_node;
	}
	return NULL;
}

int free_linked_list(struct node *root) {
	struct node *current = root;
	struct node* previous = NULL;
	while(current != NULL) {
		/* free */
		previous = current;
		current = current->next_node;
		if(previous->data != NULL) {
			free(previous->data);
		}
		free(previous);
	}
	return 0;
}
