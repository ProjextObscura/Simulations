#include "raylib.h"
#include <math.h>
#include <stdlib.h> 
#include <stdio.h> 

// --- Simulation Constants ---
#define MAX_PARTICLES 1000
#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600
#define PARTICLE_RADIUS 1.5f
#define FLOW_STRENGTH 0.8f
#define DAMPING_FACTOR 0.995f
#define VECTOR_GRID_SIZE 40  // Size of the grid cells for the static hash field

// --- Data Structures ---

typedef struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life; // Life as a float (0.0 to 1.0)
    bool active;
} Particle;

// --- Vector Field Function (Standard C Hash Flow) ---

// Calculates the force vector at a given point (x, y) using a static hash function.
// This function avoids Raylib's PerlinNoise for maximum compatibility.
Vector2 GetVectorFieldForce(float x, float y, float time) {
    // 1. Convert float coordinates to integers for hashing
    // Divide coordinates by a factor related to the grid size to get consistent force per cell.
    int ix = (int)(x / (VECTOR_GRID_SIZE * 2));
    int iy = (int)(y / (VECTOR_GRID_SIZE * 2));

    // 2. Simple 2D hashing function to get a consistent pseudo-random angle per cell
    // The "magic numbers" (13, 23, 99991) are chosen to mix the coordinates well.
    unsigned int seed = (unsigned int)(ix * 13 + iy * 23);
    
    // 3. Use an operation to generate a large, pseudo-random number from the seed,
    // and scale it to an angle in radians [0, 2*PI].
    float angle = (float)((seed * 99991) % 100000) / 100000.0f * 2.0f * PI;

    // 4. Convert the angle into a normalized vector (unit vector)
    Vector2 force = { cosf(angle), sinf(angle) };
    
    // 5. Scale the force by a constant strength factor
    force.x *= FLOW_STRENGTH;
    force.y *= FLOW_STRENGTH;
    
    return force;
}

// --- Main Program Logic ---
int main(void) {
    // Initialization
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Hash-Based Particle Flow Field");
    SetTargetFPS(60); 

    Particle particles[MAX_PARTICLES] = { 0 };
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
    }
    
    // Toggle for drawing the vector field
    bool drawField = true; 

    // Main game loop
    while (!WindowShouldClose()) {
        // --- Update ---
        float dt = GetFrameTime();
        float time = (float)GetTime();

        if (IsKeyPressed(KEY_SPACE)) drawField = !drawField;

        // Particle Emitter (Spawn at mouse position)
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < MAX_PARTICLES; j++) {
                    if (!particles[j].active) {
                        particles[j].active = true;
                        particles[j].life = 1.0f; // Full life
                        
                        // Spawn randomly near the mouse
                        particles[j].position = (Vector2){ mousePos.x + GetRandomValue(-10, 10), mousePos.y + GetRandomValue(-10, 10) };
                        particles[j].velocity = (Vector2){ 0.0f, 0.0f };
                        particles[j].color = (Color){ 0, 180, 255, 255 }; // Bright Blue
                        break;
                    }
                }
            }
        }
        
        // Update all active particles
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].active) {
                // 1. Get Vector Field Force
                Vector2 fieldForce = GetVectorFieldForce(particles[i].position.x, particles[i].position.y, time);
                
                // 2. Apply Force and Damping (v += a * dt)
                particles[i].velocity.x += fieldForce.x * dt;
                particles[i].velocity.y += fieldForce.y * dt;
                
                // Apply damping (slows the particle down over time)
                particles[i].velocity.x *= DAMPING_FACTOR;
                particles[i].velocity.y *= DAMPING_FACTOR;
                
                // 3. Update Position (p += v * dt)
                particles[i].position.x += particles[i].velocity.x;
                particles[i].position.y += particles[i].velocity.y;

                // 4. Decay and Deactivate
                particles[i].life -= dt / 10.0f; // Particle lives for about 10 seconds 
                
                // Fade effect and color shift
                unsigned char alpha = (unsigned char)(255 * particles[i].life);
                particles[i].color.a = alpha;
                
                // Color shift from blue to white as it dies (R: 0->255, G: 180->255, B: 255->255)
                particles[i].color.r = (unsigned char)(0 + 255 * (1.0f - particles[i].life));
                particles[i].color.g = (unsigned char)(180 + (255 - 180) * (1.0f - particles[i].life));
                // Blue component stays max for a bright blue to white transition
                
                
                if (particles[i].life <= 0.0f) {
                    particles[i].active = false;
                }
            }
        }

        // --- Draw ---
        BeginDrawing();

        ClearBackground(BLACK);
        
        // OPTIONAL: Draw the Vector Field Grid 
        if (drawField) {
            Color vectorColor = Fade(GRAY, 0.2f); // Faded gray for subtle background
            for (int y = 0; y < GetScreenHeight(); y += VECTOR_GRID_SIZE) {
                for (int x = 0; x < GetScreenWidth(); x += VECTOR_GRID_SIZE) {
                    Vector2 start = { (float)x, (float)y };
                    // Note: 'time' parameter is included in the function signature but ignored by the hash field
                    Vector2 force = GetVectorFieldForce((float)x, (float)y, time); 
                    
                    // Scale the vector for drawing purposes (make it visible)
                    float drawScale = 50.0f; 
                    Vector2 end = { start.x + force.x * drawScale, start.y + force.y * drawScale };
                    
                    // Draw a line segment representing the vector
                    DrawLineV(start, end, vectorColor);
                    
                    // Draw a small circle at the end of the line (head of the arrow)
                    DrawCircleV(end, 2.0f, vectorColor);
                }
            }
        }
        
        // Draw the particles
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].active) {
                DrawCircleV(particles[i].position, PARTICLE_RADIUS, particles[i].color);
            }
        }
        
        // UI Text
        // Calculate the actual number of active particles for the text display
        int active_particles = 0;
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].active) active_particles++;
        }
        
        DrawText(TextFormat("Particles: %d/%d", active_particles, MAX_PARTICLES), 10, 10, 20, WHITE);
        DrawText("Left Click: Emit | SPACE: Toggle Field", 10, 30, 20, WHITE);
        DrawFPS(GetScreenWidth() - 80, 10);

        EndDrawing();
    }

    // De-Initialization
    CloseWindow(); 

    return 0;
}
