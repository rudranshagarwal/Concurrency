#include <bits/stdc++.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

using namespace std;

#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

struct chef
{
    int chef_id;
    int t1;
    int t2;
};

struct pizza
{
    int pizza_id;
    int time;
    int ningredients;
    vector<int> special_ingredients;
    int pizza_made;
};

struct ingredient
{
    int ingredient_id;
    int amount;
};

struct customer
{
    int customer_id;
    int entry_time;
    int number_of_pizzas;
    vector<int> pizzas;
};

struct order
{
    int order_number;
    int customer;
    vector<int> pizzas;
    vector<int> finished;
    vector<int> tofinish;
};

struct picked
{
    int customer;
    int pizza;
    int order;
};

struct chef **chefs;
struct pizza **pizzas;
struct ingredient **ingredients;
struct customer **customers;
vector<struct order> orders;
int ordernumber = 1;

int *orderdone;
int *pizza_made;

int starttime;
int nchefsleft;
int ncustomerscame;
int *ovens;
int *reacheddrivethru;
int * left;

vector<struct picked> tobepicked;

pthread_mutex_t ingredient_lock;
pthread_mutex_t order_lock;
pthread_mutex_t pizza_made_lock;
pthread_mutex_t orderdone_lock;
pthread_mutex_t ordernumber_lock;
pthread_mutex_t nchefsleft_lock;
pthread_mutex_t ncustomerscame_lock;
pthread_mutex_t ovens_lock;
pthread_mutex_t reacheddrivethru_lock;
pthread_mutex_t tobepicked_lock;
pthread_mutex_t left_lock;

sem_t *ovensem;
int nchefs, npizzas, ningredients, ncustomers, novens, npickup;

int check_pizza(struct pizza *currpizza)//check if pizza can be made
{
    for (int i = 0; i < currpizza->ningredients; i++)
    {
        pthread_mutex_lock(&ingredient_lock);
        if (ingredients[currpizza->special_ingredients[i] - 1]->amount <= 0)
        {
            pthread_mutex_unlock(&ingredient_lock);

            return 0;
        }
        pthread_mutex_unlock(&ingredient_lock);
    }

    return 1;
}

void *customerthread(void *arg)//execute customer thread
{
    int i = *(int *)arg;
    sleep(customers[i]->entry_time);//wait till customer comes
    printf(BYEL "Customer %d arrives at time %d\n" ANSI_RESET, customers[i]->customer_id, customers[i]->entry_time);
    struct order ordered;
    pthread_mutex_lock(&ordernumber_lock);
    ordered.order_number = ordernumber;
    ordernumber++;
    pthread_mutex_unlock(&ordernumber_lock);
    pthread_mutex_lock(&ncustomerscame_lock);
    ncustomerscame++;
    pthread_mutex_unlock(&ncustomerscame_lock);
    ordered.customer = customers[i]->customer_id;
    ordered.pizzas = customers[i]->pizzas;
    ordered.tofinish = customers[i]->pizzas;
    pthread_mutex_lock(&order_lock);
    orders.push_back(ordered);
    pthread_mutex_unlock(&order_lock);
    printf(BRED "Order %d placed by customer %d has pizzas {" ANSI_RESET, ordered.order_number, customers[i]->customer_id);
    for (int j = 0; j < customers[i]->pizzas.size(); j++)
    {
        if (j != customers[i]->pizzas.size() - 1)
            printf(BRED "%d," ANSI_RESET, customers[i]->pizzas[j]);
        else
            printf(BRED "%d}\n" ANSI_RESET, customers[i]->pizzas[j]);
    }
    pthread_mutex_lock(&orderdone_lock);
    orderdone[ordered.order_number] = 0;
    pthread_mutex_unlock(&orderdone_lock);
    int flag = 0;
    for (int k = 0; k < customers[i]->pizzas.size(); k++)//chec if pizzas can be made if no reject
    {
        int l;
        for (l = 0; l < npizzas; l++)
        {
            if (customers[i]->pizzas[k] == pizzas[l]->pizza_id)
                break;
        }
        if (check_pizza(pizzas[l]))
            flag = 1;
    }
    pthread_mutex_lock(&nchefsleft_lock);
    if (flag && nchefsleft > 0)//accpet the order
    {
        pthread_mutex_unlock(&nchefsleft_lock);

        printf(BRED "Order %d placed by customer %d awaits processing\n" ANSI_RESET, ordered.order_number, customers[i]->customer_id);

        sleep(npickup);

        pthread_mutex_lock(&reacheddrivethru_lock);
        reacheddrivethru[i] = 1;
        pthread_mutex_unlock(&reacheddrivethru_lock);
        pthread_mutex_lock(&tobepicked_lock);
        vector<int> removed;
        if (tobepicked.size() > 0)
        {
            for (int x = 0; x < tobepicked.size(); x++)
            {
                if (tobepicked[x].customer == customers[i]->customer_id)
                {
                    printf(BYEL "Customer %d picks up their pizza %d\n" ANSI_RESET, tobepicked[x].customer, tobepicked[x].pizza);
                    removed.push_back(x);
                }
            }
            for (int x = 0; x < removed.size(); x++)
            {
                tobepicked.erase(tobepicked.begin() + removed[x]);
            }
        }
        pthread_mutex_unlock(&tobepicked_lock);
    }
    else
    {
        pthread_mutex_unlock(&nchefsleft_lock);

        printf(BYEL "Customer %d rejected\n" ANSI_RESET, customers[i]->customer_id);
        printf(BYEL "Customer %d exits the drive-thru zone\n" ANSI_RESET, customers[i]->customer_id);
    }

    return NULL;
}

