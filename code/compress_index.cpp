#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Huffman Node Structure
typedef struct HuffmanNode {
    char data;
    unsigned freq;
    struct HuffmanNode *left, *right;
} HuffmanNode;

// Huffman Priority Queue
typedef struct MinHeap {
    unsigned size;
    unsigned capacity;
    HuffmanNode **array;
} MinHeap;

#define MAX_WORD_LEN 256
#define MAX_INDEX_SIZE 10000

// Structure for storing index entries
typedef struct IndexEntry {
    char word[MAX_WORD_LEN];
    long position; // Position in the file
} IndexEntry;

// Function prototypes
void createIndex(const char *input_filename, const char *index_filename);
void searchIndex(const char *index_filename, const char *search_word);
void compressFile(const char *input_filename, const char *output_filename, const char *tree_filename);
void decompressFile(const char *compressed_filename, const char *tree_filename, const char *output_filename);

// Utility functions for Huffman
HuffmanNode *buildHuffmanTree(const char *filename, char **codeTable);
void encodeFile(const char *input_filename, const char *output_filename, char **codeTable);
void decodeFile(const char *compressed_filename, const char *tree_filename, const char *output_filename);
void buildCodeTable(HuffmanNode *roo#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Huffman Node Structure
typedef struct HuffmanNode {
    char data;
    unsigned freq;
    struct HuffmanNode *left, *right;
} HuffmanNode;

// Huffman Priority Queue
typedef struct MinHeap {
    unsigned size;
    unsigned capacity;
    HuffmanNode **array;
} MinHeap;

#define MAX_WORD_LEN 256
#define MAX_INDEX_SIZE 10000

// Structure for storing index entries
typedef struct IndexEntry {
    char word[MAX_WORD_LEN];
    long position; // Position in the file
} IndexEntry;

// Function prototypes
void createIndex(const char *input_filename, const char *index_filename);
void searchIndex(const char *index_filename, const char *search_word);
void compressFile(const char *input_filename, const char *output_filename, const char *tree_filename);
void decompressFile(const char *compressed_filename, const char *tree_filename, const char *output_filename);

// Utility functions for Huffman
HuffmanNode *buildHuffmanTree(const char *filename, char **codeTable);
void encodeFile(const char *input_filename, const char *output_filename, char **codeTable);
void decodeFile(const char *compressed_filename, const char *tree_filename, const char *output_filename);
void buildCodeTable(HuffmanNode *root, char *code, int depth, char **codeTable);
void freeHuffmanTree(HuffmanNode *root);

void createIndex(const char *input_filename, const char *index_filename) {
    FILE *input_file = fopen(input_filename, "r");
    FILE *index_file = fopen(index_filename, "w");
    if (!input_file || !index_file) {
        fprintf(stderr, "Error: Cannot open files for indexing\n");
        return;
    }

    IndexEntry index[MAX_INDEX_SIZE];
    int index_count = 0;

    char buffer[MAX_WORD_LEN];
    long position = ftell(input_file);
    while (fscanf(input_file, "%s", buffer) != EOF) {
        // Clean and normalize the word (convert to lowercase, remove punctuation)
        int len = strlen(buffer);
        for (int i = 0; i < len; i++) {
            if (ispunct(buffer[i])) {
                buffer[i] = '\0';
                break;
            }
            buffer[i] = tolower(buffer[i]);
        }

        // Skip empty words
        if (strlen(buffer) == 0) continue;

        // Add to index
        strcpy(index[index_count].word, buffer);
        index[index_count].position = position;
        index_count++;

        // Check for overflow
        if (index_count >= MAX_INDEX_SIZE) {
            fprintf(stderr, "Error: Index size exceeded maximum limit\n");
            break;
        }

        // Update position
        position = ftell(input_file);
    }

    // Write the index to the file
    for (int i = 0; i < index_count; i++) {
        fprintf(index_file, "%s %ld\n", index[i].word, index[i].position);
    }

    fclose(input_file);
    fclose(index_file);
    printf("Index created: %s\n", index_filename);
}

void searchIndex(const char *index_filename, const char *search_word) {
    FILE *index_file = fopen(index_filename, "r");
    if (!index_file) {
        fprintf(stderr, "Error: Cannot open index file for searching\n");
        return;
    }

    char word[MAX_WORD_LEN];
    long position;
    int found = 0;

    // Normalize the search word (convert to lowercase)
    int len = strlen(search_word);
    char normalized_word[MAX_WORD_LEN];
    for (int i = 0; i < len; i++) {
        normalized_word[i] = tolower(search_word[i]);
    }
    normalized_word[len] = '\0';

    // Search the index
    while (fscanf(index_file, "%s %ld", word, &position) != EOF) {
        if (strcmp(word, normalized_word) == 0) {
            printf("Found '%s' at position %ld\n", search_word, position);
            found = 1;
        }
    }

    if (!found) {
        printf("'%s' not found in the index\n", search_word);
    }

    fclose(index_file);
}

// Huffman Compression Functions
void compressFile(const char *input_filename, const char *output_filename, const char *tree_filename) {
    char *codeTable[256] = {NULL};
    HuffmanNode *root = buildHuffmanTree(input_filename, codeTable);

    // Save the Huffman tree structure to a file for decoding
    FILE *tree_file = fopen(tree_filename, "wb");
    if (!tree_file) {
        fprintf(stderr, "Error: Cannot save Huffman tree\n");
        freeHuffmanTree(root);
        return;
    }
    fwrite(root, sizeof(HuffmanNode), 1, tree_file);
    fclose(tree_file);

    encodeFile(input_filename, output_filename, codeTable);
    freeHuffmanTree(root);

    printf("File compressed: %s\n", output_filename);
}

void decompressFile(const char *compressed_filename, const char *tree_filename, const char *output_filename) {
    FILE *tree_file = fopen(tree_filename, "rb");
    if (!tree_file) {
        fprintf(stderr, "Error: Cannot open Huffman tree file for decoding\n");
        return;
    }

    HuffmanNode *root = malloc(sizeof(HuffmanNode));
    fread(root, sizeof(HuffmanNode), 1, tree_file);
    fclose(tree_file);

    decodeFile(compressed_filename, tree_filename, output_filename);
    freeHuffmanTree(root);

    printf("File decompressed: %s\n", output_filename);
}

// Implement buildHuffmanTree, encodeFile, decodeFile, and freeHuffmanTree based on previous Huffman implementation

int main() {
    // Example files
    const char *original_file = "downloaded_file.html";
    const char *index_file = "downloaded_file_index.txt";
    const char *compressed_file = "downloaded_file.huff";
    const char *tree_file = "huffman_tree.bin";
    const char *decompressed_file = "downloaded_file_decompressed.html";

    // 1. Create an index
    createIndex(original_file, index_file);

    // 2. Compress the original file
    compressFile(original_file, compressed_file, tree_file);

    // 3. Decompress the file
    decompressFile(compressed_file, tree_file, decompressed_file);

    // 4. Search in the index
    char search_word[MAX_WORD_LEN];
    printf("Enter a word to search: ");
    scanf("%s", search_word);
    searchIndex(index_file, search_word);

    return 0;
}
t, char *code, int depth, char **codeTable);
void freeHuffmanTree(HuffmanNode *root);

