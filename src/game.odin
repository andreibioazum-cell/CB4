package main

import "core:math"

BULLET_SPEED :: f32(720.0)
PLAYER_SPEED :: f32(560.0)
PLAYER_RADIUS :: 26

COLOR_BACKGROUND :: u32(0xFF111827)
COLOR_GRID       :: u32(0xFF1D2A42)
COLOR_GRID_BRIGHT :: u32(0xFF263B5C)
COLOR_WHITE     :: u32(0xFFFFFFFF)
COLOR_BLACK     :: u32(0xFF05070D)

Player :: struct {
    x:          f32,
    y:          f32,
    angle:      f32,
    last_angle: f32,
}

Bullet :: struct {
    x:       f32,
    y:       f32,
    vx:      f32,
    vy:      f32,
    radius:  int,
    active:  bool,
}

Game :: struct {
    player:      Player,
    joystick:    Joystick,
    attack:      Button,
    bullet:      Bullet,
    width:       int,
    height:      int,
    initialized: bool,
    last_time:   f64,
    fps_time:    f64,
    fps_frames:  int,
    fps:         int,
}

clamp_float :: proc(value, low, high: f32) -> f32 {
    if value < low { return low }
    if value > high { return high }
    return value
}

game_layout_controls :: proc(game: ^Game) {
    control_size := game.height / 7
    control_size = clamp_int(control_size, 72, 132)
    game.joystick.center_x = control_size + 18
    game.joystick.center_y = game.height - control_size - 18
    game.joystick.radius = control_size

    game.attack.x = game.width - control_size - 18
    game.attack.y = game.height - control_size - 18
    game.attack.radius = control_size - 12
}

game_init :: proc(game: ^Game, width, height: int) {
    game^ = Game{}
    game.width = width
    game.height = height
    game.player.x = f32(width) * 0.5
    game.player.y = f32(height) * 0.5
    game.player.angle = 0
    game.player.last_angle = 0
    game.bullet.radius = 11
    game.fps = 60
    game.initialized = true
    game_layout_controls(game)
    game.last_time = monotonic_seconds()
}

game_resize :: proc(game: ^Game, width, height: int) {
    if width <= 0 || height <= 0 {
        return
    }
    game.width = width
    game.height = height
    game_layout_controls(game)
    game.player.x = clamp_float(game.player.x, f32(PLAYER_RADIUS), f32(width - PLAYER_RADIUS))
    game.player.y = clamp_float(game.player.y, f32(PLAYER_RADIUS), f32(height - PLAYER_RADIUS))
}

game_fire :: proc(game: ^Game) {
    if game.bullet.active {
        return
    }
    sin_angle := math.sin_f32(game.player.angle)
    cos_angle := math.cos_f32(game.player.angle)
    muzzle_distance := f32(PLAYER_RADIUS + 16)
    game.bullet.x = game.player.x + sin_angle*muzzle_distance
    game.bullet.y = game.player.y - cos_angle*muzzle_distance
    game.bullet.vx = sin_angle * BULLET_SPEED
    game.bullet.vy = -cos_angle * BULLET_SPEED
    game.bullet.radius = 11
    game.bullet.active = true
}

game_update :: proc(game: ^Game) {
    if !game.initialized {
        return
    }
    now := monotonic_seconds()
    dt := f32(now - game.last_time)
    game.last_time = now
    if dt <= 0 || dt > 0.1 {
        dt = 1.0 / 60.0
    }

    game.player.x += game.joystick.dir_x * PLAYER_SPEED * dt
    game.player.y += game.joystick.dir_y * PLAYER_SPEED * dt
    game.player.x = clamp_float(game.player.x, f32(PLAYER_RADIUS), f32(game.width - PLAYER_RADIUS))
    game.player.y = clamp_float(game.player.y, f32(PLAYER_RADIUS), f32(game.height - PLAYER_RADIUS))

    direction_length := math.sqrt_f32(game.joystick.dir_x*game.joystick.dir_x + game.joystick.dir_y*game.joystick.dir_y)
    if direction_length > 0.001 {
        // Angle zero points up, matching the visual orientation of the ship.
        game.player.angle = math.atan2_f32(game.joystick.dir_x, -game.joystick.dir_y)
        game.player.last_angle = game.player.angle
    } else {
        game.player.angle = game.player.last_angle
    }

    if game.bullet.active {
        game.bullet.x += game.bullet.vx * dt
        game.bullet.y += game.bullet.vy * dt
        if game.bullet.x < -game.bullet.radius || game.bullet.x > f32(game.width + game.bullet.radius) ||
           game.bullet.y < -game.bullet.radius || game.bullet.y > f32(game.height + game.bullet.radius) {
            game.bullet.active = false
        }
    }

    game.fps_time += f64(dt)
    game.fps_frames += 1
    if game.fps_time >= 1.0 {
        game.fps = int(f64(game.fps_frames) / game.fps_time + 0.5)
        game.fps_time = 0
        game.fps_frames = 0
    }
}

