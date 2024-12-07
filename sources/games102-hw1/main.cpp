#include <Eigen/Dense>
#include <cmath>
#include <iostream>
#include <vector>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

using Eigen::MatrixXd;
using Eigen::VectorXd;

typedef enum {
  FITTING_ONE = 0,  // 插值 幂基函数线性组合
  FITTING_TWO,      // 插值 Gauss基函数线性组合
  FITTING_THREE,    // 逼近 固定幂基函数，最小二乘
  FITTING_FOUR      // 逼近 岭回归
} FittingType;

float guassBasisFunc(float x, float x0, float sigma) {
  return exp(-(pow(x - x0, 2)) / (2 * pow(sigma, 2)));
}

int main(void) {
  // Initialization
  // ---------------------------------------------------------------------------
  const int screenWidth = 800;
  const int screenHeight = 450;
  InitWindow(screenWidth, screenHeight, "GAMES102-hw1");

  Camera2D camera = {0};
  camera.zoom = 1.0f;

  int zoomMode = 0;  // 0-Mouse Wheel, 1-Mouse Move

  // config variables
  bool fittingTypeEditMode = false;
  int fittingTypeActive = FITTING_ONE;
  bool sampleRangeEditMode = false;
  bool clearPoints = false;
  bool calculateFitting = false;
  int sampleRange = 5;

  // Data
  std::vector<Vector2> points;
  std::vector<Vector2> samplePoints;

  SetTargetFPS(60);

  const float rightPos = GetScreenWidth() - 150;
  const Rectangle rightBar = {
      rightPos - 10, 0, GetScreenWidth() - (rightPos - 10), GetScreenHeight()};

  while (!WindowShouldClose()) {
    // Update
    // -------------------------------------------------------------------------
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      Vector2 delta = GetMouseDelta();
      delta = Vector2Scale(delta, -1.0f / camera.zoom);
      camera.target = Vector2Add(camera.target, delta);
    }

    if (zoomMode == 0) {
      float wheel = GetMouseWheelMove();
      if (wheel != 0) {
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
        camera.offset = GetMousePosition();
        camera.target = mouseWorldPos;

        float scaleFactor = 1.0f + (0.25f * fabsf(wheel));
        if (wheel < 0) scaleFactor = 1.0f / scaleFactor;
        camera.zoom = Clamp(camera.zoom * scaleFactor, 0.125f, 64.0f);
      }
    }

    // separate two part
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(GetMousePosition(), rightBar)) {
      Vector2 point = GetScreenToWorld2D(GetMousePosition(), camera);
      points.push_back(point);
    }

    if (calculateFitting) {
      if (points.size() > 1) {
        // Fitting ONE & TWO: A * alpha = B
        int n = points.size();
        VectorXd alpha(n);
        VectorXd Y(n);
        MatrixXd A(n, n);

        double xMin = points[0].x, xMax = points[0].x;

        const double sigma = 20;  // * important

        for (int i = 0; i < n; i++) {
          // cal sample range
          if (points[i].x > xMax) {
            xMax = points[i].x;
          }

          if (points[i].x < xMin) {
            xMin = points[i].x;
          }

          switch (fittingTypeActive) {
            case FITTING_ONE:
              for (int j = 0; j < n; j++) {
                A(i, j) = pow(points[i].x, j);  // 幂基函数的线性组合
              }
              break;
            case FITTING_TWO:
              A(i, 0) = 1;
              for (int j = 1; j < n; j++) {
                A(i, j) = guassBasisFunc(points[i].x, points[j].x,
                                         sigma);  // Gauss基函数的线性组合
              }
              break;
            default:
              break;
          }

          Y(i) = points[i].y;
        }

        // alpha = A.fullPivLu().solve(y);  // 全分解
        // alpha = A.householderQr().solve(y);  // QR分解
        // alpha = A.ldlt().solve(b);  // Cholesky分解（如果A是对称正定矩阵)
        // JacobiSVD<MatrixXd> svd(A, ComputeFullU | ComputeFullV); //
        // 使用Jacobi迭代法（对于大矩阵） alpha = svd.solve(b);
        alpha = A.fullPivLu().solve(Y);  // 全分解

        samplePoints.clear();
        int sampleNum = (int)ceil((xMax - xMin) / sampleRange);
        for (int i = 0; i < sampleNum; i++) {
          Vector2 samplePoint{0, 0};
          samplePoint.x = xMin + i * sampleRange;
          switch (fittingTypeActive) {
            case FITTING_ONE:
              for (int j = 0; j < n; j++) {
                samplePoint.y += alpha(j) * pow(samplePoint.x, j);
              }
              break;
            case FITTING_TWO:
              samplePoint.y = alpha(0);
              for (int j = 1; j < n; j++) {
                samplePoint.y += alpha(j) * guassBasisFunc(samplePoint.x,
                                                           points[j].x, sigma);
              }
            default:
              break;
          }
          samplePoints.push_back(samplePoint);
        }
      }

      calculateFitting = false;
    }

    if (clearPoints) {
      points.clear();
      samplePoints.clear();
      clearPoints = false;
    }

    // Draw
    // -------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode2D(camera);

    // Draw the 3d grid, rotated 90 degrees and centered around 0,0
    // just so we have something in the XY plane
    rlPushMatrix();
    rlTranslatef(0, 25 * 50, 0);
    rlRotatef(90, 1, 0, 0);
    DrawGrid(100, 50);
    rlPopMatrix();

    // Draw a reference circle
    DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2, 10, MAROON);

    // Draw points
    for (auto it = points.begin(); it != points.end(); it++) {
      DrawCircle((*it).x, (*it).y, 8, SKYBLUE);
    }

    if (samplePoints.size() > 1) {
      // Draw Fitting Line
      for (int i = 0; i < samplePoints.size() - 1; i++) {
        DrawLineEx(samplePoints[i], samplePoints[i + 1], 2, RED);
      }
    }

    EndMode2D();

    // mouse reference
    DrawCircleV(GetMousePosition(), 4, DARKGRAY);
    DrawTextEx(
        GetFontDefault(), TextFormat("[%i, %i]", GetMouseX(), GetMouseY()),
        Vector2Add(GetMousePosition(), (Vector2){-44, -24}), 20, 2, BLACK);
    // Helper Info
    DrawText(TextFormat("Camera Target: [%i, %i]", int(camera.target.x),
                        int(camera.target.y)),
             10, 10, 20, DARKBLUE);
    DrawFPS(10, 40);

    DrawRectangleRec(rightBar, Fade(LIGHTGRAY, 0.3f));

    // Draw GUI controls
    // Check all possible UI states that require controls lock
    if (fittingTypeEditMode) GuiLock();

    GuiLabel((Rectangle){rightPos, 10 + 24 + 28, 140, 24}, "Sample Range:");
    if (GuiSpinner((Rectangle){rightPos, 10 + 24 + 28 + 24, 140, 24}, "",
                   &sampleRange, 1, 100, sampleRangeEditMode))
      sampleRangeEditMode = !sampleRangeEditMode;

    if (GuiButton((Rectangle){rightPos, 10 + 24 + 28 + 24 + 24, 140, 24},
                  "#191#Clear Points"))
      clearPoints = true;

    if (GuiButton((Rectangle){rightPos, 10 + 24 + 28 + 24 + 28 + 24, 140, 24},
                  "Calculate Fitting"))
      calculateFitting = true;

    GuiUnlock();

    GuiLabel((Rectangle){rightPos, 10, 140, 24}, "Fitting type:");
    if (GuiDropdownBox((Rectangle){rightPos, 10 + 24, 140, 28},
                       "ONE;TWO;THREE;FOUR", &fittingTypeActive,
                       fittingTypeEditMode))
      fittingTypeEditMode = !fittingTypeEditMode;

    EndDrawing();
  }

  // De-Initializatoin
  // ---------------------------------------------------------------------------
  CloseWindow();

  return 0;
}