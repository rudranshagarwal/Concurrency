# studentinfo
> struct to store student information.

# machines
> is a semphore of machine with value eqaul to number of machines

# execute thread
> makes students threads, waits for arrival of student, after arriving student issues a timed wait for the machines. If the timed wait accepts, this means student got the machine, so it waits for this student to complete it's execution, If not the student leaves without washing. Corresponding wait times are added up and the total wait time, students who left without washing and need for washing machines is printed at the end.