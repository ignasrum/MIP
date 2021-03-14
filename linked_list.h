#ifndef LINKED_LIST_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Represents a node in linked list */
struct node {
	void *data;
	struct node *next_node;
};

/*
 * Initializes linked list
 * Return: root of new linked list
 */
struct node* initialize_list();

/*
 * Finds size of list
 * @root: root of linked list
 * Return: size of linked list
 */
size_t list_size(struct node *root);

/*
 * Adds new data to linked list
 * @root: root of linked list
 * @data: new data
 * Return: new root of linked list
 */
struct node* add_data(struct node *root, void *data);

/*
 * Removes a node with @data from linked list
 * @root: root of linked list
 * @data: data to remove
 * @data_size: size of data
 * Return: new root of linked list
 */
struct node* remove_node(struct node *root, void *data, size_t data_size);

/*
 * Find node with @data in linked list
 * @root: root of linked list
 * @data: data to find
 * @data_size: size of data
 * Return: node if found, NULL if unsuccessful
 */
struct node* find_node(struct node *root, void *data, size_t data_size);

/*
 * Frees linked list
 * @root: root of linked list
 * Return: 1 if unsuccessful, else 0
 */
int free_linked_list(struct node *root);

#endif
