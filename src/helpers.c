#include <raylib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Copy and adapt as needed
typedef struct {
    int  length;
    int  capacity;
    int *elements;
} IntArray;

IntArray NewIntArray(int capacity) {
    if (capacity < 1) {
        return (IntArray){0};
    }

    return (IntArray){
        0,
        capacity,
        MemAlloc(capacity * sizeof(int)),
    };
}

int IntArray_Get(IntArray *array, int i) {
    if (i < 0 || i >= array->length) {
        return 0;
    }

    return array->elements[i];
}

int IntArray_Push(IntArray *array, int element) {
    if (array->length >= array->capacity) {
        return 0;
    }

    array->elements[array->length] = element;
    array->length++;

    return 1;
}

int IntArray_Pop(IntArray *array) {
    if (array->length <= 0) {
        return 0;
    }

    array->length--;
    return array->elements[array->length + 1];
}

int IntArray_SwapDelete(IntArray *array, int index) {
    if (index < 0 || index >= array->length) {
        return 0;
    }

    int tmp                        = array->elements[index];
    array->elements[index]         = array->elements[array->length];
    array->elements[array->length] = tmp;
    array->length--;

    return 1;
}

typedef struct {
    char  *chars;
    size_t length;
} String;

void PrintChars(String string) {
    for (int i = 0; i < string.length; i++) {
        printf("%c", string.chars[i]);
    }
}
