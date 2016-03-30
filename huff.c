#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

FILE *source_fp, *dest_fp;

typedef struct{
	int freq;
    char sym;
	struct WORD * left;
	struct WORD * right;
	struct WORD * next;
}WORD;

#define NUM_OF_SYM 256

WORD word_array[NUM_OF_SYM];

void show_list() {
	int i;
	for(i = 0; i < sizeof(word_array)/sizeof(WORD); i++) {
		if(word_array[i].sym=='\0'){
			continue;
		}
		//printf("%3d - %d\t", (int)word_array[i].sym, word_array[i].freq );
		printf("%c - %d\t", word_array[i].sym, word_array[i].freq );
	}
}
typedef int (*compfn)(const void*, const void*);

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

void get_symbols(void) {
	char c;

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
    fprintf(stderr, "[ERR] file %s/line %d: %s\n", __FILE__, __LINE__, msg);
    exit(EXIT_FAILURE);
}

int help(void) {
	printf("Usage:\n");
	printf(" $ ./huff -s source-file.txt -d dest-file.txt\n");
	return 0;
}

WORD* get_min_elements(WORD ** min1, WORD ** min2, int i, WORD **head, WORD *tail) {
	
	if(i < NUM_OF_SYM){
		*min1 = &word_array[i];
	}else if (head == tail){ //1 element - final tree
		return head;
	} else if(head != tail) {
		*min1 = *head;
		*head = (*head)->next;
	}

	if(*head == NULL || ( (i+ 1 < NUM_OF_SYM) && ((*head)->freq > word_array[i + 1].freq))) {
	/**/
		*min2 = &word_array[i+1];
	}else {
		*min2 = *head;
	}

	return NULL;
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

	while(i < NUM_OF_SYM || (head->next) != NULL) {

		WORD * final_head = get_min_elements(&min1, &min2, i, &head, tail);
		if (final_head != NULL) {
			printf("creating binary tree: done\n");
			return final_head;
		}
		printf("mins: %c - %d/%c - %d\n", (*min1).sym, (*min1).freq, (*min2).sym, (*min2).freq );
		
		WORD *node = (WORD*)malloc(1*sizeof(WORD)); /*creating a new node with mins*/
		if (node != NULL) {
			node->freq = min1->freq + min2->freq;
			node->left = min1;
			node->right = min2;
			node->next = NULL;
			
			if(min2 == head) {
				if(head == tail) {	//only 1 (or 0) node	
					head = node;
				}else {				//more than 1 nodes
					head = head->next;
					tail->next = node;
				}
				i++;
			} else {	//min2 is a leaf
				if(head == NULL) {	
					head = node;
				}else{
					tail->next = node;
				}
				i += 2;
				printf("added a new node: %d\n", node->freq);
			}
			tail = node;
		}else{
			error("malloc doesn't work");
		}
		
		printf("total nodes:\n");
		curr_root = head;
		while(curr_root != NULL) {
			printf("%d", curr_root->freq);
			curr_root = curr_root->next;
		}
		printf("\n\n");
	}

	return head;
}

void huffman(void){
	//printf("the place for my future huffman code\n");
	get_symbols();
	show_list();
	sort_symbols();
	//printf("\n\nAFTER\n\n");
	show_list();
	WORD *tree = create_tree();
	printf("%d\n", tree->freq );
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