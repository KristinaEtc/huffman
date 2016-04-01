#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <string.h>

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
WORD unhuff_word_array[NUM_OF_SYM];

FILE *source_fp, *dest_fp, *test_fp;
char magic[] = "huff";
const short version = 1;
const char crc_begin[] = "crc_begin", crc_end[] = "crc_end";	//crc for huffman vocabilary

typedef int (*compfn)(const void*, const void*); //for compare function

/*----------------------------functions-----------------------------------*/

void show_list(WORD *array) {
	int i;

	for(i = 0; i < NUM_OF_SYM; i++) {
		if((array[i]).sym=='\0'){
			continue;
		}
		printf("%c - %d\t", array[i].sym, array[i].freq );
	}
}

int compare(WORD *elem1, WORD *elem2) {
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

void print_binary(unsigned int n, int len) {

	int curr = 0;

	while (curr < len) {
		if(curr%8==0) printf(" ");
		if (n & 1) { printf("1"); } 
		else { printf("0"); }
		n >>= 1;

		curr++;
	}
	printf("\n");
}

void show_codes(WORD *array){
	int i = 0;
	unsigned long n;

	while(array[i].freq == 0 ) {
		i++;
	}

	if (i == NUM_OF_SYM ){	//empty file
		return NULL;
	}

	for(i; i < NUM_OF_SYM; i++) {
		printf("%c: ", array[i].sym);
		print_binary(array[i].code, array[i].code_len);
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
	get_codes(root->right, code, pos + 1, code_len + 1);
	
	code &= ~(1 << pos);
	get_codes(root->left, code, pos + 1, code_len + 1);
}

void get_symbols(void) {
	unsigned char c;

	memset(word_array, 0, sizeof(word_array));

	while (!feof(source_fp)) {
	 	size_t readed = fread(&c, sizeof(char), 1, source_fp); 
	 	if(readed > 0){
	 		word_array[c].freq++;
	 		word_array[c].sym = c;
	 	}else{
	 		break;
	 	}
	}
}

void error(const char *msg) {
	close_files();
	//remove(filename); //имя файла dest должно быть глобальным, чтобы можно было удалить в случае ошибки

    fprintf(stderr, "[ERR] file %s/line %d: %s\n", __FILE__, __LINE__, msg);
    exit(EXIT_FAILURE);
}

int help(void) {
	printf("Usage: encode file\n");
	printf(" $ ./huff -s source-file.txt -d dest-file.txt\n");
	printf("To uncode use flag '-u':\n");
	printf(" $ ./huff -u -s source-file.txt -d dest-file.txt\n");
	return 0;
}

WORD* get_min_element(int *i, WORD **head, WORD *tail, WORD* array) {

	WORD * min_node = *head;
	WORD * min_leaf = NULL;

	if(*i < NUM_OF_SYM) {
		min_leaf = &array[(*i)];
	}
	
	if(min_node == NULL) {
		if(min_leaf){
			(*i)++;
		}
		return min_leaf;
	}

	if(min_leaf == NULL){
		if(*head) {
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

WORD * create_tree(WORD * array) {

	int i = 0;
	WORD * head = NULL, * tail = NULL, *curr_root;
	WORD *min1 = NULL, *min2 = NULL;

	while(array[i].freq == 0 ) {
		i++;
	}

	if (i == NUM_OF_SYM ){	//empty file
		return NULL;
	}

	while(1){

		min1 = get_min_element(&i, &head, tail, array);
		assert(min1 != NULL);
		min2 = get_min_element(&i, &head, tail, array);
		if (min2 == NULL) {
			return min1;
		}
		
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

		tail = node;
	}
	assert(0);
}

// crc for huffman vocabilary
void crc_vocabilary(int *begin, int *end){
	return;
}

void add_to_buff(char *buf, int *pos_in_buf, int code_len, unsigned int curr_code){
	
	char bit;

	size_t writed;

    int pos_in_code = 0;
    while(pos_in_code < code_len){
    	bit = ((curr_code & (1<<pos_in_code)) >> pos_in_code);
    	
    	if(*pos_in_buf == (sizeof(*buf)*8)){
    		writed = fwrite(buf, sizeof(*buf), 1, dest_fp);
    		print_binary(*buf, sizeof(*buf)*8);
			assert(writed > 0);
			*pos_in_buf = 0;
			(*buf) &= 0;
    	}
		(*buf) |= (bit << (*pos_in_buf));
		(*pos_in_buf)++;
 
      	pos_in_code++;
    }
}

void generate_eof(int *my_eof, int * my_eof_len ){
	*my_eof = 5555;
	*my_eof_len = 13; //in bits
}

int write_data(){

	unsigned char c;
	char buf;
	buf &= 0;
	
	unsigned int curr_code;
	int code_len;

	size_t readed, writed;

	fseek(source_fp, 0L, SEEK_SET);
	rewind(source_fp);

	int pos_in_buf = 0;
	while (!feof(source_fp)) {
	 	readed = fread(&c, sizeof(char), 1, source_fp); 
	 	if(readed > 0){

			//founding symbol in a sorted vocabilary
			int i = 0;
			while(word_array[i].sym != c) { i++; }
			if(i == NUM_OF_SYM) { return; }

			code_len = word_array[i].code_len;
			curr_code = word_array[i].code;

	 		add_to_buff(&buf, &pos_in_buf, code_len, curr_code);
	 	}else{
	 		break;
	 	}	
	}
	
	//adding unicum EOF
	generate_eof(&curr_code, &code_len);

	int pos = 0;
	unsigned int curr_my_eof;
	while(pos < code_len){
    	c = (char)curr_code;
    	add_to_buff(&buf, &pos_in_buf, 8, c);
    	curr_code >>= 8;
    	pos+=8;
    }

	writed = fwrite(&buf, sizeof(buf), 1, dest_fp);
	assert(writed > 0);
	return 0;
}

int write_vocabilary(){

	int i = 0, pairs_num = 0;

	while(word_array[i].freq == 0 ) {
		i++;
	}
	
	if(i == NUM_OF_SYM) {
		return -1;
	}

	crc_vocabilary(&crc_begin, &crc_end);
	
	size_t writed = fwrite(magic, sizeof(magic) - 1, 1, dest_fp);
	assert(writed > 0);
	writed = fwrite(&version, sizeof(version), 1, dest_fp);
	assert(writed > 0);
	writed = fwrite(crc_begin, sizeof(crc_begin) - 1, 1, dest_fp);
	assert(writed > 0);
	
	pairs_num = NUM_OF_SYM - i;
	writed = fwrite(&pairs_num, sizeof(pairs_num), 1, dest_fp);
	assert(writed > 0);

	for(i; i < NUM_OF_SYM; i++) {
		writed = fwrite(&(word_array[i].sym), sizeof(char), 1, dest_fp);
		assert(writed > 0);
		writed = fwrite(&(word_array[i].freq), sizeof(word_array[i].freq), 1, dest_fp);
		assert(writed > 0);
	}
	return 0;
}

void write_encoded_file() {
/*
huffman file structure:
magic|version|crc_begin|pairs_num (nums of "sym-freq" pairs)|
code vocabilary(sym1-freq1, sym2 - freq2, ...)|crc_end|

encodedData

|encodedEOF
*/
	int error;

	error = write_vocabilary();
	assert(error == 0);
	error = write_data();
	assert(!error);
	size_t writed = fwrite(crc_end, sizeof(crc_end)-1, 1, dest_fp);
	assert(writed > 0);
}

int get_vocabilary() {

	char src_magic[sizeof(magic)];
	char src_crc_begin[sizeof(crc_begin)], src_crc_end[sizeof(crc_end)];
	char c;
	int freq, sym_pairs;
	short int src_version;

	int i = 0;

	memset(unhuff_word_array, 0, sizeof(unhuff_word_array));

	size_t readed = fread(src_magic, sizeof(magic) - 1, 1, source_fp);
	if(readed > 0) {
		src_magic[sizeof(magic)-1] = '\0';
		if(strcmp(src_magic, magic) != 0) {
			error("invalid magic: could not unhuffman your file :(");
		}
	}else {
		error("invalid magic: wrong file? :(");	
	}

	readed = fread(&src_version, sizeof(version), 1, source_fp);
	if(readed > 0) {
		if(src_version != version) {
			error("invalid huffman version: could not unhuffman your file :(");
		}
	}else {
		error("invalid version: wrong file? :(");	
	}

	readed = fread(src_crc_begin, sizeof(crc_begin) - 1, 1, source_fp);
	if(readed > 0) {
		src_crc_begin[sizeof(crc_begin)-1] = '\0';
		if(strcmp(src_crc_begin, crc_begin) != 0) {
			error("invalid crc: could not unhuffman your file :(");
		}
	}else {
		error("invalid crc: file is corrupted :(");	
	}

	readed = fread(&sym_pairs, sizeof(sym_pairs), 1, source_fp);
	if(readed < 0) {
		error("invalid file structure: file is corrupted :(");	
	}

	i = NUM_OF_SYM - sym_pairs;
	while (!feof(dest_fp) && (i < NUM_OF_SYM)) {
		
		readed = fread(&c, sizeof(char), 1, source_fp); 
		if(readed > 0) {
			unhuff_word_array[i].sym = c;
			printf("%c - ",unhuff_word_array[i].sym  );
		}else { 
			break; 
		}
		
		readed = fread(&freq, sizeof(freq), 1, source_fp); 
		if(readed > 0) {
			unhuff_word_array[i].freq = freq;
			printf("%d\n ",unhuff_word_array[i].freq  );
		}else { 
			error("could not process file :(");
			return -1; 
		}
		//printf("%c - %d\n", unhuff_word_array[i].sym, unhuff_word_array[i].freq );
		i++;
	}

	readed = fread(src_crc_end, sizeof(crc_end) - 1, 1, source_fp);
	if(readed > 0) {
		src_crc_end[sizeof(crc_end)-1] = '\0';
		if(strcmp(src_crc_end, crc_end) != 0) {
			printf("%s - %s\n", crc_end, src_crc_end );
			error("invalid crc: could not unhuffman your file :(");
		}
	}

	return i;
}

void close_files() {
	fcloseall();
	/*if (!fclose(source_fp)){ //добавить отмену еоф
		error("could not close a source file");
	}
	if (!fclose(dest_fp)) {
		error("could not close a destination file");
	}*/
}

void open_files(char * source, char * dest ) {

	printf("source file: %s, destination file: %s\n", source, dest);

	if ((source_fp = fopen(source, "rb")) == NULL) {
		error("no such source file");
	}

    if ((dest_fp = fopen(dest, "wb")) == NULL) {
    	error("couldn't create destination file");
    }
}

write_final(){
}

void huffman() {

	//добавить указатели на функции и запускать их в цикле
	get_symbols();
	sort_symbols();
	show_list(&word_array);
	
	WORD *tree = create_tree(&word_array);
	if(tree == NULL) {
		close_files(); //qqq
	}
	printf("num of elements: %d\n", tree->freq);
	
	get_codes(tree, 0, 0, 0);
	show_codes(&word_array);

	write_encoded_file();
	close_files();

	/*printf("\n--------\nunhuffman part:\n\n");

	open_files("d.txt", "d1.txt"); 

	get_vocabilary();
	show_list(&unhuff_word_array);
	WORD *unhuff_tree = create_tree(&unhuff_word_array);
	if(tree == NULL) {
		close_files(); //qqq
	}
	printf("num of elements: %d\n", unhuff_tree->freq);

	get_codes(unhuff_tree, 0, 0, 0);
	show_codes(&unhuff_word_array);


	
	//write_final();

	close_files();*/
	printf("\ndone.\n");
}

char parsing_command_line(int argc, char *argv[], char **source_file, char **dest_file) {

	char *opts = "s:d:u";
	char what_to_do = 'h';	//'-h' as default 

    int opt;
	while((opt = getopt(argc, argv, opts)) != -1) {
		switch(opt) {
		    case 's': 
			    *source_file = optarg;
			    break;
		    case 'd': 
				*dest_file = optarg;
				break;
			case 'u':
				what_to_do = 'u';
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
	return what_to_do;
}

/*.........................main......................................*/

int main(int argc, char *argv[]) {

	char *src_fname = NULL, *dst_fname = NULL;

	char what_to_do = parsing_command_line(argc, argv, &src_fname, &dst_fname);

	open_files(src_fname, dst_fname);

	//if(what_to_do == 'u') { unhuffman(); }
	//else { huffman(); }

	huffman();
    
    return 0;
}