game_draw_background :: proc(buffer: ^Render_Buffer) {
    graphics_clear(buffer, COLOR_BACKGROUND)
    grid_step := 48
    for x := 0; x < buffer.width; x += grid_step {
        color := COLOR_GRID
        if x % (grid_step * 4) == 0 { color = COLOR_GRID_BRIGHT }
        graphics_draw_line(buffer, Point{x, 0}, Point{x, buffer.height - 1}, color)
    }
    for y := 0; y < buffer.height; y += grid_step {
        color := COLOR_GRID
        if y % (grid_step * 4) == 0 { color = COLOR_GRID_BRIGHT }
        graphics_draw_line(buffer, Point{0, y}, Point{buffer.width - 1, y}, color)
    }
    graphics_draw_ring(buffer, buffer.width / 2, buffer.height / 2, 90, 1, 0xFF213653)
    graphics_draw_ring(buffer, buffer.width / 2, buffer.height / 2, 210, 1, 0xFF1A2B47)
}

game_draw_player :: proc(buffer: ^Render_Buffer, player: ^Player) {
    sin_angle := math.sin_f32(player.angle)
    cos_angle := math.cos_f32(player.angle)
    forward_x := sin_angle
    forward_y := -cos_angle
    side_x := cos_angle
    side_y := sin_angle

    glow_x := int(player.x - forward_x*5)
    glow_y := int(player.y - forward_y*5)
    graphics_draw_circle(buffer, glow_x, glow_y, PLAYER_RADIUS + 10, 0xFF172D58)

    front := Point{
        int(player.x + forward_x*32),
        int(player.y + forward_y*32),
    }
    left := Point{
        int(player.x - forward_x*18 - side_x*21),
        int(player.y - forward_y*18 - side_y*21),
    }
    right := Point{
        int(player.x - forward_x*18 + side_x*21),
        int(player.y - forward_y*18 + side_y*21),
    }
    back := Point{
        int(player.x - forward_x*25),
        int(player.y - forward_y*25),
    }

    graphics_draw_triangle(buffer, front, left, back, 0xFF3B82F6)
    graphics_draw_triangle(buffer, front, back, right, 0xFF2563EB)
    graphics_draw_line(buffer, front, left, 0xFFB9D8FF)
    graphics_draw_line(buffer, left, back, 0xFF75A9FF)
    graphics_draw_line(buffer, back, right, 0xFF75A9FF)
    graphics_draw_line(buffer, right, front, 0xFFE0EEFF)

    cockpit := Point{
        int(player.x + forward_x*4),
        int(player.y + forward_y*4),
    }
    graphics_draw_circle(buffer, cockpit.x, cockpit.y, 7, 0xFFBCE7FF)
    graphics_draw_circle(buffer, cockpit.x, cockpit.y, 3, 0xFF0C1C3A)
}

game_draw :: proc(game: ^Game, buffer: ^Render_Buffer) {
    game_draw_background(buffer)
    if game.bullet.active {
        tail := Point{
            int(game.bullet.x - game.bullet.vx*0.025),
            int(game.bullet.y - game.bullet.vy*0.025),
        }
        graphics_draw_line(buffer, tail, Point{int(game.bullet.x), int(game.bullet.y)}, 0xFFFFB000)
        graphics_draw_circle(buffer, int(game.bullet.x), int(game.bullet.y), game.bullet.radius + 4, 0xFF6B4211)
        graphics_draw_circle(buffer, int(game.bullet.x), int(game.bullet.y), game.bullet.radius, 0xFFFFD166)
    }
    game_draw_player(buffer, &game.player)
    ui_draw_joystick(buffer, &game.joystick)
    ui_draw_button(buffer, &game.attack)

    ui_draw_text_outlined(buffer, 18, 18, `FPS:`, 2, COLOR_WHITE, COLOR_BLACK)
    ui_draw_number(buffer, 72, 18, game.fps, 2, COLOR_WHITE, COLOR_BLACK)
}
