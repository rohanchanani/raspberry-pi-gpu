#include "vector-multiply.h"
#include "parallel-add.h"

#define WIDTH 64
#define HEIGHT 15
#define VEC_WIDTH 64

#define DECAY_FACTOR 100

unsigned int heat[WIDTH * HEIGHT];
volatile struct mulGPU *mul_gpu;
volatile struct addGPU *add_gpu;

void fire_init(void)
{
    int i;

    for (i = 0; i < WIDTH; i++)
    {
        heat[(HEIGHT - 1) * WIDTH + i] = (timer_get_usec() % 2000);
    }


    for (i = 0; i < (HEIGHT - 1) * WIDTH; i++)
    {
        heat[i] = 0;
    }

    vec_mul_init(&mul_gpu, VEC_WIDTH);
    vec_add_init(&add_gpu);
}

void update_fire_row(int row)
{
    int col;

    for (col = 0; col < WIDTH; col += VEC_WIDTH)
    {
        int actual_width = (col + VEC_WIDTH > WIDTH) ? (WIDTH - col) : VEC_WIDTH;
        
        for (int j = 0; j < actual_width; j++) {
            int i = row * WIDTH + col + j;
            int below = (row + 1) * WIDTH + col + j;
            
            int below_left = (col + j > 0) ? below - 1 : below;
            int below_right = (col + j < WIDTH - 1) ? below + 1 : below;
            
            add_gpu->A[j] = heat[below];
            add_gpu->B[j] = (heat[below_left] + heat[below_right]) / 2;
        }
        

        vec_add_exec(add_gpu);
        

        for (int j = 0; j < actual_width; j++) {
            mul_gpu->A[j] = add_gpu->C[j] / 3;
            mul_gpu->B[j] = DECAY_FACTOR;
        }
        

        vec_mul_exec(mul_gpu);
        
        for (int j = 0; j < actual_width; j++) {
            if (col + j < WIDTH) {
                unsigned int new_val = mul_gpu->C[j] / 100;
                
                if (timer_get_usec() % 7 == 0) {
                    new_val += (timer_get_usec() % 100);
                }
                
                heat[row * WIDTH + col + j] = new_val;
            }
        }
    }
}

void update_fire(void)
{
    for (int row = HEIGHT - 2; row >= 0; row--)
    {
        update_fire_row(row);
    }


    for (int i = 0; i < WIDTH; i++)
    {

        int variation = timer_get_usec() % 500;
        heat[(HEIGHT - 1) * WIDTH + i] = 1800 + variation;
    }
}

void display_fire(void)
{
    int x, y;

    char buffer[(HEIGHT) * (WIDTH + 1) + 8]; // +8 for initial newlines
    int pos = 0;
    
    for (int i = 0; i < sizeof(buffer); i++) {
        buffer[i] = '\0';
    }

    buffer[pos++] = '\f';
    buffer[pos++] = '\n';
    buffer[pos++] = '\n';
    buffer[pos++] = '\n';
    buffer[pos++] = '\n';
    buffer[pos++] = '\n';
    buffer[pos++] = '\n';
    buffer[pos++] = '\n';
    
    for (y = 0; y < HEIGHT; y += 2) {
        for (x = 0; x < WIDTH; x += 2) {
            int value = heat[y * WIDTH + x];
            char c = ' ';
            if (value > 1000) c = '#';
            else if (value > 700) c = '*';
            else if (value > 400) c = '+';
            else if (value > 100) c = '.';
            
            buffer[pos++] = c;
        }
        buffer[pos++] = '\n';
    }
    
    printk("%s", buffer);
}

void fire_animation(void)
{
    int frame;

    fire_init();

    for (frame = 0; frame < 300; frame++)
    {
        update_fire();
        display_fire();
        delay_ms(10);
    }

    vec_mul_release(mul_gpu);
    vec_add_release(add_gpu);
}

void notmain(void)
{
    printk("Starting GPU Fire Animation\n");
    fire_animation();
    printk("Animation complete!\n");
}