#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#define GRID_COUNT     128.0f
#define VERTEX_COUNT   ((GRID_COUNT + 1) * (GRID_COUNT + 1))
#define TRIANGLE_COUNT ((GRID_COUNT * GRID_COUNT) * 2)

const float spacing    = 64.0 / GRID_COUNT;
const float half_count = 64.0 * spacing; // GRID_COUNT / 2;

typedef struct {
    Vector3 position;
    Vector2 texcoord;
} Vertex;

void PrintVertex(Vertex *vertex) {
    printf("pos: %.1f %.1f %.1f\ttex: %.1f %.1f\n",
           vertex->position.x,
           vertex->position.y,
           vertex->position.z,
           vertex->texcoord.x,
           vertex->texcoord.y);
}

typedef struct {
    int     capacity;
    int     length;
    Vertex *elements;
} VertexArray;

VertexArray NewVertexArray(int capacity) {
    return (VertexArray){
        capacity,
        0,
        (Vertex *)MemAlloc(capacity * sizeof(Vertex)),
    };
}

int VertexArray_Set(VertexArray *array, int index, Vertex vertex) {
    if (index < 0 || index >= array->length) {
        return 0;
    }

    array->elements[index] = vertex;

    return 1;
}

int VertexArray_Push(VertexArray *array, Vertex vertex) {
    if (array->length == array->capacity) {
        return 0;
    }

    array->elements[array->length]  = vertex;
    array->length                  += 1;

    return 1;
}

Vertex VertexArray_Get(VertexArray *array, int index) {
    if (index < 0 || index >= array->length) {
        return (Vertex){0};
    }

    return array->elements[index];
}

typedef struct {
    int vertex1;
    int vertex2;
    int vertex3;
    int visible;
} Triangle;

Triangle NewTriangle(int vertex1, int vertex2, int vertex3) {
    return (Triangle){
        vertex1,
        vertex2,
        vertex3,
        1,
    };
}

void PrintTriangle(Triangle *triangle) {
    printf("1: %d\t2: %d\t3: %d\tv?: %d\n",
           triangle->vertex1,
           triangle->vertex2,
           triangle->vertex3,
           triangle->visible);
}

typedef struct {
    int       length;
    int       capacity;
    Triangle *elements;
} TriangleArray;

TriangleArray NewTriangleArray(int capacity) {
    return (TriangleArray){
        0,
        capacity,
        (Triangle *)MemAlloc(capacity * sizeof(Triangle)),
    };
}

int TriangleArray_Set(TriangleArray *array, int index, Triangle vertex) {
    if (index < 0 || index >= array->length) {
        return 0;
    }

    array->elements[index] = vertex;

    return 1;
}

int TriangleArray_Push(TriangleArray *array, Triangle vertex) {
    if (array->length == array->capacity) {
        return 0;
    }

    array->elements[array->length]  = vertex;
    array->length                  += 1;

    return 1;
}

Triangle TriangleArray_Get(TriangleArray *array, int index) {
    if (index < 0 || index >= array->length) {
        printf("Miss\n");
        return (Triangle){0};
    }

    return array->elements[index];
}

typedef struct {
    VertexArray   vertices;
    TriangleArray triangles;
} Terrain;

Terrain NewTerrain() {
    VertexArray   vertices  = NewVertexArray(VERTEX_COUNT);
    TriangleArray triangles = NewTriangleArray(TRIANGLE_COUNT);

    for (float z = -half_count; z <= half_count; z += spacing) {
        for (float x = -half_count; x <= half_count; x += spacing) {
            Vertex vertex = {
                .position = (Vector3){x, 0.0f, z},
                .texcoord = (Vector2){0},
            };
            VertexArray_Push(&vertices, vertex);
        }
    }

    for (int y = 0; y < GRID_COUNT; y++) {
        int idxY0 = (GRID_COUNT + 1) * y;
        int idxY1 = (GRID_COUNT + 1) * (y + 1);
        for (int x = 0; x < GRID_COUNT; x++) {
            int idxA = idxY0 + x;
            int idxB = idxY0 + x + 1;
            int idxC = idxY1 + x + 1;
            int idxD = idxY1 + x;

            // GLTF uses counter-clockwise winding
            Triangle triADB = NewTriangle(idxA, idxD, idxB);
            Triangle triBDC = NewTriangle(idxB, idxD, idxC);

            TriangleArray_Push(&triangles, triADB);
            TriangleArray_Push(&triangles, triBDC);
        }
    }

    return (Terrain){
        vertices,
        triangles,
    };
}