void *chefthread(void *arg) //execute chef
{

    int i = *(int *)arg;
    sleep(chefs[i]->t1);//wait for arrival time
    printf(BBLU "Chef %d arrives at time %d\n" ANSI_RESET, chefs[i]->chef_id, chefs[i]->t1);
    while (time(NULL) - starttime <= chefs[i]->t2)//execute till end time
    {
        pthread_mutex_lock(&order_lock);//get the pizza which can be made
        int pizzanumber = -1;
        int ordernumber;
        for (int j = 0; j < orders.size(); j++)
        {
            for (int k = 0; k < orders[j].pizzas.size(); k++)
            {
                for (int l = 0; l < npizzas; l++)
                {
                    // printf("Searching %d\n",orders[j].pizzas[k] );
                    if (orders[j].pizzas[k] == pizzas[l]->pizza_id)
                    {

                        pizzanumber = l;
                        break;
                    }
                }
                if (!check_pizza(pizzas[pizzanumber]))//check if pizza can be made
                {
                    // printf("No: %d\n",pizzas[pizzanumber]->pizza_id);
                    orders[j].pizzas.erase(orders[j].pizzas.begin() + k);
                    for (int l = 0; l < orders[j].tofinish.size(); l++)
                    {
                        if (pizzas[pizzanumber]->pizza_id == orders[j].tofinish[l])
                        {
                            orders[j].tofinish.erase(orders[j].tofinish.begin() + l);
                            if (orders[j].tofinish.size() == 0)
                            {
                                printf(BYEL "Customer %d rejected\n" ANSI_RESET, orders[ordernumber].customer);
                                printf(BYEL "Customer %d exits the drive-thru zone\n" ANSI_RESET, orders[ordernumber].customer);
                            }
                            break;
                        }
                    }
                    pizzanumber = -1;
                }
                else if (time(NULL) - starttime + pizzas[pizzanumber]->time > chefs[i]->t2)//check if pizza can be made before chef exits
                {
                    pizzanumber = -1;
                    continue;
                }
                else
                {
                    ordernumber = j;
                    orders[j].pizzas.erase(orders[j].pizzas.begin() + k);//make pizza
                    break;
                }
            }
            if (pizzanumber != -1)
                break;
        }
        pthread_mutex_unlock(&order_lock);
        if (pizzanumber != -1)
        {
            printf(BBLU "Pizza %d in order %d assigned to chef %d\n" ANSI_RESET, pizzas[pizzanumber]->pizza_id, orders[ordernumber].order_number, chefs[i]->chef_id);
            pthread_mutex_lock(&orderdone_lock);
            if (!orderdone[ordernumber])
                printf(BRED "Order %d placed by customer %d is being processed\n" ANSI_RESET, orders[ordernumber].order_number, orders[ordernumber].customer);
            orderdone[ordernumber] = 1;
            pthread_mutex_unlock(&orderdone_lock);
            printf(BBLU "Chef %d is preparing the pizza %d from order %d\n" ANSI_RESET, chefs[i]->chef_id, pizzas[pizzanumber]->pizza_id, orders[ordernumber].order_number);
            pthread_mutex_lock(&ingredient_lock);
            for (int x = 0; x < pizzas[pizzanumber]->ningredients; x++)//use ingredients
            {
                ingredients[pizzas[pizzanumber]->special_ingredients[x] - 1]->amount--;
            }
            // for (int x = 0; x < npizzas; x++)
            // {
            //     pthread_mutex_lock(&pizza_made_lock);
            //     pizzas[x]->pizza_made = check_pizza(pizzas[x]);
            //     pthread_mutex_unlock(&pizza_made_lock);
            // }
            pthread_mutex_unlock(&ingredient_lock);
            sleep(3);
            sem_wait(ovensem);//wait for oven
            int ovennumber;
            pthread_mutex_lock(&ovens_lock);
            for (int i = 0; i < novens; i++)//get oven number
            {
                if (ovens[i] == 0)
                {
                    ovennumber = i + 1;
                    ovens[i] = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&ovens_lock);
            printf(BBLU "Chef %d has put the pizza %d for order %d in oven %d at time %ld\n" ANSI_RESET, chefs[i]->chef_id, pizzas[pizzanumber]->pizza_id, orders[ordernumber].order_number, ovennumber, time(NULL) - starttime);
            sleep(pizzas[pizzanumber]->time);
            time_t currtime = time(NULL) - starttime;

            printf(BBLU "Chef %d has picked up the pizza %d for order %d from the oven %d at time %ld\n" ANSI_RESET, chefs[i]->chef_id, pizzas[pizzanumber]->pizza_id, orders[ordernumber].order_number, ovennumber, time(NULL) - starttime);
            pthread_mutex_lock(&reacheddrivethru_lock);
            int x;
            int reachedflag = 0;
            for (x = 0; x < ncustomers; x++)//check if customer is at drive thru
            {
                if (orders[ordernumber].customer == customers[x]->customer_id)
                    break;
            }
            if (reacheddrivethru[x] == 1)
            {
                printf(BYEL "Customer %d picks up their pizza %d\n" ANSI_RESET, orders[ordernumber].customer, pizzas[pizzanumber]->pizza_id);
                reachedflag = 1;
            }
            else
            {
                pthread_mutex_lock(&tobepicked_lock);
                struct picked currpicked;
                currpicked.customer = orders[ordernumber].customer;
                currpicked.pizza = pizzas[pizzanumber]->pizza_id;
                currpicked.order = orders[ordernumber].order_number;
                tobepicked.push_back(currpicked);
                pthread_mutex_unlock(&tobepicked_lock);
            }
            pthread_mutex_unlock(&reacheddrivethru_lock);
            orders[ordernumber].finished.push_back(pizzas[pizzanumber]->pizza_id);// orders yet to be picked
            if (orders[ordernumber].tofinish.size() == orders[ordernumber].finished.size() && reachedflag)
            {
                if (orders[ordernumber].tofinish.size() == 0)
                {
                    printf(BYEL "Customer %d rejected\n" ANSI_RESET, orders[ordernumber].customer);
                    printf(BYEL "Customer %d exits the drive-thru zone\n" ANSI_RESET, orders[ordernumber].customer);
                }
                else
                {
                    printf(BYEL "Customer %d exits the drive-thru zone.\n" ANSI_RESET, orders[ordernumber].customer);
                }
            }
            pthread_mutex_lock(&ovens_lock);
            ovens[ovennumber - 1] = 0;
            pthread_mutex_unlock(&ovens_lock);
            sem_post(ovensem);
        }
    }

    printf(BBLU "Chef %d exits at time %d\n" ANSI_RESET, chefs[i]->chef_id, chefs[i]->t2);
    pthread_mutex_lock(&nchefsleft_lock);
    pthread_mutex_lock(&ncustomerscame_lock);
    nchefsleft--;
    if (nchefsleft <= 0 && ncustomerscame >= ncustomers)// check if all chefs left
    {
        pthread_mutex_unlock(&nchefsleft_lock);
        pthread_mutex_unlock(&ncustomerscame_lock);
        while (1)
        {
            pthread_mutex_lock(&tobepicked_lock);//wait till all orders are picked
            if (tobepicked.size() == 0)
            {
                pthread_mutex_unlock(&tobepicked_lock);

                break;
            }
            pthread_mutex_unlock(&tobepicked_lock);
        }
        printf(BGRN "Simulation ended\n" ANSI_RESET);

        exit(0);
    }
    pthread_mutex_unlock(&nchefsleft_lock);
    pthread_mutex_unlock(&ncustomerscame_lock);

    return NULL;
}

int main()
{
    scanf("%d %d %d %d %d %d", &nchefs, &npizzas, &ningredients, &ncustomers, &novens, &npickup);
    chefs = (struct chef **)malloc(nchefs * sizeof(struct chef *));
    pizzas = (struct pizza **)malloc(npizzas * sizeof(struct pizza *));
    ingredients = (struct ingredient **)malloc(ningredients * sizeof(struct ingredient *));
    customers = (struct customer **)malloc(ncustomers * sizeof(struct customer *));
    orderdone = (int *)malloc(ncustomers + 1);
    pizza_made = (int *)malloc(npizzas);
    ovens = (int *)malloc(novens);
    reacheddrivethru = (int *)malloc(ncustomers);

    pthread_t customerthreads[ncustomers];
    pthread_t chefthreads[nchefs];

    pthread_mutex_init(&ingredient_lock, NULL);
    pthread_mutex_init(&order_lock, NULL);
    pthread_mutex_init(&pizza_made_lock, NULL);
    pthread_mutex_init(&orderdone_lock, NULL);
    pthread_mutex_init(&ordernumber_lock, NULL);
    pthread_mutex_init(&nchefsleft_lock, NULL);
    pthread_mutex_init(&ncustomerscame_lock, NULL);
    pthread_mutex_init(&ovens_lock, NULL);
    pthread_mutex_init(&reacheddrivethru_lock, NULL);
    pthread_mutex_init(&tobepicked_lock, NULL);
    pthread_mutex_init(&left_lock, NULL);
    nchefsleft = nchefs;
    ncustomerscame = 0;

    ovensem = (sem_t *)malloc(sizeof(sem_t));
    sem_init(ovensem, 0, novens);
    struct timespec universal;
    if (ovensem == SEM_FAILED)
    {
        printf("semaphore error\n");
        exit(1);
    }
    for (int i = 0; i < npizzas; i++)
    {
        pizzas[i] = (struct pizza *)malloc(sizeof(struct pizza));
        scanf("%d %d %d", &pizzas[i]->pizza_id, &pizzas[i]->time, &pizzas[i]->ningredients);
        for (int j = 0; j < pizzas[i]->ningredients; j++)
        {
            int x;
            scanf("%d", &x);
            pizzas[i]->special_ingredients.push_back(x);
        }
        // printf("%d %d %d\n", pizzas[i]->pizza_id, pizzas[i]->ningredients, pizzas[i]->time);
    }
    for (int i = 0; i < ningredients; i++)
    {
        ingredients[i] = (struct ingredient *)malloc(sizeof(struct ingredient));
        ingredients[i]->ingredient_id = i + 1;
        scanf("%d", &ingredients[i]->amount);
    }

    for (int i = 0; i < nchefs; i++)
    {
        chefs[i] = (struct chef *)malloc(sizeof(struct chef));
        chefs[i]->chef_id = i + 1;
        scanf("%d %d", &chefs[i]->t1, &chefs[i]->t2);
    }

    for (int i = 0; i < ncustomers; i++)
    {
        customers[i] = (struct customer *)malloc(sizeof(struct customer));
        customers[i]->customer_id = i + 1;
        scanf("%d %d", &customers[i]->entry_time, &customers[i]->number_of_pizzas);
        for (int j = 0; j < customers[i]->number_of_pizzas; j++)
        {
            int x;
            scanf("%d", &x);
            customers[i]->pizzas.push_back(x);
        }
    }

    for (int i = 0; i < npizzas; i++)
    {
        pthread_mutex_lock(&pizza_made_lock);
        pizzas[i]->pizza_made = check_pizza(pizzas[i]);
        pthread_mutex_unlock(&pizza_made_lock);
    }
    for (int i = 0; i < novens; i++)
    {
        ovens[i] = 0;
    }

    for (int i = 0; i < ncustomers; i++)
    {
        reacheddrivethru[i] = 0;
    }

    starttime = time(NULL);
    int j[ncustomers];
    printf(BGRN "Simulation started\n" ANSI_RESET);
    for (int i = 0; i < ncustomers; i++)
    {
        j[i] = i;
        pthread_create(&customerthreads[i], NULL, customerthread, (void *)&j[i]);
    }
    int k[nchefs];
    for (int i = 0; i < nchefs; i++)
    {
        k[i] = i;
        pthread_create(&chefthreads[i], NULL, chefthread, (void *)&k[i]);
    }
    for (int i = 0; i < ncustomers; i++)
    {
        pthread_join(customerthreads[i], NULL);
    }
    for (int i = 0; i < ncustomers; i++)
    {
        pthread_join(chefthreads[i], NULL);
    }
    sem_destroy(ovensem);
}