void createIndex(const char *input_filename, const char *index_filename) {
    FILE *input_file = fopen(input_filename, "r");
    FILE *index_file = fopen(index_filename, "w");
    if (!input_file || !index_file) {
        fprintf(stderr, "Error: Cannot open files for indexing\n");
        return;
    }

    IndexEntry index[MAX_INDEX_SIZE];
    int index_count = 0;

    char buffer[MAX_WORD_LEN];
    long position = ftell(input_file);
    while (fscanf(input_file, "%s", buffer) != EOF) {
        // Clean and normalize the word (convert to lowercase, remove punctuation)
        int len = strlen(buffer);
        for (int i = 0; i < len; i++) {
            if (ispunct(buffer[i])) {
                buffer[i] = '\0';
                break;
            }
            buffer[i] = tolower(buffer[i]);
        }

        // Skip empty words
        if (strlen(buffer) == 0) continue;

        // Add to index
        strcpy(index[index_count].word, buffer);
        index[index_count].position = position;
        index_count++;

        // Check for overflow
        if (index_count >= MAX_INDEX_SIZE) {
            fprintf(stderr, "Error: Index size exceeded maximum limit\n");
            break;
        }

        // Update position
        position = ftell(input_file);
    }

    // Write the index to the file
    for (int i = 0; i < index_count; i++) {
        fprintf(index_file, "%s %ld\n", index[i].word, index[i].position);
    }

    fclose(input_file);
    fclose(index_file);
    printf("Index created: %s\n", index_filename);
}

void searchIndex(const char *index_filename, const char *search_word) {
    FILE *index_file = fopen(index_filename, "r");
    if (!index_file) {
        fprintf(stderr, "Error: Cannot open index file for searching\n");
        return;
    }

    char word[MAX_WORD_LEN];
    long position;
    int found = 0;

    // Normalize the search word (convert to lowercase)
    int len = strlen(search_word);
    char normalized_word[MAX_WORD_LEN];
    for (int i = 0; i < len; i++) {
        normalized_word[i] = tolower(search_word[i]);
    }
    normalized_word[len] = '\0';

    // Search the index
    while (fscanf(index_file, "%s %ld", word, &position) != EOF) {
        if (strcmp(word, normalized_word) == 0) {
            printf("Found '%s' at position %ld\n", search_word, position);
            found = 1;
        }
    }

    if (!found) {
        printf("'%s' not found in the index\n", search_word);
    }

    fclose(index_file);
}

// Huffman Compression Functions
void compressFile(const char *input_filename, const char *output_filename, const char *tree_filename) {
    char *codeTable[256] = {NULL};
    HuffmanNode *root = buildHuffmanTree(input_filename, codeTable);

    // Save the Huffman tree structure to a file for decoding
    FILE *tree_file = fopen(tree_filename, "wb");
    if (!tree_file) {
        fprintf(stderr, "Error: Cannot save Huffman tree\n");
        freeHuffmanTree(root);
        return;
    }
    fwrite(root, sizeof(HuffmanNode), 1, tree_file);
    fclose(tree_file);

    encodeFile(input_filename, output_filename, codeTable);
    freeHuffmanTree(root);

    printf("File compressed: %s\n", output_filename);
}

void decompressFile(const char *compressed_filename, const char *tree_filename, const char *output_filename) {
    FILE *tree_file = fopen(tree_filename, "rb");
    if (!tree_file) {
        fprintf(stderr, "Error: Cannot open Huffman tree file for decoding\n");
        return;
    }

    HuffmanNode *root = malloc(sizeof(HuffmanNode));
    fread(root, sizeof(HuffmanNode), 1, tree_file);
    fclose(tree_file);

    decodeFile(compressed_filename, tree_filename, output_filename);
    freeHuffmanTree(root);

    printf("File decompressed: %s\n", output_filename);
}

// Implement buildHuffmanTree, encodeFile, decodeFile, and freeHuffmanTree based on previous Huffman implementation
