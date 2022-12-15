# CHEF
> struct with chefs info.

# PIZZA
> struct with pizza info

# INGREDIENT
> struct with ingredient info

# CUSTOMER
> struct with cutomer info

# ORDER
> struct with order info

# PICKED
> struct with info about pizza's which are yet to be picked

# CHECK_PIZZA
> checks if the pizza can be made with given ingredients

# OVEN 
> Semaphore which stores the numbers of ovens.

# CUSTOMER THREAD
> waits till the customer arrives then pushes the order of the customer into order vector if the order can be prepared or else rejects and returns. Then it waits for customer to get to the drive thru. After customer gets to the drive thru all the pizzas which were not picked up but made are picked.

# CHEF THREAD
> waits for chefs to arrive. After the chef arrives the thread finds a pizza which the chef can make. That pizza is then removed from list of pizzas to be made. Decrease quantity of ingredients to be used. Then keeps it in the oven using a wait on the semaphore. Waits for it to complete and then puts it in list of completed pizzas. In case a pizza can't be made. If none of the pizzas can be made, the size of the list of tofinish pizzas is 0, this condition is checked to reject a customer after he is accepted. Checks if all the pizzas of customers are made and picked up,if yes then customers exits or else stays.
---
# ADDITIONAL QUESTIONS:
1. If pick up spot has a limit: We would then make a sempahore for the pick up spot. After being made. we spawn a thread for the pizza which waits for the pick up spot. In case number of pick up spots fall short, we can easily have pizzas wait for them now.
2. In order to keep the order complete, we might preallocate the ingredients to customer's pizzas as they come. In case any pizza can't be made we reject the customer then, as it won't affect the rating. There is still a chance of incomplete order in case of chef not being available but in all rating will improve.
3. If a ingredient can be relinquished, we don't reject the customer based on this ingredient, but we wait for it to be available.
---
# Assumption
It takes 3 second to add toppings and then oven time is the same as given in the input and not t-3.