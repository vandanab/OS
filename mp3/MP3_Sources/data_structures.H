/*
    File: data_structures.H

    Author: Vandana Bachani
            Department of Computer Science
            Texas A&M University
    Date  : 03/08/2013

    Description: All data structures needed eg. list.

*/

#ifndef _data_structures_H_                   // include file only once
#define _data_structures_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* List  */
/*--------------------------------------------------------------------------*/

class List {

private:
  Node *head;

public:
  List();
	
	void add(Node *x);
	
	Node *find(unsigned long node_search_attr);
	
	void remove(Node *x);
};

class Node {
public:
	unsigned long start_addr;
	unsigned long size;
};

#endif

