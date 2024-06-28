#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>

#define COLOR_RESET "\033[0m"
#define COLOR_WHITE "\033[0;97m"
#define COLOR_YELLOW "\033[0;93m"
#define COLOR_CYAN "\033[0;96m"
#define COLOR_BLUE "\033[0;94m"
#define COLOR_GREEN "\033[0;92m"
#define COLOR_RED "\033[0;91m"

#define MAX_CUSTOMERS 100
#define MAX_COFFEE_TYPES 10
#define MAX_NO_OF_BRISTAS 100

volatile int ticks = 0;
volatile int coffee_wasted = 0;

struct Coffee
{
    char name[50];
    int preparation_time;
};

struct Customer
{
    int id;
    int coffee_type;
    int arrival_time;
    int tolerance_time;
    int flag_of_placing_order;
    int whether_barista_assigned;
    int ticks_of_order_placing;
    int barista_id;
};

int num_baristas, num_coffee_types, num_customers;
struct Coffee coffee_types[MAX_COFFEE_TYPES];
struct Customer customers[MAX_CUSTOMERS];
int baristas[MAX_NO_OF_BRISTAS];

sem_t orders;
sem_t barista_sem;
sem_t whether_order_completed[MAX_CUSTOMERS];
pthread_mutex_t customer_mutex;
pthread_mutex_t barista_mutex;

pthread_mutex_t timer_mutex;
void *timer_thread(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&timer_mutex);
        sleep(1); // Sleep for 1 second
        ticks++;  // Increment the tick
        pthread_mutex_unlock(&timer_mutex);
    }
    pthread_exit(NULL);
}
int wait_time = 0;
int temp;
int completed_orders = 0;
void *barista_thread(void *arg)
{
    while (1)
    {
        sem_wait(&orders);

        for (int i = 0; i < num_customers; i++)
        {
            if ((customers[0].flag_of_placing_order == 1 && customers[0].whether_barista_assigned == -1) || (customers[i].flag_of_placing_order == 1 && customers[i - 1].flag_of_placing_order == 1 && customers[i].whether_barista_assigned == -1 && customers[i - 1].whether_barista_assigned == 1))
            {

                for (int j = 0; j < num_baristas; j++)
                {
                    if (baristas[j] == -1 && (customers[0].whether_barista_assigned == -1 || (customers[i].whether_barista_assigned == -1 && customers[i - 1].whether_barista_assigned == 1)))
                    {
                        pthread_mutex_lock(&barista_mutex);
                        customers[i].barista_id = j + 1;
                        customers[i].whether_barista_assigned = 1;
                        baristas[j] = 1;
                        pthread_mutex_unlock(&barista_mutex);
                        break;
                    }
                }

                int current_ticks = ticks;
                while (ticks != current_ticks + 1)
                    ;

                pthread_mutex_lock(&barista_mutex);
                wait_time += ticks - customers[i].arrival_time;
                printf(COLOR_CYAN "Barista %d begins preparing the order of customer %d at %d second(s)\n" COLOR_RESET, customers[i].barista_id, customers[i].id, ticks);
                pthread_mutex_unlock(&barista_mutex);

                int coffee_type = customers[i].coffee_type;
                int preparation_time = coffee_types[coffee_type].preparation_time;

                while (ticks <= customers[i].ticks_of_order_placing + preparation_time)
                    ;

                pthread_mutex_lock(&barista_mutex);
                baristas[customers[i].barista_id - 1] = -1;
                printf(COLOR_BLUE "Barista %d completes the order of customer %d at %d second(s)\n" COLOR_RESET, customers[i].barista_id, customers[i].id, ticks);
                completed_orders++;
                sem_post(&whether_order_completed[i]);
                sem_post(&barista_sem);
                pthread_mutex_unlock(&barista_mutex);
                break;
            }
        }
    }
    pthread_exit(NULL);
}

