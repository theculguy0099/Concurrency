#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <unistd.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_WHITE "\x1b[37m"
#define ANSI_COLOR_ORANGE "\x1b[38;5;208m"

#define MAX_MACHINES 10
#define MAX_FLAVOURS 10
#define MAX_TOPPINGS 10
#define MAX_CUSTOMERS 100
#define MAX_ORDERS 100
#define MAX_NO_OF_TOPPINGS 100 

volatile int ticks = 0;

typedef struct
{
    int id;
    int in_use;
    int tm_start;
    int tm_stop;
} Machine; 

typedef struct
{
    char name[50];
    int prepation_time;
} Flavor; 

typedef struct
{
    char name[50];
    int quantity;
} Topping;

typedef struct
{
    int id;
    int customer_id;
    char flavor_name[50];
    int time;
    int num_toppings;
    int time_of_placing_order;
    char toppings[MAX_NO_OF_TOPPINGS][50];
    int whether_taken;
} Order;

typedef struct
{
    int id;
    int arrival_time;
    int num_orders;
    Order ice_creams[MAX_ORDERS];
    int no_of_completed_orders;
    int flag_for_accepted;
    int flag_for_ingridient_exhausted;
} Customer;

Machine machines[MAX_MACHINES];
Flavor flavors[MAX_FLAVOURS];
Topping toppings[MAX_TOPPINGS];
Customer customers[MAX_CUSTOMERS];

int N, K, F, T;

sem_t current_customers_in_parlor;
sem_t total_orders;
pthread_mutex_t timer_mutex;
pthread_mutex_t customer_mutex;
pthread_mutex_t machine_mutex;

int customer_count = 0;
int parlour_closing_time;

void *timer_thread(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&timer_mutex);
        sleep(1);
        ticks++;
        pthread_mutex_unlock(&timer_mutex);
    }
    pthread_exit(NULL);
}

