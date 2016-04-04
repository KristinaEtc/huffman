#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include <stdint.h>

#define NUM_OF_SYM 257
//#define EOF_MARKER 1

#define error(msg) do { fprintf(stderr, "[ERR] file %s/line %d: %s\n", __FILE__, __LINE__, msg); exit(EXIT_FAILURE); } while(0)

#define BUF_SIZE 65536

typedef struct WORD_S WORD;

struct WORD_S{
    int freq;
    uint8_t sym;
    WORD * left;
    WORD * right;
    WORD * next;
    uint64_t code;
    int code_len;
    char eof_flag;
};

WORD word_array[NUM_OF_SYM];
WORD unhuff_word_array[NUM_OF_SYM];
WORD *tree;

FILE *source_fp, *dest_fp, *test_fp;
char magic[] = "huff";
const short version = 1;
const char crc_begin[] = "crc_begin", crc_end[] = "crc_end";    /*crc for huffman vocabilary*/

uint64_t my_eof;
int my_eof_len;
int verbose = 0;

typedef int (*compfn)(const void*, const void*); /*for compare function*/

/*----------------------------functions-----------------------------------*/

void show_list(WORD *array) {
    int i;

    for(i = 0; i < NUM_OF_SYM; i++) {
        if((array[i]).sym=='\0'){
            continue;
        }
        /*printf("%c - %d\t", array[i].sym, array[i].freq );*/
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

void print_binary(uint64_t n, int len) {

    //if(!verbose){
    //    return;
    //}
    uint64_t bit = 1<<(len-1);

    while (len > 0) {
        if(len%8==0) printf(" ");
        if (n & bit) { 
           printf("1"); 
        }else { 
           printf("0"); 
        }
        len--;
        bit >>= 1;
    }

    printf("\n");
}

void show_codes(WORD *array){
    int i = 0;

    while(i < NUM_OF_SYM && array[i].freq == 0 ) {
        i++;
    }
    assert(i < NUM_OF_SYM);

    for(; i < NUM_OF_SYM; i++) {
        char c = array[i].sym;
        if(c<32){
            c = 32;
        }
        printf("\nsym: %c /in hex: %x / code len: %d\t ", c, array[i].sym, array[i].code_len);
        //print_binary(array[i].code, array[i].code_len);
        print_binary(array[i].code, 64);
    }
    printf("\n");
}

void get_codes(WORD*root, uint64_t code, unsigned int pos, int code_len) {

    assert(code_len <= sizeof(code)*8);
    if(root->left == NULL && root->right == NULL) {
        root->code = code;
        root->code_len = code_len;
        return;
    }

    code |= ((uint64_t)1 << (pos - 1));
    get_codes(root->right, code, pos - 1, code_len + 1);
    
    code &= ~((uint64_t)1 << (pos - 1));
    get_codes(root->left, code, pos - 1, code_len + 1);
}

void get_symbols(void) {
    uint8_t c;
    uint8_t buf[BUF_SIZE];
    size_t i;

    memset(word_array, 0, sizeof(word_array));

    while (!feof(source_fp)) {
        size_t readed = fread(buf, sizeof(uint8_t), BUF_SIZE, source_fp);
        //printf("%d\n", readed); 
        if(readed > 0){
            
            for(i = 0; i < readed; i++){
                c = buf[i];
                word_array[c].freq++;
                word_array[c].sym = c;
            }
        }else{
            break;
        }
    }
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

void add_eof(WORD *array){

    int i = 0;

    while(i < NUM_OF_SYM && (array[i]).freq == 0 ) {
        i++;
    }

    my_eof = (array[i]).code;
    my_eof_len = (array[i]).code_len;
    if(verbose){
        printf("eof len %d\t", my_eof_len);
        print_binary(my_eof, 64);
    }
    return;
}

WORD * create_tree(WORD * array) {

    int i = 0;
    WORD * head = NULL, * tail = NULL;
    WORD *min1 = NULL, *min2 = NULL;

    while(i < NUM_OF_SYM && array[i].freq == 0 ) {
        i++;
    }

    if (i == NUM_OF_SYM ){  /*empty file*/
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

/* crc for huffman vocabilary*/
void crc_vocabilary(int *begin, int *end){
    return;
}

void add_to_buff(uint8_t *buf, int *pos_in_buf, int code_len, uint64_t code){
    
    uint64_t bit;
    uint64_t curr_code = code;
/*76543210
  11100000*/

    size_t writed;
    /*printf("got buf: %d ", *pos_in_buf);
    print_binary(*buf, 8);
    printf("curr_code ");
    print_binary(curr_code, sizeof(curr_code)*8);*/

    int pos_in_code = sizeof(code)*8 - 1;
    /*printf("pos int code %d - intill %d\n", pos_in_code, (sizeof(code_len)*8 - code_len) );*/
    while(pos_in_code >= (sizeof(code)*8 - code_len)){
        bit = ((curr_code & ((uint64_t)1<<pos_in_code))>> pos_in_code);
        
        if(*pos_in_buf == 0) {
            writed = fwrite(buf, sizeof(*buf), 1, dest_fp);
            /*if(verbose){
                printf("here\n");
                print_binary(*buf, sizeof(*buf)*8);
            }*/

            assert(writed > 0);
            *pos_in_buf = 8;
            (*buf) &= 0;
        }
        //printf("%d ", bit );

        (*buf) |= (bit << ((*pos_in_buf) - 1));
        (*pos_in_buf)--;

        pos_in_code--;
    }
}

int write_data(){

    uint8_t c;
    uint8_t buf;
    buf &= 0;

    uint8_t read_buf[BUF_SIZE];
    size_t j;
    
    uint64_t curr_code;
    int code_len;

    size_t readed, writed;

    fseek(source_fp, 0L, SEEK_SET);
    rewind(source_fp);

    int pos_in_buf = sizeof(buf)*8;
    while (!feof(source_fp)) {
        readed = fread(read_buf, sizeof(read_buf[0]), sizeof(read_buf), source_fp); 
        if(readed > 0){
            for(j = 0; j < readed; j++){
                c = read_buf[j];
               
                /*founding symbol in a sorted vocabilary*/
                int i = 0;
                while(i < NUM_OF_SYM && (word_array[i].sym != c || word_array[i].eof_flag == 1 || word_array[i].freq==0)) { i++; }
                assert(i < NUM_OF_SYM);

                code_len = word_array[i].code_len;
                curr_code = word_array[i].code;
                if(verbose){
                    printf("code: ");
                    print_binary(curr_code, 64);
                }

                add_to_buff(&buf, &pos_in_buf, code_len, curr_code);
            }
        }else{
            break;
        }   
    }
    
    /*adding my_EOF*/
    add_to_buff(&buf, &pos_in_buf, my_eof_len, my_eof);
    if(verbose){
        printf("my eof: ");
        print_binary(my_eof, 64);
    }

    writed = fwrite(&buf, sizeof(buf), 1, dest_fp);
    assert(writed > 0);
    return 0;
}

int write_vocabilary(){

    int i = 0, pairs_num = 0;

    while(i < NUM_OF_SYM && word_array[i].freq == 0 ) {
        i++;
    }
    
    if(i == NUM_OF_SYM) {
        return -1;
    }

    /*crc_vocabilary(&crc_begin, &crc_end);*/
    
    size_t writed = fwrite(magic, sizeof(magic) - 1, 1, dest_fp);
    assert(writed > 0);
    writed = fwrite(&version, sizeof(version), 1, dest_fp);
    assert(writed > 0);
    writed = fwrite(crc_begin, sizeof(crc_begin) - 1, 1, dest_fp);
    assert(writed > 0);
    
    pairs_num = NUM_OF_SYM - i;
    if(verbose){
         printf("%d\n", pairs_num);
    }

    writed = fwrite(&pairs_num, sizeof(pairs_num), 1, dest_fp);
    assert(writed > 0);

    for(; i < NUM_OF_SYM; i++) {
        writed = fwrite(&(word_array[i].sym), sizeof(word_array[i].sym), 1, dest_fp);
        assert(writed > 0);
        writed = fwrite(&(word_array[i].freq), sizeof(word_array[i].freq), 1, dest_fp);
        assert(writed > 0);
        writed = fwrite(&(word_array[i].eof_flag), sizeof(word_array[i].eof_flag), 1, dest_fp);
        assert(writed > 0);
    }

    writed = fwrite(crc_end, sizeof(crc_end)-1, 1, dest_fp);
    assert(writed > 0);

    return 0;
}

void write_encoded_file() {
/*
huffman file structure:
magic|version|crc_begin|pairs_num (nums of "sym-freq" pairs)|
code vocabilary(sym1-freq1, sym2 - freq2, ...)|crc_end|

encodedData

|myEOF
*/
    int error;

    error = write_vocabilary();
    assert(error == 0);
    error = write_data();
    assert(!error);
}

int get_vocabilary() {

    char src_magic[sizeof(magic)];
    char src_crc_begin[sizeof(crc_begin)], src_crc_end[sizeof(crc_end)];
    char c;
    char flag;
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
            if(verbose){
                printf("%c - ",unhuff_word_array[i].sym  );
            }
        }else { 
            break; 
        }
        
        readed = fread(&freq, sizeof(freq), 1, source_fp); 
        if(readed > 0) {
            unhuff_word_array[i].freq = freq;
            if(verbose){
                printf("/%d/ ",unhuff_word_array[i].freq  );
            }
        }else { 
            error("could not process file :(");
            return -1; 
        }

        readed = fread(&flag, sizeof(flag), 1, source_fp); 
        if(readed > 0) {
            unhuff_word_array[i].eof_flag = flag;
            if(verbose){
                printf("%c\n ",unhuff_word_array[i].eof_flag  );
            }
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
  //  printf("%d", sizeof(crc_end) -1)

    return i;
}

void close_files() {
    if (fclose(source_fp)){ 
        error("could not close a source file");
    }
    if (fclose(dest_fp)) {
        error("could not close a destination file");
    }
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

int write_sym(uint8_t buf, WORD** c_tree){
    
    int pos_in_buf = sizeof(buf)*8;

    unsigned int bit;

    size_t writed;

    WORD * curr_tree = *c_tree;

    while (pos_in_buf >= 0) {
        
        if(curr_tree->right == NULL ){
            if(verbose){
                printf("\ngot leaf: %2x\tpos in buf: %d\n", curr_tree->sym, pos_in_buf); //hex 16 bit
                print_binary(curr_tree->code, 64);
                print_binary(my_eof, 64);
            }
            if(curr_tree->code == my_eof){
             /*   if(curr_tree->eof_flag == 1){*/
                    printf("eof\n");
                    return 1;
             //   }  
            }
            writed = fwrite(&(curr_tree->sym), sizeof(curr_tree->sym), 1, dest_fp);
            if(writed < 0){
                error("could not write to file");
            }
            curr_tree = tree;
        }  

        bit = (buf >> (pos_in_buf - 1)) & 1;
        if(pos_in_buf == 0) {
            *c_tree = curr_tree;
            return 0;
        }

        if (bit & 1) { 
            if(verbose){
                printf("1");
            }
            curr_tree = curr_tree->right;
        } 
        else { 
            if(verbose){
                printf("0");
            }
            curr_tree = curr_tree->left;
        }  
        pos_in_buf--;
        //curr_c = buf;
        //c >>= 1;
    }



    return 0;
}

void write_res_file(){

    uint8_t buf;
    
    WORD * curr_tree = tree;

    size_t readed;

    /*fseek(source_fp, 0L, SEEK_SET);
    rewind(source_fp);

    int pos_in_buf = (sizeof(buf))*8;*/
    while (!feof(source_fp)) {
        readed = fread(&buf, sizeof(buf), 1, source_fp); 
        if(readed > 0){
            if(verbose){
                printf("bufpuf: ");
                print_binary(buf, 8);
            }
            int got = write_sym(buf, &curr_tree);
            if(got){
                break;
            }
        }else{
            break;
        }   
    }
}

void free_memory(WORD *root){

    //do nothing if passed a non-existent node
    if (root == NULL){
        return;
    }

    //now onto the recursion
    free_memory(root->left);
    free_memory(root->right);

    if(root->left != NULL)
        free(root);
}


void end_work(){

    free_memory(tree);
    close_files();
    if(verbose){
        printf("\ndone.\n");
    }
}

void unhuffman() {

    if(verbose){
        printf("unhuffman\n\n");
    } 

    get_vocabilary();
    show_list(unhuff_word_array);
    tree = create_tree(unhuff_word_array);
    assert(tree != NULL);

    if(verbose){
        printf("num of elements: %d\n", tree->freq);
    }

    get_codes(tree, 0, sizeof(tree->code)*8, 0);
    add_eof(unhuff_word_array);
    
    if(verbose){
      show_codes(unhuff_word_array);  
    }

    write_res_file();

    end_work();
}

void add_eof_to_array(WORD *array){

    int i = 0;
    while(i < NUM_OF_SYM && array[i].freq == 0 ) {
        i++;
    }

    assert(i>0);
    /*if (i == NUM_OF_SYM ){  //empty file
        return;
    }*/

    i--;
    /*array[i].sym = 'e';*/
    array[i].freq = 1;
    array[i].eof_flag = 1;
}

void huffman() {

    /*добавить указатели на функции и запускать их в цикле*/
    get_symbols();
    sort_symbols();
    if(verbose){
        show_list(word_array);
    }
    add_eof_to_array(word_array);
    
    tree = create_tree(word_array);
    assert(tree != NULL);
    if(verbose){
        printf("num of elements: %d\n", tree->freq);
    }
    
    get_codes(tree, 0, sizeof(tree->code)*8, 0);
    add_eof(word_array);

    if(verbose){
        show_codes(word_array);
    }
    write_encoded_file();

    end_work();

    if(verbose){
        printf("\ndone.\n");
    }
}

char parsing_command_line(int argc, char *argv[], char **source_file, char **dest_file) {

    char *opts = "vs:d:u";
    char what_to_do = 'h';  /*'-h' as default */

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
            case 'v':
                verbose = 1;
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

    if(what_to_do == 'u') { unhuffman(); }
    else { huffman(); }
    
    return 0;
}