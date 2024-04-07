#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define ARRAY_MAX 1 << 16
#define max(a, b) a < b ? b : a
#define printv(v) printf("(%f, %f)\n", v.x, v.y)

#define GRIDSIZE 8

typedef struct Polygon {
  int len;
  Vector2 *points;
} Polygon;
Polygon *polygons = NULL;
int polygons_len = 0;
int hot_polygon = 0;
Rectangle finish_rect = {0};
int finish_rect_hot = 0;

int points[800][600] = {0};

Texture
generate_grid()
{
  int gridsize = GRIDSIZE;
  uint32_t *data = malloc(sizeof(*data) * (gridsize * 2) * (gridsize * 2));
  char *curr = (char*) data;
  for (int y = 0; y < gridsize * 2; y++)
    {
      for (int x = 0; x < gridsize * 2; x++)
	{
	  Color current_color = WHITE;
	  if ((x % gridsize == 0 || x % gridsize == gridsize - 1) && (y % gridsize == 0 || y % gridsize == gridsize - 1))
	    current_color = BLUE;
	  curr[0] = (current_color.r);
	  curr[1] = (current_color.g);
	  curr[2] = (current_color.b);
	  curr[3] = (current_color.a);
	  curr += 4;
	}
    }
  Image image = {data, gridsize * 2, gridsize * 2, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
  return LoadTextureFromImage(image);
}

int CheckCollisionPointPoly(Vector2 point, Vector2 *points, int pointCount)
{
  int inside = 0;

  if (pointCount > 2)
    {
      for (int i = 0, j = pointCount - 1; i < pointCount; j = i++)
        {
	  if ((points[i].y > point.y) != (points[j].y > point.y) &&
	      (point.x < (points[j].x - points[i].x) * (point.y - points[i].y) / (points[j].y - points[i].y) + points[i].x))
            {
	      inside = !inside;
            }
        }
    }

  return inside;
}

int main(void)
{
  polygons = calloc(ARRAY_MAX, sizeof(*polygons));
  for (int i = 0; i < ARRAY_MAX; i++) {
    polygons[i].points = calloc(ARRAY_MAX, sizeof(*polygons[i].points));
  }
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(800, 600, "Walk Monster");
  Texture grid_texture = generate_grid();
  Vector2 *vees = calloc(1 << 16, sizeof(*vees));
  while (!WindowShouldClose()) {
    finish_rect_hot = CheckCollisionPointRec(GetMousePosition(), finish_rect);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      if (!hot_polygon) {
	hot_polygon = 1;
	finish_rect.x = GetMouseX();
	finish_rect.y = GetMouseY();
	finish_rect.width = 5;
	finish_rect.height = 5;

	polygons_len += 1;

	Polygon *p = &polygons[max(polygons_len - 1, 0)];
	p->points[p->len] = GetMousePosition();
	p->len += 1;
      } else {
	if (finish_rect_hot) {
	  hot_polygon = 0;
	  goto collision;
	}
	Polygon *p = &polygons[max(polygons_len - 1, 0)];
	p->points[p->len] = GetMousePosition();
	p->len += 1;
      }
    }

  collision:
    Vector2 last_pole = {-10000, -10000};
    memset(vees, 0, 1 << 16);
    int vees_len = 0;

    for (int i = 0; i < 800 * 600; i++) {
      Vector2 pole = {0};
      pole.x = (i * GRIDSIZE) % 800;
      pole.y = (i / 800) * GRIDSIZE;

      int this_pole = 0;
      int the_last_pole = 0;
      for (int pi = 0; pi < polygons_len; pi++) {
	Polygon *p = &polygons[pi];
	if (p->len < 3) continue;
	if (last_pole.x != -10000) {
	  this_pole = this_pole == 1 ? 1 : CheckCollisionPointPoly(pole, p->points, p->len);
	  the_last_pole = the_last_pole == 1 ? 1 : CheckCollisionPointPoly(last_pole, p->points, p->len);
	  if (this_pole == 1 && this_pole == the_last_pole) break;
	}
      }
      if (this_pole != the_last_pole) {
	vees[vees_len++] = this_pole ? last_pole : pole;
	vees[vees_len++] = this_pole ? pole : last_pole;
      }
      last_pole = pole;
    }

    for (int i = 0; i < 800 * 600; i++) {
      Vector2 pole = {0};
      pole.x = (i / 600) * GRIDSIZE;
      pole.y = (i * GRIDSIZE) % 600;

      int this_pole = 0;
      int the_last_pole = 0;
      for (int pi = 0; pi < polygons_len; pi++) {
	Polygon *p = &polygons[pi];
	if (p->len < 3) continue;
	if (last_pole.x != -10000) {
	  this_pole = this_pole == 1 ? 1 : CheckCollisionPointPoly(pole, p->points, p->len);
	  the_last_pole = the_last_pole == 1 ? 1 : CheckCollisionPointPoly(last_pole, p->points, p->len);
	  if (this_pole == 1 && this_pole == the_last_pole) break;
	}
      }
      if (this_pole != the_last_pole) {
	vees[vees_len++] = this_pole ? last_pole : pole;
	vees[vees_len++] = this_pole ? pole : last_pole;
      }
      last_pole = pole;
    }

    int polygon_edge_vees_len = 0;
    for (int i = 0; i < vees_len / 2; i += 2) {
      // Line 1
      Vector2 v1 = vees[i];
      Vector2 v2 = vees[i + 1];
      for (int pi = 0; pi < polygons_len; pi++) {
	Polygon *p = &polygons[pi];
	for (int j = 0; j < p->len; j++) {
	  Vector2 p1 = p->points[j];
	  Vector2 p2 = p->points[(j + 1) % p->len];
	  Vector2 collides = {0};
	  if (CheckCollisionLines(v1, v2, p1, p2, &collides)) {vees[polygon_edge_vees_len++] = collides;}
	}
      }
    }

    BeginDrawing();

    Rectangle src = (Rectangle){-100000, -100000, 200000, 200000};
    Rectangle dest = src;
    DrawTexturePro(grid_texture, src, dest, (Vector2){0}, 0, WHITE);
    DrawRectangleRec(finish_rect, finish_rect_hot ? RED : BLUE);

    /*for (int i = 0; i < polygons_len; i++) {
      Polygon *p = &polygons[i];
      for (int j = 0; j < p->len; j++) {
	DrawLineV(p->points[j], p->points[(j + 1) % p->len], BLACK);
      }
    }*/

    for (int i = 0; i < polygon_edge_vees_len; i += 1) {
      DrawRectangle(vees[i].x, vees[i].y, 4, 4, RED);
    }
    DrawFPS(0, 0);
    EndDrawing();
  }
}

