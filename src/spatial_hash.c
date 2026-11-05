#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <sys/types.h>

#include "btree.c"

const float   SPATIAL_HASH_UNIT   = 2.0;
const Vector3 SPATIAL_HASH_OFFSET = {
    0.5,
    0.5,
    0.5,
};
const float SPATIAL_HASH_UNIT_MAG =
    3.0 * (SPATIAL_HASH_UNIT * SPATIAL_HASH_UNIT);

int hashVector3(Vector3 vec) {
    int x = roundf((vec.x / SPATIAL_HASH_UNIT) + SPATIAL_HASH_OFFSET.x);
    int y = roundf((vec.y / SPATIAL_HASH_UNIT) + SPATIAL_HASH_OFFSET.y);
    int z = roundf((vec.z / SPATIAL_HASH_UNIT) + SPATIAL_HASH_OFFSET.z);

    // 17x + 31y + 127z
    return (x << 4) + x + (y << 5) - y + (z << 7) - z;
}

void translateBB(BoundingBox *bb, Vector3 translation) {
    bb->min.x += translation.x;
    bb->max.x += translation.x;
    bb->min.y += translation.y;
    bb->max.y += translation.y;
    bb->min.z += translation.z;
    bb->max.z += translation.z;
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
        array->elements =
            (int *)MemRealloc(array->elements, array->capacity * sizeof(int));
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
    int tmp                = array->elements[array->length--];
    array->elements[index] = tmp;
}

int getGrowableArray_Int(GrowableArray_Int *array, unsigned int index) {
    if (index >= array->length) {
        return 0;
    }

    return array->elements[index];
}

void setGrowableArray_Int(GrowableArray_Int *array,
                          unsigned int       index,
                          int                value) {
    if (index >= array->length) {
        return;
    }

    array->elements[index] = value;
}

void freeGrowableArray_Int(GrowableArray_Int *array) {
    MemFree(array->elements);
    array->capacity = 0;
    array->length   = 0;
}

typedef struct {
    unsigned int bucket_count;
    unsigned int live_buckets;
    int         *sparse;
    IntBTree    *buckets;
} SpatialHash;

SpatialHash newSpatialHash(unsigned int bucket_count) {
    if (bucket_count <= 1) {
        bucket_count++;
    }

    SpatialHash hash = {
        .bucket_count = bucket_count,
        .live_buckets = 0,
        .sparse       = (int *)MemAlloc(bucket_count * sizeof(int)),
        .buckets      = (IntBTree *)MemAlloc(bucket_count * sizeof(IntBTree)),
    };

    for (int i = 0; i < hash.bucket_count; i++) {
        hash.sparse[i] = -1;
    }

    return hash;
}

void SpatialHash_insert(SpatialHash *hash, unsigned int index, int value) {
    if (index >= hash->bucket_count) {
        return;
    }

    if (hash->sparse[index] == -1) {
        hash->buckets[hash->live_buckets] = newIntBTree();
        hash->sparse[index]               = hash->live_buckets;
        hash->live_buckets++;
    }

    insertIntBTree(&hash->buckets[hash->sparse[index]], value);
}

void SpatialHash_hashInsert(SpatialHash *hash, Vector3 vec, int value) {
    unsigned int index = hashVector3(vec) % hash->bucket_count;
    SpatialHash_insert(hash, index, value);
}

void SpatialHash_insertBB(SpatialHash *hash, BoundingBox box, int value) {
    // If the box is inside a single Spatial Unit,
    // insert it.
    int minHash = hashVector3(box.min);
    int maxHash = hashVector3(box.max);
    if (minHash == maxHash) {
        SpatialHash_insert(hash, minHash % hash->bucket_count, value);
        return;
    }

    // If the box spans Spatial Units,
    // but would fit inside one,
    // insert each of its corners
    float bb_mag = Vector3DistanceSqr(box.min, box.max);
    if (bb_mag < SPATIAL_HASH_UNIT_MAG) {
        float x_diff = box.max.x - box.min.x;
        float y_diff = box.max.y - box.min.y;
        float z_diff = box.max.z - box.min.z;
        SpatialHash_hashInsert(hash, box.min, value);
        if (x_diff >= SPATIAL_HASH_UNIT) {
            if (y_diff >= SPATIAL_HASH_UNIT) {
                SpatialHash_hashInsert(
                    hash, (Vector3){box.max.x, box.max.y, box.min.z}, value);
            } else if (z_diff >= SPATIAL_HASH_UNIT) {
                SpatialHash_hashInsert(
                    hash, (Vector3){box.max.x, box.min.y, box.max.z}, value);
            } else {
                SpatialHash_hashInsert(
                    hash, (Vector3){box.max.x, box.min.y, box.min.z}, value);
            }
        } else {
            if (y_diff >= SPATIAL_HASH_UNIT) {
                SpatialHash_hashInsert(
                    hash, (Vector3){box.min.x, box.max.y, box.min.z}, value);
                if (z_diff >= SPATIAL_HASH_UNIT) {
                    SpatialHash_hashInsert(
                        hash,
                        (Vector3){box.min.x, box.max.y, box.max.z},
                        value);
                }
            } else {
                SpatialHash_hashInsert(
                    hash, (Vector3){box.min.x, box.min.y, box.max.z}, value);
            }
        }
        SpatialHash_hashInsert(hash, box.max, value);
        return;
    }

    // Otherwise, subdivide the box into smaller bounding boxes and try again
    // for each
    Vector3 half_vec    = Vector3Scale(Vector3Subtract(box.max, box.min), 0.5f);
    BoundingBox half_bb = {box.min, Vector3Add(box.min, half_vec)};
    SpatialHash_insertBB(hash, half_bb, value);

    translateBB(&half_bb, (Vector3){half_vec.x, 0, 0});
    SpatialHash_insertBB(hash, half_bb, value);

    translateBB(&half_bb, (Vector3){0, half_vec.y, 0});
    SpatialHash_insertBB(hash, half_bb, value);

    translateBB(&half_bb, (Vector3){0, 0, half_vec.z});
    SpatialHash_insertBB(hash, half_bb, value);

    translateBB(&half_bb, (Vector3){-half_vec.x, 0, 0});
    SpatialHash_insertBB(hash, half_bb, value);

    translateBB(&half_bb, (Vector3){0, -half_vec.y, 0});
    SpatialHash_insertBB(hash, half_bb, value);

    translateBB(&half_bb, (Vector3){half_vec.x, 0, 0});
    SpatialHash_insertBB(hash, half_bb, value);

    translateBB(&half_bb, (Vector3){-half_vec.x, half_vec.y, -half_vec.z});
    SpatialHash_insertBB(hash, half_bb, value);
}

IntBTree *SpatialHash_get(SpatialHash *hash, unsigned int index) {
    if (hash->sparse[index] == -1) {
        return NULL;
    }

    return &hash->buckets[hash->sparse[index]];
}

IntBTree *SpatialHash_hashGet(SpatialHash *hash, Vector3 vec) {
    unsigned int index = hashVector3(vec) % hash->bucket_count;
    return SpatialHash_get(hash, index);
}

void freeSpatialHash(SpatialHash *hash) {
    for (int i = 0; i < hash->bucket_count; i++) {
        freeIntBTree(&hash->buckets[i]);
    }

    MemFree(hash->buckets);
    MemFree(hash->sparse);
    hash->bucket_count = 0;
    hash->live_buckets = 0;
}
