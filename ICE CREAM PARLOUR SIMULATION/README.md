# Ice Cream Parlour Simulation

## Implementation

This code is an implementation of an ice cream parlor simulation with multiple threads that represent machines, customers, and a timer. The simulation models the operation of an ice cream parlor where customers place orders for ice creams with various flavors and toppings, and machines prepare these orders. Here's an overview of the code's key components:

### Libraries and Definitions:

- The code begins by including standard C libraries like 'stdio.h', 'stdlib.h', 'unistd.h', 'string.h', 'pthread.h', and 'semaphore.h'. It also defines several color codes for terminal output.
- Constants like MAX_MACHINES, MAX_FLAVORS, MAX_CUSTOMERS, MAX_TOPPINGS, MAX_ORDERS and MAX_NO_OF_TOPPINGS are defined.

### Global Variables, Semaphores and Mutex Locks:

- Global variables are used to maintain various state information, such as the current simulation time (ticks).
- Arrays and variables are defined to store information about machines, customers, flavors, and toppings.
- Semaphores are used for managing total_orders and the capacity of the parlour.
- Locks are also used for implimentation to secure critical sections such as 'customer_mutex', 'timer_mutex' and 'machine_mutex'.

### <u>Timer Thread</u>:

- A dedicated timer thread updates the simulation time ('ticks') every second. This ensures that events in the simulation occur at the correct times.

### <u>Machine Thread</u>:

- The machine_thread function takes a machine ID as an argument.
- It extracts relevant machine information, including the start and stop times of the machine's operation.

- The thread enters a while loop to wait until the simulation time (represented by the ticks variable) reaches the start time of the machine. This synchronization ensures that the machine starts operating at the specified time.

- Upon reaching the machine's start time, the thread prints a message indicating that the machine has started working at the current simulation time.

- The thread enters an operational loop that continues until the simulation time reaches the stop time of the machine.
- Within this loop, the thread performs the following actions:

- The code checks whether the machine can start working. It uses conditional checks to ensure that only specific conditions are met:

  - The first machine (machine 1) can start if it is not already in use.
  - Other machines can start if they are not in use and the previous machine is in use.

- If the machine cannot start at the current time due to unmet conditions, it continues to the next iteration of the loop. This represents the time it takes for the machine to become available.
- If the machine meets the conditions and there is an order to process (indicated by the total_orders semaphore), the thread continues. The semaphore indicates the availability of a new order to be processed by the machine.

- The thread enters a critical section protected by the machine_mutex to prevent data races.
- It marks the machine as "in use" to ensure that no other machine starts working at the same time.
- It then iterates through the customer orders to find an order that can be processed. The thread checks conditions for order selection, including customer acceptance and ingredient availability.

- The code checks for ingredient availability by iterating through the selected order's toppings and verifying that there are enough ingredients in stock. If any ingredient is exhausted, the customer is marked with ingredient exhaustion, and the order is unfulfilled.

- The thread selects an order to process based on the available customer orders. It considers customer acceptance, unfulfilled orders, and the available time until the machine's stop time.
- Once an order is selected, the machine updates the order's attributes and marks it as "taken."

- Inside the critical section, the machine deducts the ingredients used for the selected order from the available stock. This ensures ingredient availability is accurate.

- The machine thread prints messages to indicate the start and completion of preparing the ice cream order. These messages include the machine ID, order ID, customer ID, and the current simulation time.

- The thread waits until the time for processing the order has passed, simulating the time required to prepare the order.

- Once the order processing time has elapsed, the machine thread prints a message indicating the completion of the order.

- The code updates the customer's record to mark the completion of the order. This includes incrementing the number of completed orders for the customer.
  _Machine Availability_:
- The machine marks itself as "not in use" to indicate that it is available for processing another order.

- The operational loop continues until the simulation time reaches the machine's stop time.

- Upon reaching the machine's stop time, the thread prints a message indicating that the machine has stopped working.

- Finally, the machine thread exits, completing its simulation of machine operation in the ice cream parlor.

