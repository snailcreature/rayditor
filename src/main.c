#include <float.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "mesh.c"
#include "spacial_hash.c"

enum editor_mode {
    VIEW,
    TERRAIN,
    PLACE,
};

char *StringifyEditorMode(enum editor_mode *mode) {
    switch (*mode) {
    case VIEW:
        return "View";
    case TERRAIN:
        return "Terrain";
    case PLACE:
        return "Place";
    default:
        return "Default";
    }
}

int screenWidth  = 960;
int screenHeight = 720;

int main(int argc, char *argv[]) {
    InitWindow(screenWidth, screenHeight, "rayditor");

    enum editor_mode mode    = VIEW;
    int              intMode = (int)mode;

    Camera camera     = {0};
    camera.position   = (Vector3){10.0f, 10.0f, 10.0f};
    camera.target     = (Vector3){0.0f};
    camera.up         = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy       = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Vector3     cubePos  = {0.0f, 0.5f, 0.0f};
    Vector3     cubeSize = {1.0f, 1.0f, 1.0f};
    BoundingBox cubeBB   = {
        (Vector3){
                  cubePos.x - cubeSize.x / 2,
                  cubePos.y - cubeSize.y / 2,
                  cubePos.z - cubeSize.z / 2,
                  },
        (Vector3){
                  cubePos.x + cubeSize.x / 2,
                  cubePos.y + cubeSize.y / 2,
                  cubePos.z + cubeSize.z / 2,
                  }
    };
    RayCollision cubeHitInfo;
    Color        cursorColour;

    Ray          ray    = {0};
    RayCollision rayCol = {0};

    Terrain     terrain      = NewTerrain();
    Mesh        terrainMesh  = GenMeshTerrain(&terrain);
    Model       terrainModel = LoadModelFromMesh(terrainMesh);
    BoundingBox terrainBB    = GetMeshBoundingBox(terrainMesh);

    SpacialHash spacialHash = newSpacialHash(128);
    ControlPointArray controlPoints = NewControlPointArray(terrain.vertices.length);
    for (int i = 0; i < terrain.vertices.length; i++) {
        Vector3 position = terrain.vertices.elements[i].position;
        ControlPoint controlPoint = {
            .position = position,
            .bounding_box = {
                Vector3Subtract(position, (Vector3){ 0.05, 0.05, 0.05 }),
                Vector3Add(position, (Vector3){ 0.05, 0.05, 0.05 }),
            },
            .vertex = i,
        };
        ControlPointArray_Push(&controlPoints, controlPoint);
        SpacialHash_hashInsert(&spacialHash, position, i);
    }

    SetTargetFPS(60);

    char statusText[100];

    int nearestControlPoint = -1;

    while (!WindowShouldClose()) {
        // UPDATE
        if (IsCursorHidden())
            UpdateCamera(&camera, CAMERA_FREE);

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (!IsCursorHidden())
                DisableCursor();
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
            if (IsCursorHidden())
                EnableCursor();
        }

        if (IsWindowResized()) {
            screenWidth  = GetScreenWidth();
            screenHeight = GetScreenHeight();
        }

        if (IsKeyPressed(KEY_ONE)) {
            mode    = VIEW;
            intMode = (int)mode;
        } else if (IsKeyPressed(KEY_TWO)) {
            mode    = TERRAIN;
            intMode = (int)mode;
        } else if (IsKeyPressed(KEY_THREE)) {
            mode    = PLACE;
            intMode = (int)mode;
        }

        rayCol          = (RayCollision){0};
        rayCol.distance = FLT_MAX;
        rayCol.hit      = false;
        cursorColour    = WHITE;

        ray = GetScreenToWorldRay(GetMousePosition(), camera);

        cubeHitInfo = GetRayCollisionBox(ray, terrainBB);

        if (cubeHitInfo.hit && cubeHitInfo.distance < rayCol.distance) {
            rayCol       = cubeHitInfo;
            cursorColour = GREEN;

            if (mode == TERRAIN) {
                GrowableArray_Int nearby = SpacialHash_hashGet(&spacialHash, rayCol.point);
                RayCollision nearestControl;
                for (int i = 0; i < nearby.length; i++) {
                    ControlPoint controlPoint = controlPoints.elements[getGrowableArray_Int(&nearby, i)];
                    nearestControl = GetRayCollisionBox(ray, controlPoint.bounding_box);
                    if (nearestControl.hit) {
                        nearestControlPoint = getGrowableArray_Int(&nearby, i);
                    }
                }
            }
        }

        // DRAW
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        DrawModel(terrainModel,
                  (Vector3){0, 0, 0},
                  1.0f,
                  mode == TERRAIN ? DARKGRAY : LIGHTGRAY);

        DrawGrid(128, 0.5f);

        if (mode == TERRAIN) {
            Vertex vertex;
            for (int i = 0; i < terrain.vertices.length; i++) {
                vertex = VertexArray_Get(&terrain.vertices, i);
                DrawCube(vertex.position, 0.1f, 0.1f, 0.1f, nearestControlPoint == i ? ORANGE : BLUE);
            }

        }

        if (rayCol.hit && !IsCursorHidden()) {
            DrawCube(rayCol.point, 0.05f, 0.05f, 0.05f, cursorColour);
            DrawCubeWires(rayCol.point, 0.1f, 0.1f, 0.1f, cursorColour);

            Vector3 normalEnd;
            normalEnd.x = rayCol.point.x + rayCol.normal.x;
            normalEnd.y = rayCol.point.y + rayCol.normal.y;
            normalEnd.z = rayCol.point.z + rayCol.normal.z;

            DrawLine3D(rayCol.point, normalEnd, cursorColour);
        }

        EndMode3D();
        if (nearestControlPoint != -1) {
            DrawText(TextFormat("Nearest Point: %i", nearestControlPoint), screenWidth/2, 20, 20, BLACK);
            nearestControlPoint = -1;
        }

        if ((GuiComboBox((Rectangle){screenWidth - 100.0f, 10.0f, 90.0f, 20.0f},
                         "View\nTerrain\nPlace",
                         &intMode) == 0)) {
            mode = (enum editor_mode)intMode;
        }

        sprintf(statusText,
                "[%7s] %ifps | Mouse x: %i y: %i | Camera x: %0.2f, y: %0.2f, "
                "z: %0.2f",
                StringifyEditorMode(&mode),
                GetFPS(),
                GetMouseX(),
                GetMouseY(),
                camera.position.x,
                camera.position.y,
                camera.position.z);
        GuiStatusBar((Rectangle){0, screenHeight - 20.0f, screenWidth, 20.0f},
                     TextFormat("%s", statusText));

        EndDrawing();
    }

    UnloadModel(terrainModel);
    freeSpacialHash(&spacialHash);

    CloseWindow();
    return 0;
}
