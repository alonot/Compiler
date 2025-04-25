/**
 * implementation of hashmap using quadratic hashing
 * to support symbol table
 */

#include "../include/includes.h"
#define MIN_HASH 5
/**
 * Converts string to int for hashing it
 * Gives different weights to different positions hence enforces ordering.
 */
lli hash_string(lli str_val) {
    lli val = 0;
    char* str = (char*) (str_val);
    int len = strlen(str);
    for (int i =0; i < len; i ++) {
        val += ((i + 1) * str[i]) % LLONG_MAX;
    }
    return val;
}

short compare_string(lli s1_val, lli s2_val) {
    char* s1 = (char*) (s1_val);
    char* s2 = (char*) (s2_val);
    if (s1_val == LLONG_MIN || s2_val == LLONG_MIN || s1 == NULL || s2 == NULL) {
        return -1;
    }
    // fprintf(debug,"%s %s\n", s1, s2);
    return (short)strcmp(s1, s2);
}

lli hash_int(lli num) {
    return (int)num;
}

HashMap* init_hash_map(lli (*hash)(lli), short (*compare)(lli , lli)) {
    HashMap* hm =  (HashMap*)(calloc(1, sizeof(HashMap)));
    hm->size = MIN_HASH;
    hm->values = (lli*) (malloc(sizeof(lli) * MIN_HASH));
    hm->keys = (lli*) (malloc(MIN_HASH * sizeof(lli)));
    for (int i = 0 ; i < MIN_HASH; i ++) {
        hm->keys[i] = LLONG_MIN;
    }
    hm->compare = compare;
    hm->hash = hash;
}

/**
 * infinite loop till gets the next prime number
 */
lli next_prime(lli num) {
    while (1) {
        num++;
        lli max_lim  = num/ 2;
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


lli insert(HashMap* hm, lli key, lli value) {
    if (key == (lli)LLONG_MIN || value == (lli)LLONG_MIN) {
        return LLONG_MIN;
    }
    if (hm->len == hm->size / 2) {
        int prev_size = hm->size;
        hm->size = next_prime(hm->size * 2);
        reallocate_hash(hm, prev_size);
        // fprintf(debug,"Here %lld\n", LLONG_MIN);

    }

    // for (int i  =0; i < hm->size; i ++) {
    //     fprintf(debug,"%ld %d\n", hm->keys[i] ,hm->keys[i] == LLONG_MIN);
    // }

    lli hash_val = hm->hash(key);
    lli pos = hash_val % hm->size;
    // lli size= (lli)pow(2,ceil(log2(hm->size)));
    lli size= hm->size;
    int i =1;
    // fprintf(debug,"%d\n", size);
    while ((hm->keys[pos] != (lli)LLONG_MIN) ) {
        pos = (hash_val + (i + (i * i)) ) % size;
        i ++ ;
    }
    // fprintf(debug,"Strdup %d\n", pos);
    hm->keys[pos] = (key); // occupied
    hm->values[pos] = (value);
    hm->len ++;

    return 0;
}

/**
 * Reallocates the HashMap
 */
int reallocate_hash(HashMap* hm, int prev_size) {
    lli* prev_values = hm->values;
    lli* prev_keys = hm->keys;
    hm->len = 0;
    hm->values = (lli*)malloc(sizeof(lli) * hm->size);
    hm->keys = (lli*)malloc(sizeof(lli) * hm->size);
    for (int i = 0 ; i < hm->size; i ++) {
        hm->keys[i] = (lli)LLONG_MIN;
    }
    for (int i =0; i < prev_size; i ++) {
        lli key = prev_keys[i];
        if (key != (lli)(LLONG_MIN) && (char*)key != NULL) {
            lli value = prev_values[i];
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
lli get_actual_pos (HashMap* hm, lli key) {
    lli hash_val = hm->hash(key);
    int pos = hash_val % hm->size;
    // lli size= (lli)pow(2,ceil(log2(hm->size)));
    lli size= hm->size;
    lli i =1;
    short res = 0;
    // fprintf(debug,"pos %ld %p %ld\n", pos, hm->keys[pos], size);
    while (((res = hm->compare(hm->keys[pos], key) ) != 0 ) && (i <= hm->size)) {
        pos = (hash_val + (i + (i * i)) ) % size;
        // fprintf(debug,"pos %ld\n", pos);
        i ++ ;
    }
    if (i > hm->size) {
        return LLONG_MIN;
    }
    return pos;
}

/**
 * If key not present then insert in a new entry 
 * else updates the previous entry
 */
lli upsert(HashMap* hm, lli key, lli value) {
    // fprintf(debug,"__\n");
    lli pos = get_actual_pos(hm, key);
    if (pos == LLONG_MIN) {
        // fprintf(debug,"PP %s: %p, %d\n",key, key, hm->len);
        return insert(hm,key,value);
    } else {
        hm->values[pos] = value;
        return 0;
    }
}

/**
 * If key not present then returns INT_MIN 
 * else updates the previous entry
 */
lli update(HashMap* hm, lli key, lli value) {
    // fprintf(debug,"__\n");
    lli pos = get_actual_pos(hm, key);
    if (pos == LLONG_MIN) {
        return LLONG_MIN;
    } else {
        hm->values[pos] = value;
        return 0;
    }
}


lli get(HashMap* hm, lli key) {
    // fprintf(debug,"PP %s: %d, %d\n",key, hm->len);
    lli pos = get_actual_pos(hm, key);
    if (pos == LLONG_MIN) {
        return (lli)pos;
    }
    return hm->values[pos];
}

int remove_hm(HashMap* hm, lli key) {
    lli pos = get_actual_pos(hm, key);
    if (pos == LLONG_MIN) {
        return -1;
    } 
    hm->keys[pos] = LLONG_MIN;
    hm->values[pos] = 0;
    hm->len --;

    if (hm->len < (hm->size / 2)) {
        int prev_size = hm->size;
        hm->size = next_prime(hm->size / 2);
        reallocate_hash(hm,prev_size);
    }
    return 0; // successfull
}

lli* keys(HashMap* hm) {
    lli* keys = (lli*)malloc(sizeof(lli) * hm->len);
    // lli size= (lli)pow(2,ceil(log2(hm->size)));
    lli size= hm->size;
    int pos = 0;
    for (int i =0; i < size; i ++) {
        // fprintf(debug,"%p %d\n", hm->keys[i], pos);
        lli k = hm->keys[i];
        if (k != (lli)(LLONG_MIN) && (char*)k != NULL) {
            keys[pos ++] = hm->keys[i];
        }
    }
    return keys;
}



int free_hashmap(HashMap* hm) {
    // for (int i =0; i < hm->size;i ++) {
    //     lli k = hm->keys[i];
    //     if (k != (lli)(LLONG_MIN) && k != NULL) {
    //         free(k);
    //         // free(hm->values[i]);
    //     }
    // }
    free(hm->keys);
    free(hm->values);
    free(hm);
}