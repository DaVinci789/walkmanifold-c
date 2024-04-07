#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// #define DEBUG

#define max(a, b) (a) < (b) ? (b) : (a)
#define next_empty_n(buf, len, cap, n) (*(len) < cap - n ? (*(len) += n, buf + (*(len) - n)) : 0)

#define next_empty(buf, len, cap) next_empty_n(buf, len, cap, 1)
#define next_empty_in_arr(a) next_empty_n(a, &a##_len, a##_cap, 1)
#define push_arr(elm, a, len, cap) *next_empty(a, len, cap) = elm
#define push(elm, a) push_arr(elm, a, &a##_len, a##_cap)
#define SemanticArray(type, name, cap)		\
  type name[cap] = {0}; \
  int name##_len = 0; \
  int name##_cap = cap
#define SemanticArrayArgs(type, name, cap)		\
  type name[cap]; \
  int name##_len; \
  int name##_cap

int GRIDSIZE = 64;

typedef struct Polygon {
  SemanticArrayArgs(Vector2, points, 1 << 8);
} Polygon;

typedef struct EdgeNode EdgeNode;
struct EdgeNode {
  Vector2 a1;
  Vector2 b1;
  EdgeNode *prev;
  EdgeNode *next;
};

SemanticArray(Polygon, polygons, 1<<8);
SemanticArray(Vector2, edge_points, 1<<16);
SemanticArray(EdgeNode, edge_pairs, 1<<16);
SemanticArray(Vector2, grid_deltas, 1<<16);

Rectangle finish_rect = {0};
int hot_polygon = 0;
int finish_rect_hot = 0;

int show_edge_points = 1;

