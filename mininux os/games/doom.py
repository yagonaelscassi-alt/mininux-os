import math
import msvcrt
import os
import sys
import time

# --- PANTALLA ULTRA COMPACTA ---
SCREEN_WIDTH = 40
SCREEN_HEIGHT = 18
FOV = math.pi / 3
MAX_DEPTH = 12.0

# --- CÓDIGOS DE COLOR ANSI ---
COLOR_RESET = "\033[0m"
COLOR_WALL_1 = "\033[91m"  # Rojo brillante (Cerca)
COLOR_WALL_2 = "\033[31m"  # Rojo oscuro
COLOR_WALL_3 = "\033[33m"  # Amarillo (Lejos)
COLOR_WALL_4 = "\033[35m"  # Magenta (Muy lejos)
COLOR_FLOOR = "\033[32m"  # Verde clásico
COLOR_CEILING = "\033[30m\033[1m"  # Gris oscuro (Cielo)
COLOR_HUD = "\033[96m"  # Cian brillante

# --- MAPA DEL JUEGO ---
MAP_DATA = [
    "########################################",
    "#..................#...................#",
    "#.###.####.######..#.######.####.###...#",
    "#.#....#......#....#....#......#....#..#",
    "#.#.##.#.####.#.########.#.####.#.##.#..#",
    "#.#..#....#....#..##..#....#....#..#.#..#",
    "#.##.####.#.##.#..##..#.##.#.####.##.#..#",
    "#......................................#",
    "#.######.##.##########.##.######.#####.#",
    "#.#......##....#..#....##......#.....#.#",
    "#.#.##########.#..#.##########.#####.#.#",
    "#.#............#..#............#.....#.#",
    "#.##############..##############.#####.#",
    "#......................................#",
    "########################################",
]

MAP_HEIGHT = len(MAP_DATA)
MAP_WIDTH = len(MAP_DATA[0])

player_x = 2.5
player_y = 2.5
player_a = 0.0
player_hp = 100
score = 0
enemies = []


def is_wall(x, y):
  margin = 0.2
  for cx in [x - margin, x + margin]:
    for cy in [y - margin, y + margin]:
      if cx < 0 or cx >= MAP_WIDTH or cy < 0 or cy >= MAP_HEIGHT:
        return True
      if MAP_DATA[int(cy)][int(cx)] == "#":
        return True
  return False


def get_non_blocking_input():
  if msvcrt.kbhit():
    try:
      return msvcrt.getch().decode("utf-8").lower()
    except:
      return None
  return None


def main():
  global player_x, player_y, player_a, player_hp, score

  os.system("")  # Activar soporte de colores ANSI en Windows
  print("\033[?25l", end="")  # Ocultar cursor

  try:
    last_time = time.time()
    while player_hp > 0:
      current_time = time.time()
      dt = current_time - last_time
      last_time = current_time

      key = get_non_blocking_input()

      move_speed = 7.0 * dt
      rot_speed = 6.5 * dt

      if key == "x":
        break
      elif key == "r":  # Reiniciar posición
        player_x, player_y, player_a = 2.5, 2.5, 0.0
      elif key == "k":  # Girar izquierda
        player_a -= rot_speed
      elif key == "l":  # Girar derecha
        player_a += rot_speed
      elif key == "w":  # Avanzar
        nx = player_x + math.cos(player_a) * move_speed
        ny = player_y + math.sin(player_a) * move_speed
        if not is_wall(nx, player_y):
          player_x = nx
        if not is_wall(player_x, ny):
          player_y = ny
      elif key == "s":  # Retroceder
        nx = player_x - math.cos(player_a) * move_speed
        ny = player_y - math.sin(player_a) * move_speed
        if not is_wall(nx, player_y):
          player_x = nx
        if not is_wall(player_x, ny):
          player_y = ny
      elif key == "a":  # Strafe Izquierda
        nx = player_x + math.sin(player_a) * move_speed
        ny = player_y - math.cos(player_a) * move_speed
        if not is_wall(nx, player_y):
          player_x = nx
        if not is_wall(player_x, ny):
          player_y = ny
      elif key == "d":  # Strafe Derecha
        nx = player_x - math.sin(player_a) * move_speed
        ny = player_y + math.cos(player_a) * move_speed
        if not is_wall(nx, ny):
          player_x = nx
        if not is_wall(player_x, ny):
          player_y = ny

      screen_output = []

      for y in range(SCREEN_HEIGHT):
        row_str = ""
        for x in range(SCREEN_WIDTH):
          # Si es la última línea, imprimimos el HUD a color
          if y == SCREEN_HEIGHT - 1:
            hud_msg = f" W/A/S/D:Mover | K/L:Girar | R:Reiniciar | X:Salir "
            row_str = (
                COLOR_HUD + hud_msg[:SCREEN_WIDTH].ljust(SCREEN_WIDTH) + COLOR_RESET
            )
            break

          ray_angle = player_a - FOV / 2 + (x / SCREEN_WIDTH) * FOV
          distance_to_wall = 0.0
          hit_wall = False

          cos_a = math.cos(ray_angle)
          sin_a = math.sin(ray_angle)

          while not hit_wall and distance_to_wall < MAX_DEPTH:
            distance_to_wall += 0.1
            test_x = int(player_x + cos_a * distance_to_wall)
            test_y = int(player_y + sin_a * distance_to_wall)

            if (
                test_x < 0
                or test_x >= MAP_WIDTH
                or test_y < 0
                or test_y >= MAP_HEIGHT
            ):
              hit_wall = True
              distance_to_wall = MAX_DEPTH
            elif MAP_DATA[test_y][test_x] == "#":
              hit_wall = True

          ceiling = int((SCREEN_HEIGHT / 2) - SCREEN_HEIGHT / distance_to_wall)
          floor = SCREEN_HEIGHT - ceiling

          if y < ceiling:
            row_str += " "  # Cielo vacío
          elif ceiling <= y <= floor:
            # Seleccionar color de muro según la distancia (efecto profundidad)
            if distance_to_wall < MAX_DEPTH / 4:
              row_str += COLOR_WALL_1 + "█" + COLOR_RESET
            elif distance_to_wall < MAX_DEPTH / 2:
              row_str += COLOR_WALL_2 + "▓" + COLOR_RESET
            elif distance_to_wall < MAX_DEPTH / 1.5:
              row_str += COLOR_WALL_3 + "▒" + COLOR_RESET
            else:
              row_str += COLOR_WALL_4 + "░" + COLOR_RESET
          else:
            # Suelo texturizado a color
            floor_char = "." if y % 2 == 0 else ","
            row_str += COLOR_FLOOR + floor_char + COLOR_RESET

        screen_output.append(row_str)

      # Volcar búfer a la terminal sin parpadeo
      output = "\033[H" + "\n".join(screen_output)
      sys.stdout.write(output)
      sys.stdout.flush()

      time.sleep(0.01)

  finally:
    print("\033[?25h" + COLOR_RESET)  # Restaurar cursor y colores
    print("\n¡Juego terminado!")


if __name__ == "__main__":
  main()