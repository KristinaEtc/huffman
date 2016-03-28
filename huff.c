#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*struct tree {
  char sym;
  struct tree *left;
  struct tree *right;
};*/
FILE *source_fp, *dest_fp;

typedef struct{
    char sym;
	int freq;
	struct WORD * next;
}WORD;

WORD *word_array = NULL;

void show_list() {

	WORD * list_p = word_array;
	while(list_p != NULL) {
		printf("%c - %d\n", list_p->sym, list_p->freq );
		list_p = list_p->next;
	}
}

void get_symbols(void) {
	//struct word * word_item = NULL;
	char c;
	WORD * curr_word = word_array, *last_item = word_array;
	int sym_checked = 0;

	printf("func: get_symbols\n");

	while (1) {
	 	fread(&c, sizeof(char), 1, source_fp); 
	 	if (feof(source_fp)){
	 		printf("end of source_file\n");
	 		show_list();
	 		break;
	 	}
	 	printf("curr sym = %c\n", c);
	 	while (curr_word != NULL){
	 		if(curr_word->sym == c) {
	 			curr_word->freq = (curr_word->freq)+ 1;
	 			printf("repeating word: %c - %d \n", c, curr_word->freq);
	 			sym_checked = 1;
	 		}else{
	 			last_item = curr_word;
	
	 		}
	 		curr_word = curr_word->next;
	 	}
	 	if(sym_checked == 0) {
	 		printf("adding a new sym = %c\n", c);
	 		WORD * word;
		 	if ((word = (WORD*)malloc(1*sizeof(WORD)) ) != NULL) {
		 		word->sym = c;
			 	word->freq = 1;
			 	word->next = NULL;
			 } else {
		 		error("creating a new word struct");
		 	}
		 	if(word_array == NULL) {
		 		word_array = word;
		 		printf("%c\n", word_array->sym );
		 		curr_word = word_array;
		 		last_item = word_array;
		 		printf("%c\n", last_item->sym );
		 	}else {
		 		printf("last it c\n", last_item->sym );
		 		last_item->next = word;
		 		last_item = word;
		 		curr_word = word_array;
		 	}
	 	}else {
	 		sym_checked = 0;
	 		curr_word = word_array;
	 	}
	}
	fclose ( source_fp );
	/*while(curr_word != NULL) {
		printf("%c", curr_word->sym );
		curr_word = curr_word->next;
	}*/
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

void huffman(void){
	printf("the place for my future huffman code\n");
	get_symbols();
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
    printf("mission complete\n");

    /*Makefile:2: recipe for target 'all' failed
````make: *** [all] Segmentation fault (core dumped)
````*/

//SEGMENTATION FAULT
    /*fclose(dest_fp);
    fclose(source_fp);*/
    
    return 0;
}