void *machine_thread(void *arg)
{
    int machine_id = *(int *)arg;
    int start_time = machines[machine_id - 1].tm_start;
    int stop_time = machines[machine_id - 1].tm_stop;

    while (ticks != start_time)
    {
        ;
    }
    printf(ANSI_COLOR_ORANGE "Machine %d has started working at %d second(s)\n" ANSI_COLOR_RESET, machine_id, ticks);
    while (ticks != stop_time)
    {
        if (!(machine_id == 1 && machines[0].in_use == 0 || (machines[machine_id - 1].in_use == 0 && machines[machine_id - 2].in_use == 1)))
        {
            continue;
        }
        if (sem_trywait(&total_orders) == 0)
        {
            pthread_mutex_lock(&machine_mutex);
            machines[machine_id - 1].in_use = 1;
            Order *my_order = NULL;
            for (int i = 0; i < customer_count; i++)
            {
                if ((customers[0].flag_for_accepted == 1 && customers[0].no_of_completed_orders != customers[0].num_orders) || (customers[i - 1].flag_for_accepted == 1 && customers[i].flag_for_accepted == 1 && customers[i].no_of_completed_orders != customers[i].num_orders))
                {
                    int f = 0;
                    for (int j = 0; j < customers[i].num_orders; j++)
                    {
                        if (customers[i].ice_creams[j].whether_taken == 1)
                        {
                            continue;
                        }
                        for (int k = 0; k < customers[i].ice_creams[j].num_toppings; k++)
                        {
                            for (int l = 0; l < T; l++)
                            {
                                if (strcmp(customers[i].ice_creams[j].toppings[k], toppings[l].name) == 0)
                                {
                                    if (toppings[l].quantity <= 0)
                                    {
                                        customers[i].flag_for_ingridient_exhausted = 1;
                                        customers[i].flag_for_accepted = 0;
                                        break;
                                    }
                                }
                            }
                            if (customers[i].flag_for_ingridient_exhausted == 1)
                            {
                                break;
                            }
                        }
                        if (customers[i].flag_for_ingridient_exhausted == 1)
                        {
                            break;
                        }
                        else if (ticks + customers[i].ice_creams[j].time <= stop_time)
                        {
                            my_order = &customers[i].ice_creams[j];
                            my_order->time_of_placing_order = ticks;
                            my_order->whether_taken = 1;
                            f = 1;
                            break;
                        }
                    }
                    if (f == 1)
                    {
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&machine_mutex);
            if (my_order == NULL)
            {
                sem_post(&total_orders);
                continue;
            }
            pthread_mutex_lock(&machine_mutex);
            for (int i = 0; i < my_order->num_toppings; i++)
            {
                for (int j = 0; j < T; j++)
                {
                    if (strcmp(my_order->toppings[i], toppings[j].name) == 0)
                    {
                        toppings[j].quantity--;
                    }
                }
            }
            printf(ANSI_COLOR_CYAN "Machine %d starts preparing ice cream %d of customer %d at %d second(s)\n" ANSI_COLOR_RESET, machine_id, my_order->id, my_order->customer_id, ticks);
            pthread_mutex_unlock(&machine_mutex);
            while (ticks < my_order->time_of_placing_order + my_order->time)
            {
                ;
            }
            pthread_mutex_lock(&machine_mutex);
            printf(ANSI_COLOR_BLUE "Machine %d completes preparing ice cream %d of customer %d at %d second(s)\n" ANSI_COLOR_RESET, machine_id, my_order->id, my_order->customer_id, ticks);
            customers[my_order->customer_id - 1].no_of_completed_orders++;
            machines[machine_id - 1].in_use = 0;
            pthread_mutex_unlock(&machine_mutex);
        }
    }
    printf(ANSI_COLOR_ORANGE "Machine %d has stopped working at %d second(s)\n" ANSI_COLOR_RESET, machine_id, ticks);
    pthread_exit(NULL);
}

void *customer_thread(void *arg)
{
    int customer_id = *(int *)arg;
    int arrival_time = customers[customer_id - 1].arrival_time;

    int num = customers[customer_id - 1].num_orders;
    if (sem_trywait(&current_customers_in_parlor) != 0)
    {
        pthread_exit(NULL);
    }
    else
    {
        customers[customer_id - 1].flag_for_accepted = 1;
    }
    pthread_mutex_lock(&customer_mutex);
    printf(ANSI_COLOR_WHITE "Customer %d enters at %d second(s)\n" ANSI_COLOR_RESET, customer_id, ticks);
    printf(ANSI_COLOR_YELLOW "Customer %d orders %d ice cream(s)\n" ANSI_COLOR_RESET, customer_id, num);
    int check_arr[T];
    for (int i = 0; i < T; i++)
    {
        check_arr[i] = 0;
    }

    for (int i = 0; i < num; i++)
    {
        printf(ANSI_COLOR_YELLOW "Ice Cream %d: %s " ANSI_COLOR_RESET, customers[customer_id - 1].ice_creams[i].id, customers[customer_id - 1].ice_creams[i].flavor_name);
        for (int j = 0; j < customers[customer_id - 1].ice_creams[i].num_toppings; j++)
        {
            printf(ANSI_COLOR_YELLOW "%s " ANSI_COLOR_RESET, customers[customer_id - 1].ice_creams[i].toppings[j]);
            for (int k = 0; k < T; k++)
            {
                if (strcmp(customers[customer_id - 1].ice_creams[i].toppings[j], toppings[k].name) == 0)
                {

                    check_arr[k]++;
                }
            }
        }
        printf("\n");
    }
    for (int i = 0; i < T; i++)
    {
        if (check_arr[i] > toppings[i].quantity)
        {
            customers[customer_id - 1].flag_for_ingridient_exhausted = 1;
            break;
        }
    }

    pthread_mutex_unlock(&customer_mutex);
    if (customers[customer_id - 1].flag_for_ingridient_exhausted == 1)
    {
        pthread_mutex_lock(&customer_mutex);
        printf(ANSI_COLOR_RED "Customer %d left at %d second(s) with an unfulfilled order\n" ANSI_COLOR_RESET, customer_id, ticks);
        customers[customer_id - 1].flag_for_accepted = 0;
        pthread_mutex_unlock(&customer_mutex);

        sleep(1);

        pthread_mutex_lock(&customer_mutex);
        sem_post(&current_customers_in_parlor);
        pthread_mutex_unlock(&customer_mutex);
        pthread_exit(NULL);
    }
    int current_ticks = ticks;
    while (ticks != current_ticks + 1)
    {
        ;
    }
    for (int o = 0; o < num; o++)
    {
        pthread_mutex_lock(&customer_mutex);
        sem_post(&total_orders);
        pthread_mutex_unlock(&customer_mutex);
    }

    while (customers[customer_id - 1].no_of_completed_orders != customers[customer_id - 1].num_orders)
    {
        if (customers[customer_id - 1].flag_for_ingridient_exhausted == 1)
        {
            pthread_mutex_lock(&customer_mutex);
            printf(ANSI_COLOR_RED " Customer %d left at %d second(s) with an unfulfilled order\n" ANSI_COLOR_RESET, customer_id, ticks);
            customers[customer_id - 1].flag_for_accepted = 0;
            pthread_mutex_unlock(&customer_mutex);

            sleep(1);
            pthread_mutex_lock(&customer_mutex);
            sem_post(&current_customers_in_parlor);
            pthread_mutex_unlock(&customer_mutex);

            for (int i = 0; i < customers[customer_id - 1].no_of_completed_orders - customers[customer_id - 1].num_orders; i++)
            {
                pthread_mutex_lock(&customer_mutex);
                sem_trywait(&total_orders);
                pthread_mutex_unlock(&customer_mutex);
            }
            pthread_exit(NULL);
        }
        if (ticks > parlour_closing_time)
        {
            pthread_exit(NULL);
        }
    }
    pthread_mutex_lock(&customer_mutex);
    printf(ANSI_COLOR_GREEN "Customer %d has collected their order(s) and left at %d second(s)\n" ANSI_COLOR_RESET, customer_id, ticks);
    sem_post(&current_customers_in_parlor);
    pthread_mutex_unlock(&customer_mutex);

    pthread_exit(NULL);
}
int main()
{
    scanf("%d %d %d %d", &N, &K, &F, &T);
    int max = -1;
    for (int i = 0; i < N; i++)
    {
        scanf("%d %d", &machines[i].tm_start, &machines[i].tm_stop);
        if (max <= machines[i].tm_stop)
        {
            max = machines[i].tm_stop;
        }
    }
    parlour_closing_time = max;

    for (int i = 0; i < F; i++)
    {
        scanf("%s %d", flavors[i].name, &flavors[i].prepation_time);
    }
    for (int i = 0; i < T; i++)
    {
        scanf("%s %d", toppings[i].name, &toppings[i].quantity);
        if (toppings[i].quantity == -1)
        {
            toppings[i].quantity = 1e9;
        }
    }
    while (1)
    {
        int c, t_arr, num_orders;
        char input1[15];
        fgets(input1, sizeof(input1), stdin);
        if (input1[0] == '\n')
        {
            fgets(input1, sizeof(input1), stdin);
            if (input1[0] == '\n')
            {
                break;
            }
        }
        char *t1 = strtok(input1, " \n");
        c = atoi(t1);
        t1 = strtok(NULL, " \n");
        t_arr = atoi(t1);
        t1 = strtok(NULL, " \n");
        num_orders = atoi(t1);

        customers[customer_count].arrival_time = t_arr;
        customers[customer_count].num_orders = num_orders;
        customers[customer_count].flag_for_accepted = -1;
        customers[customer_count].flag_for_ingridient_exhausted = -1;
        customers[customer_count].no_of_completed_orders = 0;

        for (int i = 0; i < num_orders; i++)
        {
            char input[500];
            fgets(input, sizeof(input), stdin);
            char *t = strtok(input, " \n");
            strcpy(customers[customer_count].ice_creams[i].flavor_name, t);
            for (int k = 0; k < F; k++)
            {
                if (strcmp(customers[customer_count].ice_creams[i].flavor_name, flavors[k].name) == 0)
                {
                    customers[customer_count].ice_creams[i].time = flavors[k].prepation_time;
                }
            }
            t = strtok(NULL, " \n");
            customers[customer_count].ice_creams[i].num_toppings = 0;
            customers[customer_count].ice_creams[i].id = i + 1;
            customers[customer_count].ice_creams[i].whether_taken = -1;
            customers[customer_count].ice_creams[i].customer_id = customer_count + 1;

            while (t != NULL)
            {
                strcpy(customers[customer_count].ice_creams[i].toppings[customers[customer_count].ice_creams[i].num_toppings++], t);
                t = strtok(NULL, " \n");
            }
        }
        customer_count++;
    }
    sem_init(&current_customers_in_parlor, 0, K);
    sem_init(&total_orders, 0, 0);
    pthread_mutex_init(&timer_mutex, NULL);
    pthread_mutex_init(&customer_mutex, NULL);
    pthread_mutex_init(&machine_mutex, NULL);

    pthread_t timer;
    pthread_create(&timer, NULL, timer_thread, NULL);

    pthread_t machines_threads[N];
    for (int i = 0; i < N; i++)
    {
        machines[i].id = i + 1;
        machines[i].in_use = 0;
        pthread_create(&machines_threads[i], NULL, machine_thread, &machines[i].id);
    }
    pthread_t customers_threads[customer_count];

    for (int i = 0; i < customer_count; i++)
    {
        customers[i].id = i + 1;
        while (ticks != customers[i].arrival_time)
        {
            ;
        }
        pthread_create(&customers_threads[i], NULL, customer_thread, &customers[i].id);
        usleep(9000 * customers[i].id);
    }
    for (int i = 0; i < N; i++)
    {
        pthread_join(machines_threads[i], NULL);
    }
    for (int i = 0; i < customer_count; i++)
    {
        if (customers[i].flag_for_accepted == 1 && customers[i].no_of_completed_orders != customers[i].num_orders)
        {
            printf(ANSI_COLOR_RED "Customer %d was not seviced due to unavailability of machines\n" ANSI_COLOR_RESET, customers[i].id);
        }
    }
    for (int i = 0; i < customer_count; i++)
    {
        pthread_join(customers_threads[i], NULL);
    }
    pthread_cancel(timer);
    pthread_join(timer, NULL);
    printf("Parlour Closed\n");
    return 0;
}
