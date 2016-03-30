#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define NUM_OF_SYM 256

typedef struct{
	int freq;
    char sym;
	struct WORD * left;
	struct WORD * right;
	struct WORD * next;
	unsigned int code;
	int code_len;
}WORD;

WORD word_array[NUM_OF_SYM];
//unsigned long code_array[NUM_OF_SYM];

FILE *source_fp, *dest_fp;

typedef int (*compfn)(const void*, const void*); //for compare function

/*----------------------------functions-----------------------------------*/

void show_list() {
	int i;

	for(i = 0; i < sizeof(word_array)/sizeof(WORD); i++) {
		if(word_array[i].sym=='\0'){
			continue;
		}
		printf("%c - %d\t", word_array[i].sym, word_array[i].freq );
	}
}

int compare(WORD *elem1, WORD *elem2) {
//int compare(const void *a, const void *b) {
	if ( elem1->freq < elem2->freq) {
	  return -1;
	}
	if (elem1->freq > elem2->freq) {
	  return 1;
	}
	return 0;
}

void sort_symbols() {
	qsort((void *) word_array, 
	sizeof(word_array)/sizeof(WORD), 
	sizeof(WORD), 
	(compfn)compare );
}

/*typedef struct{
    char sym;
    int code_len;
    int code;
}CODE;*/

void print_binary(unsigned int n, int len) {

	int curr = 0;

	while (curr < len) {
		if (n & 1) { printf("1"); } 
		else { printf("0"); }
		n >>= 1;

		curr++;
	}
	printf("\n");
}

void show_codes(){
	int i = 0;
	unsigned long n;

	while(word_array[i].freq == 0 ) {
		i++;
	}

	if (i == NUM_OF_SYM ){	//empty file
		return NULL;
	}

	for(i; i < NUM_OF_SYM; i++) {
		printf("%c: ", word_array[i].sym);
		print_binary(word_array[i].code, word_array[i].code_len);
	}
	printf("\n");
}

void get_codes(WORD*root, int code, int pos, int code_len) {

	if(root->left == NULL && root->right == NULL) {
		root->code = code;
		root->code_len = code_len;
		return;
	}
	code |= 1 << pos;
	code_len++;
	pos++;
	get_codes(root->right, code, pos, code_len);
	
	pos--;
	code_len--;
	code &= ~(1 << pos);
	code_len++;
	pos++;
	get_codes(root->left, code, pos, code_len);
}

void get_symbols(void) {
	unsigned char c;

	printf("func: get_symbols\n");

	memset(word_array, 0, sizeof(word_array));

	while (!feof(source_fp)) {
	 	size_t readed = fread(&c, sizeof(char), 1, source_fp); 
	 	if(readed>0){
	 		//printf("%d\n", (int)c );
	 		word_array[c].freq++;
	 		word_array[c].sym = c;
	 	}else{
	 		break;
	 	}
	}
	fclose ( source_fp );
}

void error(const char *msg)
{
	//perror();
	//a.out: huff.c:151: create_tree: Assertion `min1 != ((void *)0)' failed.
    fprintf(stderr, "[ERR] file %s/line %d: %s\n", __FILE__, __LINE__, msg);
    exit(EXIT_FAILURE);
}

int help(void) {
	printf("Usage:\n");
	printf(" $ ./huff -s source-file.txt -d dest-file.txt\n");
	return 0;
}

WORD* get_min_element(int *i, WORD **head, WORD *tail) {

	WORD * min_node = *head;
	WORD * min_leaf = NULL;

	if(*i < NUM_OF_SYM) {
		min_leaf = &word_array[(*i)];
	}
	
	if(min_node == NULL) {
		if(min_leaf){
			(*i)++;
		}
		return min_leaf;
	}

	if(min_leaf == NULL){
		if(*head){
			*head = (*head)->next;
		}
		return min_node;
	}
		
	if(min_leaf->freq < min_node->freq) {
		(*i)++;
		return min_leaf;
	}else{
		*head = (*head)->next;
		return min_node;
	}	
}

WORD * create_tree() {

	int i = 0;
	WORD * head = NULL, * tail = NULL, *curr_root;
	WORD *min1 = NULL, *min2 = NULL;

	printf("\nfunc: get_symbols\n");

	while(word_array[i].freq == 0 ) {
		i++;
	}

	if (i == NUM_OF_SYM ){	//empty file
		return NULL;
	}

	//int c = 0;
	//while(/*c++<20 &&*/ (i < NUM_OF_SYM || (head->next) != NULL)) {
	while(1){

		min1 = get_min_element(&i, &head, tail);
		assert(min1 != NULL);
		min2 = get_min_element(&i, &head, tail);
		if (min2 == NULL) {
			//printf("creating binary tree: done\n");
			return min1;
		}
		//printf("mins: %c - %d/%c - %d\n", (*min1).sym, (*min1).freq, (*min2).sym, (*min2).freq );
		
		WORD *node = (WORD*)malloc(1*sizeof(WORD)); /*creating a new node with mins*/
		assert(node != NULL);

		node->freq = min1->freq + min2->freq;
		node->left = min1;
		node->right = min2;
		node->next = NULL;
		
		if(head == NULL) {	
			head = node;
		}else {
			tail->next = node;
		}
		//printf("added a new node: %d\n", node->freq);

		tail = node;
	
		/*printf("total nodes:\n");
		curr_root = head;
		while(curr_root != NULL) {
			printf("%d", curr_root->freq);
			curr_root = curr_root->next;
		}
		printf("\n\n");*/
	}
	assert(0);
}

void huffman(void){
	//printf("the place for my future huffman code\n");
	get_symbols();
	show_list();
	sort_symbols();
	//printf("\n\nAFTER\n\n");
	show_list();
	WORD *tree = create_tree();
	assert(tree!=NULL);
	printf("%d\n", tree->freq);
	get_codes(tree, 0, 0, 0);
	show_codes();
}

void parsing_command_line(int argc, char *argv[], char **source_file, char **dest_file) {

	char *opts = "s:d:";

    int opt;
	while((opt = getopt(argc, argv, opts)) != -1) {
		switch(opt) {
		    case 's': 
			    *source_file = optarg;
			    break;
		    case 'd': 
				*dest_file = optarg;
			    break;
        	case '?': 
            	fprintf(stderr, "invalid optget option");
            	break;
        	default :
            	break;
		}
	}
	if (dest_file == NULL || source_file == NULL) {
		error("invalid optget argument: you didn't choose files");
	}
}

/*.........................main......................................*/

int main(int argc, char *argv[]) {

	char *source_file = NULL, *dest_file = NULL;

	parsing_command_line(argc, argv, &source_file, &dest_file);
	printf("source file: %s, destination file: %s\n", source_file, dest_file);

    if ((source_fp = fopen(source_file, "rb")) == NULL) {
    	error("no such source file");
    }
    if ((dest_fp = fopen(dest_file, "wb")) == NULL) {
    	error("couldn't create destination file");
    }

    huffman();
    printf("\nmission complete\n");

    /*Makefile:2: recipe for target 'all' failed
````make: *** [all] Segmentation fault (core dumped)
````*/

//SEGMENTATION FAULT
    /*fclose(dest_fp);
    fclose(source_fp);*/
    
    return 0;
}




/*
for (;;) {
    size_t n = fread(buf, 1, bufsize, infile);
    consume(buf, n);
    if (n < bufsize) { break; }
}*/     