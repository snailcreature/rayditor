#include <math.h>
#include <raylib.h>
#include <sys/types.h>

int hashVector3(Vector3 vec) {
    int x = roundf(vec.x);
    int y = roundf(vec.y);
    int z = roundf(vec.z);
    // 17x + 31y + 127z
    return (x << 4) + x + (y << 5) - y + (z << 7) - z;
}

typedef struct {
    unsigned int length;
    unsigned int capacity;
    int         *elements;
} GrowableArray_Int;

GrowableArray_Int newGrowableArray_Int() {
    return (GrowableArray_Int){
        .length   = 0,
        .capacity = 8,
        .elements = (int *)MemAlloc(8 * sizeof(int)),
    };
}

void pushGrowableArray_Int(GrowableArray_Int *array, int element) {
    if (array->length == array->capacity) {
        array->capacity *= 2;
        array->elements = (int *)MemRealloc(array->elements, array->capacity * sizeof(int));
    }
    array->elements[array->length++] = element;
}

int popGrowableArray_Int(GrowableArray_Int *array) {
    if (array->length == 0) {
        return 0;
    }
    array->length--;
    return array->elements[array->length + 1];
}

void swapDeleteGrowableArray_Int(GrowableArray_Int *array, unsigned int index) {
    if (index >= array->length) {
        return;
    }
    int tmp = array->elements[array->length--];
    array->elements[index] = tmp;
}

int getGrowableArray_Int(GrowableArray_Int *array, unsigned int index) {
    if (index >= array->length) {
        return 0;
    }

    return array->elements[index];
}

void setGrowableArray_Int(GrowableArray_Int *array, unsigned int index, int value) {
    if (index >= array->length) {
        return;
    }

    array->elements[index] = value;
}

void freeGrowableArray_Int(GrowableArray_Int *array) {
    MemFree(array->elements);
    array->capacity = 0;
    array->length = 0;
}

typedef struct {
    unsigned int bucket_count;
    unsigned int live_buckets;
    int *sparse;
    GrowableArray_Int *buckets;
} SpacialHash;

SpacialHash newSpacialHash(unsigned int bucket_count) {
    if (bucket_count <= 1) {
        bucket_count++;
    }

    SpacialHash hash = {
        .bucket_count = bucket_count,
        .live_buckets = 0,
        .sparse = (int *)MemAlloc(bucket_count * sizeof(int)),
        .buckets = (GrowableArray_Int *)MemAlloc(bucket_count * sizeof(GrowableArray_Int)),
    };

    for (int i = 0; i < hash.bucket_count; i++) {
        hash.sparse[i] = -1;
    }

    return hash;
}

void SpacialHash_insert(SpacialHash *hash, unsigned int index, int value) {
    if (index >= hash->bucket_count) {
        return;
    }

    if (hash->sparse[index] == -1) {
        hash->buckets[hash->live_buckets] = newGrowableArray_Int();
        hash->sparse[index] = hash->live_buckets;
        hash->live_buckets++;
    }

    pushGrowableArray_Int(&hash->buckets[hash->sparse[index]], value);
}

void SpacialHash_hashInsert(SpacialHash *hash, Vector3 vec, int value) {
    unsigned int index = hashVector3(vec) % hash->bucket_count;
    SpacialHash_insert(hash, index, value);
}

GrowableArray_Int SpacialHash_get(SpacialHash *hash, unsigned int index) {
    if (hash->sparse[index] == -1) {
        return (GrowableArray_Int){ 0 };
    }

    return hash->buckets[hash->sparse[index]];
}

GrowableArray_Int SpacialHash_hashGet(SpacialHash *hash, Vector3 vec) {
    unsigned int index = hashVector3(vec) % hash->bucket_count;
    return SpacialHash_get(hash, index);
}

void freeSpacialHash(SpacialHash *hash) {
    for (int i = 0; i < hash->bucket_count; i++) {
        freeGrowableArray_Int(&hash->buckets[i]);
    }

    MemFree(hash->buckets);
    MemFree(hash->sparse);
    hash->bucket_count = 0;
    hash->live_buckets = 0;
}
