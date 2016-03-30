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

/*NODE * allocate_memory(NODE * node_p, int index, NODE ** left_p, NODE ** right_p){
	NODE *tree_node = (NODE*)malloc(1*sizeof(NODE));
	if (tree_node != NULL) {
		tree_node->freq = word_array[index].freq + word_array[index+1].freq;
		tree_node->left = *left_p;
		tree_node->right = *right_p;
		curr_root = tree_node;
		i++; 
		return tree_node;
	}else{
		error("malloc doesn't work");
	}
}*/

/*void get_min_elements(WORD ** min1, WORD ** min2, int index, WORD **curr_root) {

	*min1 = word_array[index];
	printf("test1\n\n");

	if (curr_root == NULL || (**curr_root).freq <= word_array[index + 1].freq) {
		printf("test1\n\n");
		min2 = curr_root;
		return;
	}else {
		printf("test2\n\n");
		**min2 = word_array[index+1];
	}
}*/

WORD * create_tree() {

	int i = 0;
	WORD * head = NULL, * tail;
	WORD *min1 = NULL, *min2 = NULL;

	while(word_array[i].freq == 0 ) {
		i++;
	}

	if (i == NUM_OF_SYM ){
		printf("no symbols in your file? exit\n");
		//не закрывать, а тоже обрабатывать
		return NULL;
	}

	while(i + 1 < NUM_OF_SYM && (head->next) != NULL) {
		//get_min_elements(&min1, &min2, i, &root);

		min1 = &word_array[i];

		if(head == NULL || head->freq < word_array[i + 1].freq) {
			min2 = &word_array[i+1];
		}else {
			min2 = head;
		}
		printf("1\n");
		/*creating a new node with mins*/
		WORD *node = (WORD*)malloc(1*sizeof(WORD));
		if (node != NULL) {
			printf("2\n");
			node->freq = min1->freq + min2->freq;
			node->left = min1;
			node->right = min2;
			printf("3\n");
			if(min2 == head) {
				printf("4\n");
				node->next = head->next;
				if(head == tail) {
					tail = node;
				}
				printf("5\n");
				i++;
			} else if (min2 == &(word_array[i + 1])) {
				printf("6\n");
				tail->next = node;
				node = tail;
				i += 2;
				printf("7\n");
			}
		}else{
			error("malloc doesn't work");
		}

	/* printf("\ngot mins: %c - %d\n", min1->sym, min1->freq );
	printf("got mins: %c - %d\n", min2->sym, min2->freq );
	/*NODE * tree_node = allocate_memory(i, NULL, NULL);*/
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
	//WORD *tree = create_tree();
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