Texture
generate_grid()
{
  int gridsize = GRIDSIZE;
  uint32_t *data = malloc(sizeof(*data) * (gridsize * 2) * (gridsize * 2));
  char *curr = (char*) data;
  for (int y = 0; y < gridsize * 2; y++) {
    for (int x = 0; x < gridsize * 2; x++) {
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
  Texture tex = LoadTextureFromImage(image);
  free(data);
  return tex;
}

int CheckCollisionPointPoly(Vector2 point, Vector2 *points, int pointCount)
{
  int inside = 0;

  if (pointCount > 2) {
    for (int i = 0, j = pointCount - 1; i < pointCount; j = i++) {
      if ((points[i].y > point.y) != (points[j].y > point.y) &&
          (point.x < (points[j].x - points[i].x) * (point.y - points[i].y) / (points[j].y - points[i].y) + points[i].x)) {
	      inside = !inside;
      }
    }
  }

  return inside;
}

int
main(void)
{
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(800, 600, "Walk Monster");

  Texture grid_texture = generate_grid();
  Rectangle src = (Rectangle){-100000, -100000, 200000, 200000};
  Rectangle dest = src;

  for (int i = 0; i < polygons_cap; i++) {
    polygons[i].points_cap = 1 << 8;
  }

  for (int i = 0; i < edge_pairs_cap; i++) {
    edge_pairs[i].next = NULL;
  }

  while (!WindowShouldClose()) {
    edge_points_len = 0;
    edge_pairs_len = 0;
    grid_deltas_len = 0;
    manifold_len = 0;
    finish_rect_hot = CheckCollisionPointRec(GetMousePosition(), finish_rect);

    if (IsKeyPressed(KEY_D)) {
      show_edge_points = !show_edge_points;
    }

    if (IsKeyPressed(KEY_UP)) {
      GRIDSIZE += 4;
      UnloadTexture(grid_texture);
      grid_texture = generate_grid();
    }
    if (IsKeyPressed(KEY_DOWN)) {
      GRIDSIZE -= 4;
      UnloadTexture(grid_texture);
      grid_texture = generate_grid();
    }

    if (IsKeyPressed(KEY_R)) {
      polygons_len = 0;
      for (int i = 0; i < polygons_cap; i++) {
	polygons[i].points_len = 0;
      }
      hot_polygon = 0;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      if (!hot_polygon) {
	hot_polygon = 1;
	finish_rect.x = GetMouseX();
	finish_rect.y = GetMouseY();
	finish_rect.width = 5;
	finish_rect.height = 5;

	polygons_len += 1;
      } else {
	if (finish_rect_hot) {
	  hot_polygon = 0;
	  goto next;
	}
      }
      Polygon *p = &polygons[max(polygons_len - 1, 0)];
      push(GetMousePosition(), p->points);
    }

  next:
    for (int x = 0; x < GetScreenWidth(); x += GRIDSIZE) {
      for (int y = 0; y < GetScreenHeight(); y += GRIDSIZE) {
	Vector2 v1 = (Vector2) {x, y};
	Vector2 v2 = (Vector2) {x + GRIDSIZE, y};
	Vector2 v3 = (Vector2) {x + GRIDSIZE, y + GRIDSIZE};
	Vector2 v4 = (Vector2) {x, y + GRIDSIZE};
	int c1 = 0;
	int c2 = 0;
	int c3 = 0;
	int c4 = 0;

	for (int i = 0; i < polygons_len; i++) {
	  Polygon p = polygons[i];
	  c1 = c1 == 0 ? CheckCollisionPointPoly(v1, p.points, p.points_len) : 1;
	  c2 = c2 == 0 ? CheckCollisionPointPoly(v2, p.points, p.points_len) : 1;
	  c3 = c3 == 0 ? CheckCollisionPointPoly(v3, p.points, p.points_len) : 1;
	  c4 = c4 == 0 ? CheckCollisionPointPoly(v4, p.points, p.points_len) : 1;
	}

#define point_for for (int j = 0; j < p.points_len; j++)
	int edge_points_prelen = edge_points_len;
	for (int i = 0; i < polygons_len; i++) {
	  Polygon p = polygons[i];
	  if (c1 != c2) {
	    point_for {
	      Vector2 collide = {0};
	      if (CheckCollisionLines(v1, v2,
				      p.points[j], p.points[(j + 1) % p.points_len],
		                      &collide)) {
		push(collide, edge_points);
		push(v1, grid_deltas);
		push(v2, grid_deltas);
	      }
	    }
	  }
	  if (c2 != c3) {
	    point_for {
	      Vector2 collide = {0};
	      if (CheckCollisionLines(v2, v3,
				      p.points[j], p.points[(j + 1) % p.points_len],
		                      &collide)) {
		push(collide, edge_points);
		push(v2, grid_deltas);
		push(v3, grid_deltas);
	      }
	    }
	  }
	  if (c3 != c4) {
	    point_for {
	      Vector2 collide = {0};
	      if (CheckCollisionLines(v3, v4,
				      p.points[j], p.points[(j + 1) % p.points_len],
		                      &collide)) {
		push(collide, edge_points);
		push(v3, grid_deltas);
		push(v4, grid_deltas);
	      }
	    }
	  }
	  if (c4 != c1) {
	    point_for {
	      Vector2 collide = {0};
	      if (CheckCollisionLines(v4, v1,
				      p.points[j], p.points[(j + 1) % p.points_len],
		                      &collide)) {
		push(collide, edge_points);
		push(v4, grid_deltas);
		push(v1, grid_deltas);
	      }
	    }
	  }
	}
#undef point_for

	if (edge_points_len - edge_points_prelen >= 2) {
	  EdgeNode first_pair = {0};
	  first_pair.a1 = edge_points[edge_points_len - 2];
	  first_pair.b1 = edge_points[edge_points_len - 1];

	  EdgeNode *next_empty = next_empty_in_arr(edge_pairs);
	  *next_empty = first_pair;
	  next_empty->next = &edge_pairs[0];
	  next_empty->prev = &edge_pairs[max(0, edge_pairs_len - 2)];
	  (next_empty->next)->prev = next_empty;
	  (next_empty->prev)->next = next_empty;

	  if (edge_pairs_len > 1)
	    edge_pairs[edge_pairs_len - 2].next = &edge_pairs[edge_pairs_len - 1];
	} else if (edge_points_len - edge_points_prelen == 4) {
	}
      }
    }

    EdgeNode *start_pair = &edge_pairs[0];
    Vector2 query = start_pair->a1;
    while (edge_pairs_len > 0) {
      for (EdgeNode *current_pair = start_pair->next;
	   ;
	   current_pair = current_pair->next) {
	if (current_pair == start_pair) {
	  push(query, manifold);
	  (start_pair->prev)->next = start_pair->next;
	  (start_pair->next)->prev = start_pair->prev;
	  start_pair = current_pair->next;
	  current_pair = start_pair->next;
	  edge_pairs_len -= 1;
	  query = current_pair->a1;
	  break;
	}
	int equal_a = Vector2Equals(query, current_pair->a1);
	int equal_b = Vector2Equals(query, current_pair->b1);
	if (equal_a || equal_b) {
	  push(query, manifold);
	  if (equal_a) {
	    query = current_pair->b1;
	  } else if (equal_b) {
	    query = current_pair->a1;
	  }
	  (start_pair->prev)->next = start_pair->next;
	  (start_pair->next)->prev = start_pair->prev;
	  start_pair = current_pair;
	  edge_pairs_len -= 1;
	}
      }
    }

    BeginDrawing();
    ClearBackground(WHITE);
    DrawTexturePro(grid_texture, src, dest, (Vector2){0}, 0, WHITE);

    for (int i = 0; i < polygons_len; i++) {
      Polygon p = polygons[i];
      for (int j = 0; j < p.points_len; j++) {
	DrawLineV(p.points[j], p.points[(j + 1) % p.points_len], BLACK);
      }
    }

    if (show_edge_points) {
      for (int i = 0; i < edge_points_len; i++) {
	DrawRectangle(edge_points[i].x, edge_points[i].y, 5, 5, GREEN);
      }
    }

    for (int i = 0; i < manifold_len; i++) {
      DrawLineV(manifold[i], manifold[(i + 1) % manifold_len], ORANGE);
    }

    /* for (int i = 0; i < grid_deltas_len; i++) { */
    /*   DrawRectangle(grid_deltas[i].x, grid_deltas[i].y, 5, 5, RED); */
    /* } */

    DrawRectangleRec(finish_rect, finish_rect_hot ? RED : BLUE);

    EndDrawing();
  }
}
