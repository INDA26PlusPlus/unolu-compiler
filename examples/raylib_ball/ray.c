// Helper functions
#include "raylib.h"

void ClearBackgroundWhite() {
   ClearBackground(RAYWHITE);
 }

void DrawCircleRed(int centerX, int centerY, int radius) {
  DrawCircle(centerX, centerY, (float)radius, RED);
}; 

void InitWindowDefName(int width, int height) {
    InitWindow(width, height, "window");
}
