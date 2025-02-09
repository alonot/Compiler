/**
 * implementation of hashmap using quadratic hashing
 * to support symbol table
 */

#include "includes.h"

#define ulli unsigned long long int
#define MIN_HASH 5
/**
 * Converts string to int for hashing it
 * Gives different weights to different positions hence enforces ordering.
 */
ulli hash_string(char* str) {
    ulli val = 0;
    int len = strlen(str);
    for (int i =0; i < len; i ++) {
        val += ((i + 1) * str[i]) % LLONG_MAX;
    }
    return val;
}

short compare_string(char* s1, char* s2) {
    if (s1 == INT_MIN || s2 == INT_MIN || s1 == NULL || s2 == NULL) {
        return -1;
    }
    // printf("%s %s\n", s1, s2);
    return (short)strcmp(s1, s2);
}

ulli hash_int(char* num) {
    return (int)num;
}

HashMap* init_hash_map(ulli (*hash)(char*), short (*compare)(char* , char*)) {
    HashMap* hm =  (HashMap*)(calloc(1, sizeof(HashMap)));
    hm->size = MIN_HASH;
    hm->values = (char**) (malloc(sizeof(char*) * MIN_HASH));
    hm->keys = (char**) (malloc(MIN_HASH * sizeof(char*)));
    for (int i = 0 ; i < MIN_HASH; i ++) {
        hm->keys[i] = (char*)INT_MIN;
    }
    hm->compare = compare;
    hm->hash = hash;
}

/**
 * infinite loop till gets the next prime number
 */
ulli next_prime(ulli num) {
    while (1) {
        num++;
        ulli max_lim  = num/ 2;
        short is_prime = 0;
        for (int i =2; i < max_lim; i ++ ) {
            if (num % i == 0) {
                is_prime = 1;
                break;
            }
        }
        if (is_prime == 0) {
            break;
        }
    }
    return num;
}


int insert(HashMap* hm, char* key, char* value) {
    if (key == (char*)INT_MIN && value == (char*)INT_MIN) {
        return -1;
    }
    if (hm->len == hm->size / 2) {
        int prev_size = hm->size;
        hm->size = next_prime(hm->size * 2);
        reallocate_hash(hm, prev_size);
    }

    ulli hash_val = hm->hash(key);
    ulli pos = hash_val % hm->size;
    // ulli size= (ulli)pow(2,ceil(log2(hm->size)));
    ulli size= hm->size;
    int i =1;
    while ((hm->keys[pos] != (char*)INT_MIN) && (hm->keys[pos] != NULL)) {
        pos = (hash_val + (i + (i * i)) ) % size;
        i ++ ;
    }
    // printf("Strdup %d\n", pos);
    hm->keys[pos] = (key); // occupied
    hm->values[pos] = (value);
    hm->len ++;

    return 0;
}

/**
 * Reallocates the HashMap
 */
int reallocate_hash(HashMap* hm, int prev_size) {
    char** prev_values = hm->values;
    char** prev_keys = hm->keys;
    hm->len = 0;
    hm->values = (char**)malloc(sizeof(char*) * hm->size);
    hm->keys = (char**)malloc(sizeof(char*) * hm->size);
    for (int i = 0 ; i < MIN_HASH; i ++) {
        hm->keys[i] = (char*)INT_MIN;
    }
    for (int i =0; i < prev_size; i ++) {
        char* key = prev_keys[i];
        if (key != (char*)(INT_MIN) && key != NULL) {
            char* value = prev_values[i];
            insert(hm, key, value);
        }
    }
    free(prev_keys);
    free(prev_values);
}

/**
 * Uses property:
 * If m is prime and the probing sequence is well-formed (e.g., c1=c2=1), all slots will eventually be visited before a cycle occurs.
 * 
 */
int get_actual_pos (HashMap* hm, char* key) {
    ulli hash_val = hm->hash(key);
    int pos = hash_val % hm->size;
    // ulli size= (ulli)pow(2,ceil(log2(hm->size)));
    ulli size= hm->size;
    ulli i =1;
    short res = 0;
    // printf("pos %ld %p\n", pos, hm->keys[pos]);
    while (((res = hm->compare(hm->keys[pos], key) ) != 0 ) && (i <= hm->size)) {
        pos = (hash_val + (i + (i * i)) ) % size;
        // printf("pos %ld\n", pos);
        i ++ ;
    }
    if (i > hm->size) {
        return INT_MIN;
    }
    return pos;
}

/**
 * If key not present then insert in a new entry 
 * else updates the previous entry
 */
int upsert(HashMap* hm, char* key, char* value) {
    // printf("__\n");
    // printf("PP %s: %p, %d\n",key, key, hm->len);
    int pos = get_actual_pos(hm, key);
    if (pos == INT_MIN) {
        return insert(hm,key,value);
    } else {
        hm->values[pos] = value;
        return 0;
    }
}


char* get(HashMap* hm, char* key) {
    // printf("PP %s: %d, %d\n",key, hm->len);
    int pos = get_actual_pos(hm, key);
    if (pos == INT_MIN) {
        return (char*)pos;
    }
    return hm->values[pos];
}

int remove_hm(HashMap* hm, char* key) {
    int pos = get_actual_pos(hm, key);
    if (pos == INT_MIN) {
        return -1;
    } 
    hm->keys[pos] = (char *)INT_MIN;
    hm->values[pos] = 0;
    hm->len --;

    if (hm->len < (hm->size / 2)) {
        int prev_size = hm->size;
        hm->size = next_prime(hm->size / 2);
        reallocate_hash(hm,prev_size);
    }
    return 0; // successfull
}

char** keys(HashMap* hm) {
    char** keys = (char**)malloc(sizeof(char*) * hm->len);
    // ulli size= (ulli)pow(2,ceil(log2(hm->size)));
    ulli size= hm->size;
    int pos = 0;
    for (int i =0; i < size; i ++) {
        // printf("%p %d\n", hm->keys[i], pos);
        char* k = hm->keys[i];
        if (k != (char*)(INT_MIN) && k != NULL) {
            keys[pos ++] = hm->keys[i];
        }
    }
    return keys;
}



int free_hashmap(HashMap* hm) {
    for (int i =0; i < hm->size;i ++) {
        char* k = hm->keys[i];
        if (k != (char*)(INT_MIN) && k != NULL) {
            free(k);
            // free(hm->values[i]);
        }
    }
    free(hm->keys);
    free(hm->values);
    free(hm);
}