void *customer_thread(void *arg)
{
    int customer_id = *((int *)arg);
    int arrival_time = customers[customer_id - 1].arrival_time;
    int tolerance_time = customers[customer_id - 1].tolerance_time;

    while (ticks != arrival_time)
        ;

    pthread_mutex_lock(&customer_mutex);
    printf(COLOR_WHITE "Customer %d arrives at %d second(s)\n" COLOR_RESET, customer_id, arrival_time);
    printf(COLOR_YELLOW "Customer %d orders a %s\n" COLOR_RESET, customer_id, coffee_types[customers[customer_id - 1].coffee_type].name);
    int flag = 0;
    pthread_mutex_unlock(&customer_mutex);

    while (ticks < tolerance_time + arrival_time + 1)
    {
        if ((customer_id == 1 && customers[0].flag_of_placing_order == -1) || (customers[customer_id - 1].flag_of_placing_order == -1 && customers[customer_id - 2].flag_of_placing_order == 1))
        {
            if (sem_trywait(&barista_sem) == 0)
            {
                pthread_mutex_lock(&customer_mutex);
                customers[customer_id - 1].flag_of_placing_order = 1;
                customers[customer_id - 1].ticks_of_order_placing = ticks;

                sem_post(&orders);
                pthread_mutex_unlock(&customer_mutex);
                break;
            }
        }
    }

    int coffee_type = customers[customer_id - 1].coffee_type;
    int preparation_time = coffee_types[coffee_type].preparation_time;

    if ((customers[customer_id - 1].ticks_of_order_placing + preparation_time < arrival_time + tolerance_time) && customers[customer_id - 1].flag_of_placing_order == 1)
    {
        while (ticks <= customers[customer_id - 1].ticks_of_order_placing + preparation_time)
            ;

        pthread_mutex_lock(&customer_mutex);
        flag = 1;
        sem_wait(&whether_order_completed[customer_id - 1]);
        printf(COLOR_GREEN "Customer %d leaves with their order at %d second(s)\n" COLOR_RESET, customer_id, ticks);
        pthread_mutex_unlock(&customer_mutex);
    }

    if (flag == 0)
    {
        while (ticks <= arrival_time + tolerance_time)
            ;

        pthread_mutex_lock(&customer_mutex);
        if (customers[customer_id - 1].flag_of_placing_order == 1)
        {
            coffee_wasted++;
        }
        else
        {
            temp--;
            wait_time += ticks - arrival_time;
        }
        customers[customer_id - 1].flag_of_placing_order = 1;
        customers[customer_id - 1].whether_barista_assigned = 1;
        printf(COLOR_RED "Customer %d leaves without their order at %d second(s)\n" COLOR_RESET, customer_id, ticks);
        pthread_mutex_unlock(&customer_mutex);
    }

    pthread_exit(NULL);
}

int main()
{

    scanf("%d %d %d", &num_baristas, &num_coffee_types, &num_customers);
    for (int i = 0; i < num_coffee_types; i++)
    {
        scanf("%s %d", coffee_types[i].name, &coffee_types[i].preparation_time);
    }
    for (int i = 0; i < num_baristas; i++)
    {
        baristas[i] = -1;
    }

    for (int i = 0; i < num_customers; i++)
    {
        customers[i].id = i + 1;
        customers[i].flag_of_placing_order = -1;
        customers[i].whether_barista_assigned = -1;
        customers[i].ticks_of_order_placing = 0;

        char coffee_name[50];
        scanf("%d %s %d %d", &customers[i].coffee_type, coffee_name, &customers[i].arrival_time, &customers[i].tolerance_time);
        for (int j = 0; j < num_coffee_types; j++)
        {
            if (strcmp(coffee_name, coffee_types[j].name) == 0)
            {
                customers[i].coffee_type = j;
                break;
            }
        }
        sem_init(&whether_order_completed[i], 0, 0);
    }

    temp = num_customers;
    sem_init(&orders, 0, 0);
    sem_init(&barista_sem, 0, num_baristas);
    pthread_mutex_init(&customer_mutex, NULL);
    pthread_mutex_init(&timer_mutex, NULL);
    pthread_mutex_init(&barista_mutex, NULL);

    pthread_t timer;
    pthread_create(&timer, NULL, timer_thread, NULL);

    pthread_t customer_threads[num_customers];

    for (int i = 0; i < num_customers; i++)
    {
        pthread_create(&customer_threads[i], NULL, customer_thread, &customers[i].id);
    }

    pthread_t barista_threads[num_baristas];
    for (int i = 0; i < num_baristas; i++)
    {
        pthread_create(&barista_threads[i], NULL, barista_thread, NULL);
    }

    for (int i = 0; i < num_customers; i++)
    {
        pthread_join(customer_threads[i], NULL);
    }

    while (temp != completed_orders)
        ;

    for (int i = 0; i < num_baristas; i++)
    {
        pthread_cancel(barista_threads[i]);
        pthread_join(barista_threads[i], NULL);
    }

    sem_destroy(&orders);
    sem_destroy(&barista_sem);
    pthread_mutex_destroy(&customer_mutex);
    pthread_mutex_destroy(&timer_mutex);
    pthread_mutex_destroy(&barista_mutex);

    pthread_cancel(timer);
    pthread_join(timer, NULL);
    
    printf("\n\n%d coffee(s) wasted\n", coffee_wasted);
    printf("Average waiting time: %f\n", (float)wait_time / num_customers);
    return 0;
}