### <u>Customer Thread</u>:

- The customer_thread function takes a customer ID as an argument.
- It extracts relevant customer information, including arrival time, the number of orders, and other details.

- The thread first attempts to acquire a seat at the parlor by using the sem_trywait function with the current_customers_in_parlor semaphore. If the semaphore is not available (indicating that no tables are available), the customer leaves due to unavailability and the thread exits, simulating the customer's departure.

- Upon successfully securing a table, the function updates the customer's status to indicate that they have been accepted into the parlor. This is done to prevent other customers from taking the available seats.

- Inside a critical section protected by the customer_mutex, the function prints messages to indicate the customer's arrival and order placement.
- It keeps track of the toppings that customers order, checking for ingredient availability.

- The code checks whether there are enough ingredients available for the customer's selected toppings. If not, the customer is marked as having ingredient exhaustion, and the "accepted" status is set to 0.

- If the customer's order cannot be fulfilled due to ingredient exhaustion, the function prints a message indicating that the customer left with an unfulfilled order.
- The "accepted" status is set to 0, indicating that the order was not accepted.

- The customer thread waits for one simulation second to represent the time it takes for the order to be placed.

- For each order placed by the customer, the thread signals the availability of a new order to the machines by posting to the total_orders semaphore.
- This action allows the machines to process the customer's orders.

- The thread enters a loop, checking if the customer's orders have been completed.
- It handles the following scenarios:

  - If ingredients are exhausted (ingredient exhaustion flag is set), the customer leaves, and the order is marked as unfulfilled.
  - If the simulation time exceeds the parlor's closing time, the customer leaves the parlor.
  - If all orders are completed, the customer collects their orders and leaves the parlor.

- When all of the customer's orders are completed, and the customer has not left due to ingredient exhaustion or reaching the closing time, the function prints a message indicating that the customer has collected their orders and left.
- The customer signals the availability of their seat by posting to the current_customers_in_parlor semaphore.

- Finally, the customer thread exits, effectively simulating the customer's complete experience in the ice cream parlor.

### Main Function:

- The 'main' function reads input parameters such as the number of machines, flavors, toppings, and customers. The end of input is indicated by pressing the 'Enter' Key two times more.
- It initializes semaphores and mutexes and creates threads for machines and customers based on the provided inputs.
- The simulation runs until the parlor's closing time, and upon completion, it prints information about customers who were not serviced due to machine unavailability and finally closes the parlor.

## Assumptions

I am not reserving the ingrediants for all the orders of the ice cream and there might be cases where partial orders of the customer are completed but they go wasted because all the orders can't be completed.

## Answer to Questions(Report)

### Question-1:
Minimizing Incomplete Orders: Describe your approach to redesign the simulation to minimize incomplete orders, given that incomplete orders can impact the parlor’s reputation. Note that the parlor’s reputation is unaffected if orders are instantly rejected due to topping shortages.

### ANSWER 1:

We can take the following action as the parlor's reputation won't be harmed if orders are refused right away: We start by making sure that all of the customers' orders with the shortest arrival times have the ingredients available. We reserve the right to reject a customer if ingredients are not available for even a single order. We are also bookeeping the machine status for the future, so if all the ingredients are present, we will check if the machine will be available for the customer's order to be created. If the machines aren't going to be available, we turn away the customer; if they are, we take their orders and begin preparation after updating our bookkeeping.

### Question-2:
Ingredient Replenishment: Assuming ingredients can be replenished by contacting the nearest supplier, outline how you would adjust the parlour order acceptance/rejection process based on ingredient availability.

### ANSWER 2:

We may state that there will always be an abundance of ingredients because they can be supplied. Therefore, all we have to do is make sure the machines are ready to receive orders.We can approach this in the same manner as described in response 1.


### Question-3:
Unserviced Orders: Suggest ways by which we can avoid/minimise the number of unserviced orders or customers having to wait until the parlor closes.

### ANSWER 3:

By taking the same method as in response 1, where we keep a record of all the ingredients and machinery, we can reduce the amount of unserviced orders.