Vertex Terrain_GetVertex(Terrain *terrain, int x, int y) {
    if (x < 0 || x > GRID_COUNT) {
        printf("Wrong x\n");
        return (Vertex){0};
    }

    if (y < 0 || y > GRID_COUNT) {
        printf("Wrong y\n");
        return (Vertex){0};
    }

    int index = ((GRID_COUNT + 1) * y) + x;

    return VertexArray_Get(&terrain->vertices, index);
}

static Mesh GenMeshTerrain(Terrain *terrain) {
    Mesh mesh = {0};

    mesh.triangleCount = TRIANGLE_COUNT;
    mesh.vertexCount   = mesh.triangleCount * 3;
    mesh.vertices  = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float *)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
    mesh.normals   = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));

    for (int i = 0; i < terrain->triangles.length; i++) {
        int      arrInd3  = i * 9; // 3 vertices, 3 coords each, 9 indices
        int      arrInd2  = i * 6; // 3 vertices, 2 coords each, 6 indices
        Triangle triangle = TriangleArray_Get(&terrain->triangles, i);
        if (!triangle.visible) {
            continue;
        }

        Vertex v1 = VertexArray_Get(&terrain->vertices, triangle.vertex1);
        Vertex v2 = VertexArray_Get(&terrain->vertices, triangle.vertex2);
        Vertex v3 = VertexArray_Get(&terrain->vertices, triangle.vertex3);

        Vector3 edge1  = Vector3Subtract(v2.position, v1.position);
        Vector3 edge2  = Vector3Subtract(v3.position, v1.position);
        Vector3 normal = Vector3Normalize(Vector3CrossProduct(edge1, edge2));

        mesh.vertices[arrInd3]      = v1.position.x;
        mesh.vertices[arrInd3 + 1]  = v1.position.y;
        mesh.vertices[arrInd3 + 2]  = v1.position.z;
        mesh.normals[arrInd3]       = normal.x;
        mesh.normals[arrInd3 + 1]   = normal.y;
        mesh.normals[arrInd3 + 2]   = normal.z;
        mesh.texcoords[arrInd2]     = v1.texcoord.x;
        mesh.texcoords[arrInd2 + 1] = v1.texcoord.y;

        mesh.vertices[arrInd3 + 3]  = v2.position.x;
        mesh.vertices[arrInd3 + 4]  = v2.position.y;
        mesh.vertices[arrInd3 + 5]  = v2.position.z;
        mesh.normals[arrInd3 + 3]   = normal.x;
        mesh.normals[arrInd3 + 4]   = normal.y;
        mesh.normals[arrInd3 + 5]   = normal.z;
        mesh.texcoords[arrInd2 + 2] = v2.texcoord.x;
        mesh.texcoords[arrInd2 + 3] = v2.texcoord.y;

        mesh.vertices[arrInd3 + 6]  = v3.position.x;
        mesh.vertices[arrInd3 + 7]  = v3.position.y;
        mesh.vertices[arrInd3 + 8]  = v3.position.z;
        mesh.normals[arrInd3 + 6]   = normal.x;
        mesh.normals[arrInd3 + 7]   = normal.y;
        mesh.normals[arrInd3 + 8]   = normal.z;
        mesh.texcoords[arrInd2 + 4] = v3.texcoord.x;
        mesh.texcoords[arrInd2 + 5] = v3.texcoord.y;
    }

    UploadMesh(&mesh, false);

    return mesh;
}

typedef struct {
    Vector3     position;
    BoundingBox bounding_box;
    int         vertex;
} ControlPoint;

typedef struct {
    int           capacity;
    int           length;
    ControlPoint *elements;
} ControlPointArray;

ControlPointArray NewControlPointArray(int capacity) {
    if (capacity < 1) {
        return (ControlPointArray){0};
    }

    return (ControlPointArray){
        capacity,
        0,
        (ControlPoint *)MemAlloc(capacity * sizeof(ControlPoint)),
    };
}

ControlPoint ControlPointArray_Get(ControlPointArray *array, int index) {
    if (index < 0 || index >= array->length) {
        return (ControlPoint){0};
    }

    return array->elements[index];
}

int ControlPointArray_Push(ControlPointArray *array, ControlPoint point) {
    if (array->length >= array->capacity) {
        return 0;
    }

    array->elements[array->length] = point;
    array->length++;

    return 1;
}
